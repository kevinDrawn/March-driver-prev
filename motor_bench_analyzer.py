#!/usr/bin/env python3
"""
Motor Bench Analyzer — Automated data collection & visualization
Connects to MCU, sends 'Ba' (or B1-B4), parses [MBENCH] output,
generates matplotlib graphs + HTML report.

Usage:
  python motor_bench_analyzer.py              # auto-detect port, run all phases
  python motor_bench_analyzer.py COM5         # specify port
  python motor_bench_analyzer.py COM5 B1      # run specific phase only
"""

import serial
import serial.tools.list_ports
import time
import re
import sys
import os
import json
from datetime import datetime
from pathlib import Path

try:
    import matplotlib
    matplotlib.use('Agg')  # Non-interactive backend
    import matplotlib.pyplot as plt
    import matplotlib.gridspec as gridspec
    from matplotlib.patches import FancyBboxPatch
    import numpy as np
    HAS_MPL = True
except ImportError:
    HAS_MPL = False
    print("[!] matplotlib not installed -- graphs will be skipped")
    print("  Install: pip install matplotlib numpy")

BAUD = 115200
TIMEOUT = 2
REPORT_DIR = Path(__file__).parent / "bench_reports"

# ─── Dark theme colors ────────────────────────────────────────────────────────
COLORS = {
    'bg':       '#0d1117',
    'card':     '#161b22',
    'border':   '#30363d',
    'text':     '#c9d1d9',
    'text2':    '#8b949e',
    'blue':     '#58a6ff',
    'green':    '#3fb950',
    'orange':   '#f0883e',
    'red':      '#f85149',
    'purple':   '#bc8cff',
    'cyan':     '#76e3ea',
    'yellow':   '#e3b341',
}
MPL_COLORS = [COLORS['blue'], COLORS['orange'], COLORS['green'],
              COLORS['purple'], COLORS['cyan'], COLORS['yellow'], COLORS['red']]


def find_port():
    """Auto-detect serial port."""
    ports = serial.tools.list_ports.comports()
    for p in ports:
        d = (p.description or "").lower()
        if any(k in d for k in ["stlink", "stm32", "serial", "uart", "cp210", "ch340", "ftdi"]):
            return p.device
    return ports[0].device if ports else None


def collect_mbench(ser, cmd="Ba", timeout=300):
    """Send bench command and collect all [MBENCH] lines until COMPLETE or timeout."""
    ser.reset_input_buffer()
    ser.write((cmd + "\n").encode())
    time.sleep(0.2)

    lines = []
    all_lines = []
    deadline = time.time() + timeout
    last_data = time.time()
    while time.time() < deadline:
        raw = ser.readline()
        if raw:
            line = raw.decode("utf-8", errors="replace").rstrip()
            all_lines.append(line)
            if "[MBENCH]" in line:
                lines.append(line)
                last_data = time.time()
            if "BENCH TEST COMPLETE" in line:
                break
        else:
            # No data for 60s after last [MBENCH] line = done
            if lines and (time.time() - last_data > 60):
                break
    return lines, all_lines


def parse_phases(lines):
    """Parse [MBENCH] lines into phase data structures."""
    phases = {}
    current_phase = None
    header = None

    for line in lines:
        text = line.replace("[MBENCH] ", "")

        # Phase start
        m = re.match(r"PHASE(\d)_START\s+(\w+)", text)
        if m:
            pnum = int(m.group(1))
            pname = m.group(2)
            current_phase = pnum
            phases[pnum] = {"name": pname, "header": [], "data": [], "raw": []}
            continue

        # Phase end
        if re.match(r"PHASE\d_END", text):
            current_phase = None
            header = None
            continue

        # Header
        if text.startswith("H:") and current_phase is not None:
            header = text[2:].split(",")
            phases[current_phase]["header"] = header
            continue

        # Data
        if text.startswith("D:") and current_phase is not None and header:
            vals = text[2:].split(",")
            row = {}
            for i, h in enumerate(header):
                if i < len(vals):
                    try:
                        row[h] = float(vals[i])
                    except ValueError:
                        row[h] = vals[i]
            phases[current_phase]["data"].append(row)
            phases[current_phase]["raw"].append(vals)
            continue

        # Sysinfo, fault, summary lines
        if current_phase is not None:
            phases.get(current_phase, {}).get("raw", []).append(text)

    return phases


