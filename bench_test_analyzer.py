#!/usr/bin/env python3
"""
Bench Test Analyzer — MCU ↔ DRV8316 Focus
Runs core bench tests + Buck diagnostic + nFAULT timing,
collects ADC stream, generates HTML report.

Usage:
  python bench_test_analyzer.py              # auto-detect port
  python bench_test_analyzer.py COM5         # specify port
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

BAUD = 115200
TIMEOUT = 2
REPORT_DIR = Path(__file__).parent / "bench_reports"


def find_port():
    ports = serial.tools.list_ports.comports()
    for p in ports:
        d = (p.description or "").lower()
        if any(k in d for k in ["stlink", "stm32", "serial", "uart", "cp210", "ch340", "ftdi"]):
            return p.device
    return ports[0].device if ports else None


def send_cmd(ser, cmd, wait=12.0):
    ser.reset_input_buffer()
    ser.write((cmd + "\n").encode())
    time.sleep(0.1)
    lines, deadline = [], time.time() + wait
    while time.time() < deadline:
        raw = ser.readline()
        if raw:
            lines.append(raw.decode("utf-8", errors="replace").rstrip())
            deadline = min(deadline, time.time() + 0.8)
        elif lines:
            break
    return lines


def collect_stream(ser, cmd, duration=6.0):
    ser.reset_input_buffer()
    ser.write((cmd + "\n").encode())
    lines, end = [], time.time() + duration
    while time.time() < end:
        raw = ser.readline()
        if raw:
            lines.append(raw.decode("utf-8", errors="replace").rstrip())
    return lines


def parse_adc_stream(lines):
    data = {"PA0": [], "PA1": [], "PA2": [], "PB0": []}
    for line in lines:
        for key in data:
            m = re.search(rf"{key}=(\d+)", line)
            if m:
                data[key].append(int(m.group(1)))
    return {k: v for k, v in data.items() if v}


def parse_test_results(lines):
    results, current = [], None
    for line in lines:
        if "=== TEST" in line or "=== BUCK" in line or "=== nFAULT" in line or "=== DRV8316" in line:
            current = line.strip("= ")
        for pat, status in [("✓", "PASS"), ("PASS", "PASS"), ("✗", "FAIL"),
                            ("FAIL", "FAIL"), ("WRONG", "FAIL"), ("broken", "FAIL"),
                            ("DISABLED", "FAIL"), ("STILL", "FAIL"),
                            ("immediate re-fault", "FAIL"), ("persistent", "FAIL")]:
            if pat in line:
                results.append((status, current or "", line.strip()))
                break
    return results


def _svg_chart(data, title="", ylabel="", width=700, height=300, expected=None):
    if isinstance(data, list):
        data = {"value": data}
    m = {"t": 40, "r": 20, "b": 40, "l": 60}
    pw, ph = width - m["l"] - m["r"], height - m["t"] - m["b"]
    all_v = [v for vs in data.values() for v in vs]
    if not all_v:
        return "<p>No data</p>"
    vmin, vmax = min(all_v) * 0.95, max(all_v) * 1.05
    if vmax == vmin: vmax += 1
    max_n = max(len(v) for v in data.values())
    colors = ["#58a6ff", "#f0883e", "#3fb950"]
    parts = []
    for idx, (name, vals) in enumerate(data.items()):
        if not vals: continue
        c = colors[idx % len(colors)]
        pts = " ".join(f"{m['l'] + i / max(len(vals)-1,1) * pw:.1f},"
                       f"{m['t'] + ph - (v - vmin) / (vmax - vmin) * ph:.1f}"
                       for i, v in enumerate(vals))
        parts.append(f'<polyline points="{pts}" fill="none" stroke="{c}" stroke-width="1.5" opacity="0.85"/>')
        lx = m["l"] + 10 + idx * 120
        parts.append(f'<rect x="{lx}" y="{m["t"]-25}" width="12" height="12" fill="{c}"/>')
        parts.append(f'<text x="{lx+16}" y="{m["t"]-15}" font-size="11">{name}</text>')
    exp = ""
    if expected:
        ey = m["t"] + ph - (expected - vmin) / (vmax - vmin) * ph
        exp = (f'<line x1="{m["l"]}" y1="{ey:.0f}" x2="{width-m["r"]}" y2="{ey:.0f}" '
               f'stroke="#e74c3c" stroke-dasharray="6,3"/>'
               f'<text x="{width-m["r"]-90}" y="{ey-5}" font-size="10" fill="#e74c3c">expected~{expected}</text>')
    ticks = ""
    for i in range(5):
        val = vmin + (vmax - vmin) * i / 4
        y = m["t"] + ph - i / 4 * ph
        ticks += f'<text x="{m["l"]-5}" y="{y+4}" text-anchor="end" font-size="10">{val:.0f}</text>'
        ticks += f'<line x1="{m["l"]}" y1="{y}" x2="{width-m["r"]}" y2="{y}" stroke="#21262d"/>'
    return (f'<svg width="{width}" height="{height}" xmlns="http://www.w3.org/2000/svg">'
            f'<text x="{width//2}" y="15" text-anchor="middle" font-size="13" fill="#c9d1d9">{title}</text>'
            f'<text x="15" y="{height//2}" text-anchor="middle" font-size="11" '
            f'transform="rotate(-90,15,{height//2})">{ylabel}</text>'
            f'<text x="{width//2}" y="{height-5}" text-anchor="middle" font-size="11">Sample</text>'
            f'{ticks}{exp}{"".join(parts)}</svg>')


def section_html(title, lines):
    """Format a section of serial output as styled HTML block."""
    content = "\n".join(lines).replace("<", "&lt;").replace(">", "&gt;")
    return f'<h3>{title}</h3><pre style="max-height:300px;overflow-y:auto">{content}</pre>'


def generate_report(all_sections, results, adc_data, path):
    ts = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    np_ = sum(1 for r in results if r[0] == "PASS")
    nf = sum(1 for r in results if r[0] == "FAIL")

    adc_svg = _svg_chart(adc_data, "ADC Current Sense (idle)", "Raw 12-bit",
                         expected=2048) if adc_data else ""

    rows = ""
    for s, t, d in results:
        c = "#2ecc71" if s == "PASS" else "#e74c3c"
        rows += (f'<tr><td style="color:{c};font-weight:bold;font-size:1.3em">'
                 f'{"✓" if s=="PASS" else "✗"}</td><td>{t}</td>'
                 f'<td style="font-family:monospace;font-size:0.85em">{d}</td></tr>\n')

    adc_stats = ""
    if adc_data:
        for name, vals in adc_data.items():
            avg = sum(vals) / len(vals)
            v = avg * 3.3 / 4095
            mn, mx, noise = min(vals), max(vals), max(vals) - min(vals)
            ok = "✓" if 1.2 < v < 2.1 else "✗"
            adc_stats += (f"<tr><td>{name}</td><td>{avg:.0f}</td><td>{v:.3f}V</td>"
                          f"<td>{mn}</td><td>{mx}</td><td>{noise}</td><td>{ok}</td></tr>")

    sections_html = "\n".join(
        section_html(title, lines) for title, lines in all_sections
    )

    html = f"""<!DOCTYPE html><html lang="ko"><head><meta charset="utf-8">
