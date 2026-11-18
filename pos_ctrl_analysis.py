"""
pos_ctrl_analysis.py -- Position control step response analysis
Reads CSV log from position control test, extracts step response metrics.
"""
import re, sys, os
import numpy as np
import matplotlib
import matplotlib.pyplot as plt

matplotlib.rcParams['font.family'] = 'Malgun Gothic'
matplotlib.rcParams['axes.unicode_minus'] = False

def load_log(logfile):
    ts, target, angle, error, vel, vq = [], [], [], [], [], []
    with open(logfile, "r", encoding="utf-8", errors="ignore") as f:
        for line in f:
            line = line.strip()
            # Match 6-column CSV: ts_ms,target_deg,angle_deg,error_deg,velocity,Vq
            m = re.match(r'^(\d+),([-\d.]+),([-\d.]+),([-\d.]+),([-\d.]+),([-\d.]+)$', line)
            if m:
                ts.append(int(m.group(1)))
                target.append(float(m.group(2)))
                angle.append(float(m.group(3)))
                error.append(float(m.group(4)))
                vel.append(float(m.group(5)))
                vq.append(float(m.group(6)))
    return (np.array(ts), np.array(target), np.array(angle),
            np.array(error), np.array(vel), np.array(vq))

def find_step_events(ts, target, min_change=5.0, settle_window=500):
    """Find indices where target changes significantly"""
    steps = []
    i = 1
    while i < len(target):
        if abs(target[i] - target[i-1]) > min_change:
            step_start = i
            new_target = target[i]
            # Find where target stabilizes
            while i < len(target) and target[i] == new_target:
                i += 1
            steps.append({
                'idx': step_start,
                'ts': ts[step_start],
                'from': target[step_start-1],
                'to': new_target,
                'end_idx': min(i, len(target)-1)
            })
        else:
            i += 1
    return steps

def analyze_step(ts, angle, target_val, step_start, step_end):
    """Analyze a single step response"""
    # Time relative to step start (seconds)
    t_rel = (ts[step_start:step_end] - ts[step_start]) / 1000.0
    ang = angle[step_start:step_end]
    
    if len(t_rel) < 10:
        return None
    
    # Step magnitude
    initial = ang[0]
    step_size = target_val - initial
    if abs(step_size) < 1.0:
        return None
    
    # Settling time (within 2% of target)
    band = abs(step_size) * 0.02
    settled_idx = None
    for i in range(len(ang)):
        if abs(ang[i] - target_val) < max(band, 0.5):  # at least 0.5 deg
            # Check if it stays settled for 200ms
            stay_settled = True
            check_end = min(i + int(0.2 / (t_rel[1]-t_rel[0]+1e-6)), len(ang))
            for j in range(i, check_end):
                if abs(ang[j] - target_val) > max(band, 0.5):
                    stay_settled = False
                    break
            if stay_settled:
                settled_idx = i
                break
    
    settling_time = t_rel[settled_idx] if settled_idx else None
    
    # Overshoot
    if step_size > 0:
        peak = np.max(ang)
        overshoot_deg = peak - target_val
    else:
        peak = np.min(ang)
        overshoot_deg = target_val - peak
    overshoot_pct = (overshoot_deg / abs(step_size)) * 100 if abs(step_size) > 0 else 0
    
    # Steady state error (last 20% of data)
    ss_start = int(len(ang) * 0.8)
    ss_error = np.mean(ang[ss_start:]) - target_val
    ss_std = np.std(ang[ss_start:])
    
    return {
        'step_size': step_size,
        'settling_time': settling_time,
        'overshoot_deg': max(0, overshoot_deg),
        'overshoot_pct': max(0, overshoot_pct),
        'ss_error': ss_error,
        'ss_std': ss_std,
        't_rel': t_rel,
        'ang': ang,
        'target': target_val
    }