def setup_mpl_style():
    """Configure matplotlib dark theme."""
    plt.rcParams.update({
        'figure.facecolor': COLORS['bg'],
        'axes.facecolor': COLORS['card'],
        'axes.edgecolor': COLORS['border'],
        'axes.labelcolor': COLORS['text'],
        'text.color': COLORS['text'],
        'xtick.color': COLORS['text2'],
        'ytick.color': COLORS['text2'],
        'grid.color': COLORS['border'],
        'grid.alpha': 0.5,
        'font.family': 'sans-serif',
        'font.sans-serif': ['Segoe UI', 'Inter', 'Roboto', 'DejaVu Sans'],
        'font.size': 10,
        'axes.titlesize': 13,
        'axes.labelsize': 11,
    })


def plot_phase1(data, save_dir):
    """Phase 1: Static Current — I vs Vq with theoretical overlay."""
    if not data:
        return None
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))
    fig.suptitle("Phase 1: Static Current Characterization", fontsize=15,
                 fontweight='bold', color=COLORS['blue'])

    vq = [d.get('Vq_cmd', 0) for d in data]
    irms = [d.get('Irms', 0) for d in data]
    ia = [d.get('Ia_avg', 0) for d in data]
    ib = [d.get('Ib_avg', 0) for d in data]
    ic = [d.get('Ic_avg', 0) for d in data]
    ia_std = [d.get('Ia_std', 0) for d in data]
    ib_std = [d.get('Ib_std', 0) for d in data]
    faults = [d.get('nFAULT', 1) for d in data]

    # Left: Irms vs Vq
    ax1.bar(vq, irms, width=0.15, color=COLORS['blue'], alpha=0.8, label='Measured Irms')
    # Theoretical: I = V / (2 × R_phase) using sqrt(3) for line current
    R = 0.32
    vq_th = np.linspace(0, max(vq) * 1.1, 100)
    i_th = vq_th / (2 * R)  # Two phases in series
    ax1.plot(vq_th, i_th, '--', color=COLORS['red'], linewidth=1.5,
             label=f'Theory I=V/(2R), R={R}Ω')
    ax1.set_xlabel("Applied Voltage Vq (V)")
    ax1.set_ylabel("Current Irms (A)")
    ax1.set_title("Current vs Voltage")
    ax1.legend(fontsize=9)
    ax1.grid(True, alpha=0.3)
    # Mark faults
    for i, f in enumerate(faults):
        if f == 0:
            ax1.annotate('⚠', (vq[i], irms[i]), fontsize=14, color=COLORS['red'],
                         ha='center', va='bottom')

    # Right: Phase currents with error bars
    x = np.arange(len(vq))
    w = 0.25
    ax2.bar(x - w, [abs(v) for v in ia], w, yerr=ia_std,
            color=COLORS['blue'], alpha=0.8, label='|Ia|', capsize=3)
    ax2.bar(x, [abs(v) for v in ib], w, yerr=ib_std,
            color=COLORS['orange'], alpha=0.8, label='|Ib|', capsize=3)
    ax2.bar(x + w, [abs(v) for v in ic], w,
            color=COLORS['green'], alpha=0.8, label='|Ic|')
    ax2.set_xlabel("Vq Step")
    ax2.set_ylabel("Phase Current (A)")
    ax2.set_title("Per-Phase Current Distribution")
    ax2.set_xticks(x)
    ax2.set_xticklabels([f"{v:.1f}V" for v in vq], rotation=45, fontsize=8)
    ax2.legend(fontsize=9)
    ax2.grid(True, alpha=0.3)

    plt.tight_layout()
    path = save_dir / "phase1_static_current.png"
    fig.savefig(path, dpi=150, bbox_inches='tight')
    plt.close(fig)
    return path