<title>MCU↔DRV8316 Bench Report</title>
<style>
body{{font-family:'Segoe UI',sans-serif;max-width:900px;margin:2em auto;background:#0d1117;color:#c9d1d9}}
h1{{color:#58a6ff;border-bottom:2px solid #30363d;padding-bottom:.3em}}
h2{{color:#79c0ff;margin-top:2em}}
h3{{color:#d2a8ff;margin-top:1.5em}}
.summary{{display:flex;gap:2em;margin:1em 0}}
.stat{{background:#161b22;border-radius:12px;padding:1.2em 2em;border:1px solid #30363d;text-align:center}}
.stat .num{{font-size:2.5em;font-weight:bold}}
.pass .num{{color:#2ecc71}}.fail .num{{color:#e74c3c}}
table{{border-collapse:collapse;width:100%;margin:1em 0}}
th,td{{padding:8px 12px;border:1px solid #30363d;text-align:left}}
th{{background:#161b22;color:#8b949e}}
.chart{{background:#161b22;border-radius:8px;padding:1em;margin:1em 0;border:1px solid #30363d}}
pre{{background:#161b22;padding:1em;border-radius:8px;overflow-x:auto;font-size:.8em;
     border:1px solid #30363d;white-space:pre-wrap}}
svg text{{fill:#8b949e}}
</style></head><body>
<h1>🔧 MCU ↔ DRV8316 Bench Test Report</h1>
<p style="color:#8b949e">{ts}</p>
<div class="summary">
<div class="stat pass"><div class="num">{np_}</div>PASS</div>
<div class="stat fail"><div class="num">{nf}</div>FAIL</div>
<div class="stat"><div class="num">{np_+nf}</div>TOTAL</div>
</div>

<h2>📋 Test Results</h2>
<table><tr><th></th><th>Test</th><th>Detail</th></tr>{rows}</table>

<h2>📊 ADC Current Sense</h2>
{f'<div class="chart">{adc_svg}</div>' if adc_svg else '<p>No data</p>'}
{f'<table><tr><th>Channel</th><th>Avg</th><th>V</th><th>Min</th><th>Max</th><th>Noise</th><th>OK</th></tr>{adc_stats}</table>' if adc_stats else ''}

<h2>🔍 Detailed Test Logs</h2>
{sections_html}

</body></html>"""
    path.write_text(html, encoding="utf-8")
    return path


def main():
    port = None
    for arg in sys.argv[1:]:
        if arg.startswith("COM") or arg.startswith("/dev/"):
            port = arg
    if not port:
        port = find_port()
    if not port:
        print("❌ No serial port found. Usage: python bench_test_analyzer.py COM5")
        sys.exit(1)

    print(f"🔌 {port} @ {BAUD}")
    ser = serial.Serial(port, BAUD, timeout=TIMEOUT)
    time.sleep(2)
    ser.reset_input_buffer()

    all_sections = []
    all_lines = []

    # ── Phase 1: Core tests ──
    print("🧪 [1/5] Core bench tests (LED + DRV SPI + ADC + nFAULT)...")
    lines = send_cmd(ser, "r", wait=15)
    for l in lines: print(f"  {l}")
    all_sections.append(("Core Bench Tests", lines))
    all_lines += lines

    # ── Phase 2: Full fault dump ──
    print("\n📋 [2/5] DRV8316 full status dump...")
    lines = send_cmd(ser, "f", wait=8)
    for l in lines: print(f"  {l}")
    all_sections.append(("DRV8316 Full Status", lines))
    all_lines += lines

    # ── Phase 3: Buck diagnostic ──
    print("\n🔋 [3/5] Buck converter diagnostic...")
    lines = send_cmd(ser, "b", wait=10)
    for l in lines: print(f"  {l}")
    all_sections.append(("Buck Converter Diagnostic", lines))
    all_lines += lines

    # ── Phase 4: nFAULT timing ──
    print("\n⚡ [4/5] nFAULT timing analysis...")
    lines = send_cmd(ser, "n", wait=8)
    for l in lines: print(f"  {l}")
    all_sections.append(("nFAULT Timing Analysis", lines))
    all_lines += lines

    # ── Phase 5: ADC stream ──
    print("\n📊 [5/5] ADC stream (5s)...")
    adc_lines = collect_stream(ser, "a", 6)
    adc_data = parse_adc_stream(adc_lines)
    for name, vals in adc_data.items():
        avg = sum(vals) / len(vals) if vals else 0
        print(f"  {name}: {len(vals)} samples, avg={avg:.0f} ({avg*3.3/4095:.3f}V)")
    all_sections.append(("ADC Stream Raw", adc_lines))

    ser.close()

    # ── Generate report ──
    results = parse_test_results(all_lines)
    REPORT_DIR.mkdir(exist_ok=True)
    ts = datetime.now().strftime("%Y%m%d_%H%M%S")
    rp = REPORT_DIR / f"bench_{ts}.html"
    generate_report(all_sections, results, adc_data, rp)

    jp = REPORT_DIR / f"bench_{ts}.json"
    jp.write_text(json.dumps({
        "timestamp": ts,
        "sections": {title: lines for title, lines in all_sections},
        "results": [(s, t, d) for s, t, d in results],
        "adc": adc_data,
    }, indent=2, ensure_ascii=False), encoding="utf-8")

    print(f"\n✅ Report: {rp}")
    print(f"   Data:   {jp}")
    if sys.platform == "win32":
        os.startfile(str(rp))


if __name__ == "__main__":
    main()