def main(logfile):
    ts, target, angle, error, vel, vq = load_log(logfile)
    print(f"[INFO] {len(ts)} samples, {(ts[-1]-ts[0])/1000:.1f}s duration")
    print(f"[INFO] Target range: {target.min():.1f} ~ {target.max():.1f} deg")
    print(f"[INFO] Angle range:  {angle.min():.1f} ~ {angle.max():.1f} deg")
    
    # Find step events
    steps = find_step_events(ts, target)
    print(f"[INFO] {len(steps)} step events detected")
    
    if not steps:
        print("[WARN] No step events found. Plotting raw data...")
        fig, axes = plt.subplots(3, 1, figsize=(12, 8), sharex=True)
        t_s = (ts - ts[0]) / 1000.0
        axes[0].plot(t_s, target, 'r--', linewidth=1, label='Target')
        axes[0].plot(t_s, angle, 'b-', linewidth=0.5, label='Angle')
        axes[0].set_ylabel('Angle (deg)')
        axes[0].legend()
        axes[1].plot(t_s, error, 'g-', linewidth=0.5)
        axes[1].set_ylabel('Error (deg)')
        axes[2].plot(t_s, vq, 'm-', linewidth=0.5)
        axes[2].set_ylabel('Vq (V)')
        axes[2].set_xlabel('Time (s)')
        fig.suptitle('Position Control - Raw Data', fontweight='bold')
        fig.tight_layout()
        outdir = os.path.join(os.path.dirname(logfile) or ".", "plots")
        os.makedirs(outdir, exist_ok=True)
        fig.savefig(os.path.join(outdir, 'pos_ctrl_raw.png'), dpi=150)
        print(f"[PLOT] {outdir}/pos_ctrl_raw.png")
        plt.close('all')
        return
    
    # Analyze each step
    results = []
    for i, step in enumerate(steps):
        r = analyze_step(ts, angle, step['to'], step['idx'], step['end_idx'])
        if r:
            results.append(r)
            st = f"{r['settling_time']:.3f}s" if r['settling_time'] else "N/A"
            print(f"  Step {i}: {step['from']:.0f} -> {step['to']:.0f} deg "
                  f"| Ts={st} OS={r['overshoot_pct']:.1f}% "
                  f"SSE={r['ss_error']:+.2f} deg")
    
    # --- Plot: Full timeline ---
    outdir = os.path.join(os.path.dirname(logfile) or ".", "plots")
    os.makedirs(outdir, exist_ok=True)
    
    fig, axes = plt.subplots(3, 1, figsize=(14, 8), sharex=True)
    t_s = (ts - ts[0]) / 1000.0
    
    axes[0].plot(t_s, target, 'r--', linewidth=1.2, label='Target', zorder=5)
    axes[0].plot(t_s, angle, 'b-', linewidth=0.6, label='Angle', alpha=0.8)
    axes[0].set_ylabel('Angle (deg)')
    axes[0].legend(loc='upper right')
    axes[0].set_title('Position Control Step Response', fontweight='bold')
    
    axes[1].plot(t_s, error, color='#2E7D32', linewidth=0.5)
    axes[1].axhline(0, color='gray', linewidth=0.5)
    axes[1].set_ylabel('Error (deg)')
    
    axes[2].plot(t_s, vq, color='#7B1FA2', linewidth=0.5)
    axes[2].axhline(0, color='gray', linewidth=0.5)
    axes[2].set_ylabel('Vq (V)')
    axes[2].set_xlabel('Time (s)')
    
    fig.tight_layout()
    fig.savefig(os.path.join(outdir, 'pos_ctrl_timeline.png'), dpi=150)
    print(f"\n[PLOT] {outdir}/pos_ctrl_timeline.png")
    
    # --- Plot: Individual step responses ---
    if results:
        n = len(results)
        cols = min(n, 3)
        rows = (n + cols - 1) // cols
        fig2, axes2 = plt.subplots(rows, cols, figsize=(5*cols, 4*rows), squeeze=False)
        
        for i, r in enumerate(results):
            ax = axes2[i//cols][i%cols]
            ax.plot(r['t_rel'], r['ang'], 'b-', linewidth=1)
            ax.axhline(r['target'], color='r', linestyle='--', linewidth=1)
            # Settling band
            band = max(abs(r['step_size']) * 0.02, 0.5)
            ax.axhline(r['target'] + band, color='orange', linestyle=':', alpha=0.5)
            ax.axhline(r['target'] - band, color='orange', linestyle=':', alpha=0.5)
            if r['settling_time']:
                ax.axvline(r['settling_time'], color='green', linestyle='--', alpha=0.7)
            
            st = f"Ts={r['settling_time']:.2f}s" if r['settling_time'] else "Ts=N/A"
            ax.set_title(f"Step {r['step_size']:+.0f} deg | {st} | OS={r['overshoot_pct']:.1f}%")
            ax.set_xlabel('Time (s)')
            ax.set_ylabel('Angle (deg)')
            ax.grid(True, alpha=0.3)
        
        # Hide empty subplots
        for i in range(n, rows*cols):
            axes2[i//cols][i%cols].set_visible(False)
        
        fig2.suptitle('Individual Step Responses', fontweight='bold')
        fig2.tight_layout()
        fig2.savefig(os.path.join(outdir, 'pos_ctrl_steps.png'), dpi=150)
        print(f"[PLOT] {outdir}/pos_ctrl_steps.png")
    
    plt.close('all')
    
    # --- Summary ---
    if results:
        print(f"\n{'='*50}")
        print(f"  POSITION CONTROL PERFORMANCE SUMMARY")
        print(f"{'='*50}")
        settling_times = [r['settling_time'] for r in results if r['settling_time']]
        overshoots = [r['overshoot_pct'] for r in results]
        ss_errors = [abs(r['ss_error']) for r in results]
        
        if settling_times:
            print(f"  Settling time:  avg={np.mean(settling_times):.3f}s  max={max(settling_times):.3f}s")
        print(f"  Overshoot:      avg={np.mean(overshoots):.1f}%  max={max(overshoots):.1f}%")
        print(f"  SS error:       avg={np.mean(ss_errors):.2f} deg  max={max(ss_errors):.2f} deg")
        print(f"{'='*50}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        log_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "logs")
        logs = sorted([f for f in os.listdir(log_dir) if f.endswith(".log")])
        logfile = os.path.join(log_dir, logs[-1]) if logs else None
    else:
        logfile = sys.argv[1]
    
    if logfile:
        print(f"[AUTO] {logfile}")
        main(logfile)