def plot_phase2(data, save_dir):
    """Phase 2: Speed Sweep — Current and speed tracking vs command."""
    if not data:
        return None
    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    fig.suptitle("Phase 2: Speed Sweep Current Profile", fontsize=15,
                 fontweight='bold', color=COLORS['orange'])

    spd_cmd = [d.get('Spd_cmd', 0) for d in data]
    spd_meas = [d.get('Spd_meas', 0) for d in data]
    irms = [d.get('Irms', 0) for d in data]
    vq = [d.get('Vq', 0) for d in data]
    id_val = [d.get('Id_avg', 0) for d in data]
    iq_val = [d.get('Iq_avg', 0) for d in data]

    # Top-left: Speed tracking
    ax = axes[0, 0]
    ax.plot(spd_cmd, spd_cmd, '--', color=COLORS['text2'], label='Command', linewidth=1)
    ax.plot(spd_cmd, spd_meas, 'o-', color=COLORS['blue'], markersize=6, label='Measured')
    ax.set_xlabel("Commanded Speed (rad/s)")
    ax.set_ylabel("Measured Speed (rad/s)")
    ax.set_title("Speed Tracking")
    ax.legend(fontsize=9)
    ax.grid(True, alpha=0.3)

    # Top-right: Irms vs speed
    ax = axes[0, 1]
    ax.fill_between(spd_cmd, 0, irms, alpha=0.3, color=COLORS['orange'])
    ax.plot(spd_cmd, irms, 'o-', color=COLORS['orange'], markersize=6, linewidth=2)
    ax.axhline(y=1.5, color=COLORS['red'], linestyle='--', alpha=0.5, label='I_MAX_SAFE')
    ax.set_xlabel("Speed (rad/s)")
    ax.set_ylabel("RMS Current (A)")
    ax.set_title("Current vs Speed")
    ax.legend(fontsize=9)
    ax.grid(True, alpha=0.3)

    # Bottom-left: Id/Iq decomposition
    ax = axes[1, 0]
    ax.plot(spd_cmd, id_val, 's-', color=COLORS['purple'], markersize=5, label='Id (flux)')
    ax.plot(spd_cmd, iq_val, 'o-', color=COLORS['cyan'], markersize=5, label='Iq (torque)')
    ax.set_xlabel("Speed (rad/s)")
    ax.set_ylabel("Current (A)")
    ax.set_title("DQ Current Decomposition")
    ax.legend(fontsize=9)
    ax.grid(True, alpha=0.3)

    # Bottom-right: Voltage (Vq) vs speed
    ax = axes[1, 1]
    # BEMF theoretical
    Ke = 0.088
    spd_th = np.linspace(0, max(spd_cmd) * 1.1, 100)
    bemf = Ke * spd_th
    ax.fill_between(spd_th, 0, bemf, alpha=0.2, color=COLORS['green'])
    ax.plot(spd_th, bemf, '--', color=COLORS['green'], label=f'BEMF (Ke={Ke})', linewidth=1.5)
    ax.plot(spd_cmd, vq, 'o-', color=COLORS['yellow'], markersize=6, label='Applied Vq')
    ax.set_xlabel("Speed (rad/s)")
    ax.set_ylabel("Voltage (V)")
    ax.set_title("Voltage & Back-EMF vs Speed")
    ax.legend(fontsize=9)
    ax.grid(True, alpha=0.3)

    plt.tight_layout()
    path = save_dir / "phase2_speed_sweep.png"
    fig.savefig(path, dpi=150, bbox_inches='tight')
    plt.close(fig)
    return path


def plot_phase3(data, save_dir):
    """Phase 3: OCP Test — Voltage sweep with OCP trip markers."""
    if not data:
        return None
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))
    fig.suptitle("Phase 3: OCP Limit Exploration", fontsize=15,
                 fontweight='bold', color=COLORS['red'])

    vlim = [d.get('Vlim', 0) for d in data]
    irms = [d.get('Irms', 0) for d in data]
    ocp_trips = [d.get('OCP_trips', 0) for d in data]
    faults = [d.get('nFAULT', 1) for d in data]

    # Left: Current vs Vlim with OCP markers
    ax1.plot(vlim, irms, 'o-', color=COLORS['blue'], markersize=8, linewidth=2, label='Irms')
    for i, trips in enumerate(ocp_trips):
        if isinstance(trips, (int, float)) and trips > 0:
            ax1.plot(vlim[i], irms[i], 'X', color=COLORS['red'], markersize=15, zorder=5)
            ax1.annotate(f'{int(trips)} trips', (vlim[i], irms[i]),
                         textcoords="offset points", xytext=(10, 5),
                         fontsize=8, color=COLORS['red'])
    ax1.axhline(y=4.0, color=COLORS['red'], linestyle='--', alpha=0.5, label='OCP Level (4A)')
    ax1.set_xlabel("Voltage Limit (V)")
    ax1.set_ylabel("RMS Current (A)")
    ax1.set_title("Current vs Voltage Limit")
    ax1.legend(fontsize=9)
    ax1.grid(True, alpha=0.3)

    # Right: OCP trip count bar chart
    colors = [COLORS['green'] if (isinstance(t, (int, float)) and t == 0)
              else COLORS['yellow'] if (isinstance(t, (int, float)) and t < 10)
              else COLORS['red']
              for t in ocp_trips]
    trip_vals = [int(t) if isinstance(t, (int, float)) else 0 for t in ocp_trips]
    ax2.bar([f"{v:.1f}V" for v in vlim], trip_vals, color=colors, alpha=0.8)
    ax2.set_xlabel("Voltage Limit")
    ax2.set_ylabel("OCP Trip Count (3s window)")
    ax2.set_title("OCP AutoRetry Events")
    ax2.tick_params(axis='x', rotation=45)
    ax2.grid(True, alpha=0.3)

    plt.tight_layout()
    path = save_dir / "phase3_ocp_test.png"
    fig.savefig(path, dpi=150, bbox_inches='tight')
    plt.close(fig)
    return path


def plot_phase4(data, save_dir):
    """Phase 4: Sustained Run — Time series with fault/OTW markers."""
    if not data:
        return None
    fig, axes = plt.subplots(3, 1, figsize=(14, 10), sharex=True)
    fig.suptitle("Phase 4: Sustained Run (30 seconds)", fontsize=15,
                 fontweight='bold', color=COLORS['green'])

    t = [d.get('Time_ms', 0) / 1000.0 for d in data]  # Convert to seconds
    spd = [d.get('Spd_meas', 0) for d in data]
    irms = [d.get('Irms', 0) for d in data]
    id_val = [d.get('Id', 0) for d in data]
    iq_val = [d.get('Iq', 0) for d in data]
    ia = [d.get('Ia', 0) for d in data]
    ib = [d.get('Ib', 0) for d in data]
    ic = [d.get('Ic', 0) for d in data]
    faults = [d.get('nFAULT', 1) for d in data]
    otw = [d.get('OTW', 0) for d in data]

    # Top: Speed
    ax = axes[0]
    ax.plot(t, spd, color=COLORS['blue'], linewidth=1, alpha=0.8)
    if spd:
        avg_spd = sum(spd) / len(spd)
        ax.axhline(y=avg_spd, color=COLORS['blue'], linestyle='--', alpha=0.4)
        ax.fill_between(t, avg_spd - 2, avg_spd + 2, alpha=0.1, color=COLORS['blue'])
    ax.set_ylabel("Speed (rad/s)")
    ax.set_title("Velocity Tracking")
    ax.grid(True, alpha=0.3)

    # Middle: Current
    ax = axes[1]
    ax.plot(t, irms, color=COLORS['orange'], linewidth=1.5, label='Irms', alpha=0.9)
    ax.plot(t, ia, color=COLORS['blue'], linewidth=0.7, alpha=0.4, label='Ia')
    ax.plot(t, ib, color=COLORS['green'], linewidth=0.7, alpha=0.4, label='Ib')
    if irms:
        avg_i = sum(irms) / len(irms)
        std_i = (sum((x - avg_i)**2 for x in irms) / len(irms)) ** 0.5
        ax.axhline(y=avg_i, color=COLORS['orange'], linestyle='--', alpha=0.4)
        ax.fill_between(t, avg_i - std_i, avg_i + std_i, alpha=0.15, color=COLORS['orange'])
        ax.text(0.98, 0.95, f'avg={avg_i:.3f}A  σ={std_i:.3f}A',
                transform=ax.transAxes, fontsize=9, ha='right', va='top',
                color=COLORS['text2'])
    ax.axhline(y=1.5, color=COLORS['red'], linestyle=':', alpha=0.4, label='I_MAX_SAFE')
    ax.set_ylabel("Current (A)")
    ax.set_title("Current Stability")
    ax.legend(fontsize=8, loc='upper left')
    ax.grid(True, alpha=0.3)

    # Bottom: Fault & OTW events
    ax = axes[2]
    fault_times = [t[i] for i in range(len(faults)) if faults[i] == 0]
    otw_times = [t[i] for i in range(len(otw))
                 if isinstance(otw[i], (int, float)) and otw[i] > 0]
    ax.eventplot([fault_times], colors=[COLORS['red']], linewidths=2,
                 linelengths=0.8, lineoffsets=1.5, label='nFAULT')
    ax.eventplot([otw_times], colors=[COLORS['yellow']], linewidths=2,
                 linelengths=0.8, lineoffsets=0.5, label='OTW')
    ax.set_ylabel("Events")
    ax.set_xlabel("Time (seconds)")
    ax.set_title("Fault & Temperature Events")
    ax.set_yticks([0.5, 1.5])
    ax.set_yticklabels(["OTW", "nFAULT"])
    ax.set_ylim(0, 2.5)
    ax.legend(fontsize=8)
    ax.grid(True, alpha=0.3)

    plt.tight_layout()
    path = save_dir / "phase4_sustained.png"
    fig.savefig(path, dpi=150, bbox_inches='tight')
    plt.close(fig)
    return path


def plot_dashboard(phases, save_dir):
    """Combined 2x2 overview dashboard."""
    fig = plt.figure(figsize=(16, 12))
    fig.suptitle("Motor Driver Bench Test — Dashboard",
                 fontsize=18, fontweight='bold', color=COLORS['blue'], y=0.98)
    gs = gridspec.GridSpec(2, 2, hspace=0.35, wspace=0.3)

    # Phase 1 summary
    ax = fig.add_subplot(gs[0, 0])
    if 1 in phases and phases[1]['data']:
        d = phases[1]['data']
        vq = [x.get('Vq_cmd', 0) for x in d]
        irms = [x.get('Irms', 0) for x in d]
        ax.bar(vq, irms, width=0.12, color=COLORS['blue'], alpha=0.8)
        R = 0.32
        vq_th = np.linspace(0, max(vq)*1.1, 50)
        ax.plot(vq_th, vq_th/(2*R), '--', color=COLORS['red'], linewidth=1, alpha=0.6)
    ax.set_title("① Static Current", fontweight='bold', color=COLORS['blue'])
    ax.set_xlabel("Vq (V)")
    ax.set_ylabel("Irms (A)")
    ax.grid(True, alpha=0.3)

    # Phase 2 summary
    ax = fig.add_subplot(gs[0, 1])
    if 2 in phases and phases[2]['data']:
        d = phases[2]['data']
        spd = [x.get('Spd_cmd', 0) for x in d]
        irms = [x.get('Irms', 0) for x in d]
        ax.fill_between(spd, 0, irms, alpha=0.3, color=COLORS['orange'])
        ax.plot(spd, irms, 'o-', color=COLORS['orange'], markersize=5)
        ax.axhline(y=1.5, color=COLORS['red'], linestyle='--', alpha=0.4)
    ax.set_title("② Speed Sweep", fontweight='bold', color=COLORS['orange'])
    ax.set_xlabel("Speed (rad/s)")
    ax.set_ylabel("Irms (A)")
    ax.grid(True, alpha=0.3)

    # Phase 3 summary
    ax = fig.add_subplot(gs[1, 0])
    if 3 in phases and phases[3]['data']:
        d = phases[3]['data']
        vlim = [x.get('Vlim', 0) for x in d]
        trips = [int(x.get('OCP_trips', 0)) if isinstance(x.get('OCP_trips', 0), (int, float)) else 0 for x in d]
        colors = [COLORS['green'] if t == 0 else COLORS['yellow'] if t < 10 else COLORS['red'] for t in trips]
        ax.bar([f"{v:.1f}" for v in vlim], trips, color=colors, alpha=0.8)
    ax.set_title("③ OCP Test", fontweight='bold', color=COLORS['red'])
    ax.set_xlabel("Vlim (V)")
    ax.set_ylabel("OCP Trips")
    ax.tick_params(axis='x', rotation=45)
    ax.grid(True, alpha=0.3)

    # Phase 4 summary
    ax = fig.add_subplot(gs[1, 1])
    if 4 in phases and phases[4]['data']:
        d = phases[4]['data']
        t = [x.get('Time_ms', 0)/1000 for x in d]
        irms = [x.get('Irms', 0) for x in d]
        ax.plot(t, irms, color=COLORS['green'], linewidth=1, alpha=0.8)
        if irms:
            avg = sum(irms)/len(irms)
            ax.axhline(y=avg, color=COLORS['green'], linestyle='--', alpha=0.4)
    ax.set_title("④ Sustained Run", fontweight='bold', color=COLORS['green'])
    ax.set_xlabel("Time (s)")
    ax.set_ylabel("Irms (A)")
    ax.grid(True, alpha=0.3)

    path = save_dir / "dashboard.png"
    fig.savefig(path, dpi=150, bbox_inches='tight')
    plt.close(fig)
    return path


def generate_html_report(phases, images, all_lines, save_dir):
    """Generate comprehensive HTML report."""
    ts = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

    # Phase statistics
    stats = {}
    for pnum, pdata in phases.items():
        d = pdata['data']
        if not d:
            continue
        s = {'name': pdata['name'], 'count': len(d)}
        if pnum == 1:
            irms_vals = [x.get('Irms', 0) for x in d]
            s['max_current'] = max(irms_vals) if irms_vals else 0
            s['fault_count'] = sum(1 for x in d if x.get('nFAULT', 1) == 0)
        elif pnum == 2:
            irms_vals = [x.get('Irms', 0) for x in d]
            spd_vals = [x.get('Spd_meas', 0) for x in d]
            s['max_speed'] = max(spd_vals) if spd_vals else 0
            s['max_current'] = max(irms_vals) if irms_vals else 0
        elif pnum == 3:
            trips = [x.get('OCP_trips', 0) for x in d if isinstance(x.get('OCP_trips', 0), (int, float))]
            s['total_ocp'] = sum(int(t) for t in trips)
            s['first_trip_vlim'] = next((x.get('Vlim', 0) for x in d
                                         if isinstance(x.get('OCP_trips', 0), (int, float))
                                         and x.get('OCP_trips', 0) > 0), None)
        elif pnum == 4:
            irms_vals = [x.get('Irms', 0) for x in d]
            s['avg_current'] = sum(irms_vals)/len(irms_vals) if irms_vals else 0
            s['std_current'] = (sum((x - s['avg_current'])**2 for x in irms_vals)/len(irms_vals))**0.5 if irms_vals else 0
            s['fault_count'] = sum(1 for x in d if x.get('nFAULT', 1) == 0)
            s['otw_count'] = sum(1 for x in d if isinstance(x.get('OTW', 0), (int, float)) and x.get('OTW', 0) > 0)
        stats[pnum] = s

    # Image embeddings
    img_html = ""
    for name, path in images.items():
        if path and path.exists():
            import base64
            with open(path, 'rb') as f:
                b64 = base64.b64encode(f.read()).decode()
            img_html += f'<h3>{name}</h3>\n'
            img_html += f'<img src="data:image/png;base64,{b64}" style="max-width:100%;border-radius:8px;">\n'

    # Build stats cards
    cards = ""
    for pnum, s in stats.items():
        color = [COLORS['blue'], COLORS['orange'], COLORS['red'], COLORS['green']][pnum-1]
        card = f'<div class="stat-card" style="border-left:3px solid {color}">'
        card += f'<h4 style="color:{color}">Phase {pnum}: {s["name"]}</h4>'
        card += f'<p>{s["count"]} data points</p>'
        if 'max_current' in s:
            card += f'<p>Max Current: <b>{s["max_current"]:.3f} A</b></p>'
        if 'max_speed' in s:
            card += f'<p>Max Speed: <b>{s["max_speed"]:.1f} rad/s</b></p>'
        if 'total_ocp' in s:
            card += f'<p>Total OCP trips: <b>{s["total_ocp"]}</b></p>'
            if s.get('first_trip_vlim'):
                card += f'<p>First trip at: <b>{s["first_trip_vlim"]:.1f}V</b></p>'
        if 'avg_current' in s:
            card += f'<p>Avg Current: <b>{s["avg_current"]:.3f} ± {s["std_current"]:.3f} A</b></p>'
        if 'fault_count' in s:
            fc = s['fault_count']
            card += f'<p>Fault events: <b style="color:{"#f85149" if fc > 0 else "#3fb950"}">{fc}</b></p>'
        if 'otw_count' in s:
            oc = s['otw_count']
            card += f'<p>OTW events: <b style="color:{"#e3b341" if oc > 0 else "#3fb950"}">{oc}</b></p>'
        card += '</div>\n'
        cards += card

    # Raw log
    raw_log = "\n".join(all_lines[-200:]).replace("<", "&lt;").replace(">", "&gt;")

    html = f"""<!DOCTYPE html><html lang="ko"><head><meta charset="utf-8">
<title>Motor Driver Bench Test Report</title>
<style>
body {{font-family:'Segoe UI','Inter',sans-serif;max-width:1100px;margin:2em auto;
  background:{COLORS['bg']};color:{COLORS['text']};padding:0 2em}}
h1 {{color:{COLORS['blue']};border-bottom:2px solid {COLORS['border']};padding-bottom:.3em;font-size:1.8em}}
h2 {{color:#79c0ff;margin-top:2em}}
h3 {{color:{COLORS['purple']};margin-top:1.5em}}
.stats-grid {{display:grid;grid-template-columns:repeat(auto-fit,minmax(250px,1fr));gap:1em;margin:1.5em 0}}
.stat-card {{background:{COLORS['card']};border-radius:12px;padding:1.2em;border:1px solid {COLORS['border']}}}
.stat-card h4 {{margin:0 0 0.5em 0}}
.stat-card p {{margin:0.3em 0;color:{COLORS['text2']};font-size:0.9em}}
.stat-card b {{color:{COLORS['text']}}}
img {{margin:1em 0;border:1px solid {COLORS['border']}}}
pre {{background:{COLORS['card']};padding:1em;border-radius:8px;overflow-x:auto;font-size:.75em;
  border:1px solid {COLORS['border']};max-height:400px;overflow-y:auto;white-space:pre-wrap}}
table {{border-collapse:collapse;width:100%;margin:1em 0;font-size:0.85em}}
th,td {{padding:6px 10px;border:1px solid {COLORS['border']};text-align:left}}
th {{background:{COLORS['card']};color:{COLORS['text2']}}}
.pass {{color:{COLORS['green']}}} .fail {{color:{COLORS['red']}}} .warn {{color:{COLORS['yellow']}}}
</style></head><body>
<h1>🔧 Motor Driver Bench Test Report</h1>
<p style="color:{COLORS['text2']}">{ts}</p>

<h2>📊 Summary</h2>
<div class="stats-grid">{cards}</div>

<h2>📈 Graphs</h2>
{img_html}

<h2>📋 Raw Data Tables</h2>
"""
    # Add data tables for each phase
    for pnum, pdata in phases.items():
        if not pdata['data']:
            continue
        html += f'<h3>Phase {pnum}: {pdata["name"]}</h3>\n<table>\n<tr>'
        for h in pdata['header']:
            html += f'<th>{h}</th>'
        html += '</tr>\n'
        for row in pdata['data']:
            html += '<tr>'
            for h in pdata['header']:
                val = row.get(h, '')
                if isinstance(val, float):
                    html += f'<td>{val:.4f}</td>'
                else:
                    html += f'<td>{val}</td>'
            html += '</tr>\n'
        html += '</table>\n'

    html += f"""
<h2>🔍 Serial Log (last 200 lines)</h2>
<pre>{raw_log}</pre>
</body></html>"""

    report_path = save_dir / f"motor_bench_{datetime.now().strftime('%Y%m%d_%H%M%S')}.html"
    report_path.write_text(html, encoding="utf-8")
    return report_path


def main():
    port = None
    cmd = "Ba"
    for arg in sys.argv[1:]:
        if arg.startswith("COM") or arg.startswith("/dev/"):
            port = arg
        elif arg.startswith("B"):
            cmd = arg

    if not port:
        port = find_port()
    if not port:
        print("[X] No serial port found.")
        print("   Available ports:")
        for p in serial.tools.list_ports.comports():
            print(f"     {p.device}: {p.description}")
        print("\n   Usage: python motor_bench_analyzer.py COM6 [Ba|B1|B2|B3|B4]")
        sys.exit(1)

    print(f"[>] Port: {port} @ {BAUD}")
    print(f"[>] Command: {cmd}")
    print(f"{'='*60}")

    ser = serial.Serial(port, BAUD, timeout=TIMEOUT)
    time.sleep(2)
    ser.reset_input_buffer()

    # ── Collect data ──
    print(f"[>] Sending '{cmd}' -- collecting data...")
    timeout = 300 if cmd == "Ba" else 120  # Ba takes ~3+ minutes
    mbench_lines, all_lines = collect_mbench(ser, cmd, timeout)
    ser.close()

    print(f"[>] Collected {len(mbench_lines)} [MBENCH] lines ({len(all_lines)} total)")

    if not mbench_lines:
        print("[X] No [MBENCH] data received!")
        print("Last 20 lines received:")
        for l in all_lines[-20:]:
            print(f"  {l}")
        sys.exit(1)

    # ── Parse ──
    phases = parse_phases(mbench_lines)
    phase_info = [f'Phase {k} ({v["name"]}, {len(v["data"])} pts)' for k, v in phases.items()]
    print(f"[>] Parsed {len(phases)} phases: {phase_info}")

    # ── Save raw data ──
    REPORT_DIR.mkdir(exist_ok=True)
    ts = datetime.now().strftime("%Y%m%d_%H%M%S")
    json_path = REPORT_DIR / f"motor_bench_{ts}.json"
    json_path.write_text(json.dumps({
        "timestamp": ts,
        "command": cmd,
        "phases": {str(k): {"name": v["name"], "header": v["header"],
                             "data": v["data"]} for k, v in phases.items()},
        "raw_lines": all_lines,
    }, indent=2, ensure_ascii=False), encoding="utf-8")
    print(f"[>] Raw data: {json_path}")

    # ── Generate graphs ──
    images = {}
    if HAS_MPL:
        setup_mpl_style()
        print("[>] Generating graphs...")

        if 1 in phases:
            images["Phase 1: Static Current"] = plot_phase1(phases[1]['data'], REPORT_DIR)
        if 2 in phases:
            images["Phase 2: Speed Sweep"] = plot_phase2(phases[2]['data'], REPORT_DIR)
        if 3 in phases:
            images["Phase 3: OCP Test"] = plot_phase3(phases[3]['data'], REPORT_DIR)
        if 4 in phases:
            images["Phase 4: Sustained Run"] = plot_phase4(phases[4]['data'], REPORT_DIR)
        if len(phases) >= 2:
            images["Dashboard"] = plot_dashboard(phases, REPORT_DIR)

        for name, path in images.items():
            if path:
                print(f"  [OK] {name}: {path.name}")

    # ── Generate HTML report ──
    report_path = generate_html_report(phases, images, all_lines, REPORT_DIR)
    print(f"\n[OK] Report: {report_path}")

    # ── Print summary ──
    print(f"\n{'='*60}")
    print("[SUMMARY]")
    print(f"{'='*60}")
    for pnum, pdata in phases.items():
        d = pdata['data']
        if not d:
            continue
        print(f"\n  Phase {pnum} ({pdata['name']}): {len(d)} data points")
        if pnum == 1:
            irms = [x.get('Irms', 0) for x in d]
            print(f"    Max Irms: {max(irms):.3f} A at Vq={d[irms.index(max(irms))].get('Vq_cmd', '?')}V")
        elif pnum == 2:
            irms = [x.get('Irms', 0) for x in d]
            spd = [x.get('Spd_meas', 0) for x in d]
            print(f"    Max speed: {max(spd):.1f} rad/s, Max Irms: {max(irms):.3f} A")
        elif pnum == 3:
            trips = [int(x.get('OCP_trips', 0)) for x in d if isinstance(x.get('OCP_trips', 0), (int, float))]
            print(f"    Total OCP trips: {sum(trips)}")
        elif pnum == 4:
            irms = [x.get('Irms', 0) for x in d]
            if irms:
                avg = sum(irms)/len(irms)
                std = (sum((x-avg)**2 for x in irms)/len(irms))**0.5
                print(f"    Current: {avg:.3f} ± {std:.3f} A (stability: {'GOOD' if std < 0.1 else 'FAIR' if std < 0.3 else 'POOR'})")
                faults = sum(1 for x in d if x.get('nFAULT', 1) == 0)
                print(f"    Faults: {faults}/{len(d)}")

    if sys.platform == "win32" and report_path.exists():
        os.startfile(str(report_path))


if __name__ == "__main__":
    main()
