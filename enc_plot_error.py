"""
enc_plot_error.py — Encoder calibration error plots
Generates: step error bar chart, time-domain angle plot, error vs position curve
"""
import csv, sys, os
import matplotlib.pyplot as plt
import matplotlib
import numpy as np

matplotlib.rcParams['font.family'] = 'Malgun Gothic'
matplotlib.rcParams['axes.unicode_minus'] = False

def load_csv(csvfile):
    ts, deg, ma_deg, unwrap = [], [], [], []
    with open(csvfile, "r") as f:
        reader = csv.DictReader(f)
        for row in reader:
            try:
                ts.append(int(row["ts_ms"]))
                deg.append(float(row["deg"]))
                ma_deg.append(float(row["ma_deg"]))
                unwrap.append(float(row["unwrap_deg"]))
            except (ValueError, KeyError):
                continue
    return np.array(ts), np.array(deg), np.array(ma_deg), np.array(unwrap)

def detect_plateaus(unwrap, min_samples=200, threshold=1.0):
    plateaus = []
    i = 0
    window = 20
    while i < len(unwrap) - window:
        local_range = max(unwrap[i:i+window]) - min(unwrap[i:i+window])
        if local_range < threshold:
            start = i
            end = i + window
            while end < len(unwrap) - 1:
                ext = unwrap[end:min(end+5, len(unwrap))]
                if max(ext) - min(ext) > threshold or abs(unwrap[end] - unwrap[start]) > threshold:
                    break
                end += 1
            if end - start >= min_samples:
                plateaus.append((start, end))
            i = end + 1
        else:
            i += 1
    return plateaus

def compute_step_data(unwrap, plateaus):
    step_angles = []
    for s, e in plateaus:
        mid = (s + e) // 2
        settled = np.mean(unwrap[mid:e])
        step_angles.append(settled)
    
    base = step_angles[0]
    steps = list(range(len(step_angles)))
    settled_rel = [a - base for a in step_angles]
    expected = [i * 22.5 for i in steps]
    errors = [settled_rel[i] - expected[i] for i in steps]
    return steps, settled_rel, expected, errors

def plot_all(ts, deg, ma_deg, unwrap, plateaus, steps, settled_rel, expected, errors, outdir):
    os.makedirs(outdir, exist_ok=True)
    t_sec = (ts - ts[0]) / 1000.0

    # --- Fig 1: Time-domain unwrapped angle ---
    fig1, ax1 = plt.subplots(figsize=(12, 5))
    ax1.plot(t_sec, unwrap, color='#4FC3F7', linewidth=0.8, label='MAQ473 (unwrap)')
    
    # Mark plateaus
    for idx, (s, e) in enumerate(plateaus):
        mid = (s + e) // 2
        ax1.axhspan(unwrap[mid]-0.5, unwrap[mid]+0.5, 
                     xmin=(t_sec[s]-t_sec[0])/(t_sec[-1]-t_sec[0]),
                     xmax=(t_sec[e-1]-t_sec[0])/(t_sec[-1]-t_sec[0]),
                     alpha=0.15, color='#66BB6A')
    
    # Expected staircase
    for i in range(len(plateaus)):
        s, e = plateaus[i]
        ax1.hlines(i * 22.5, t_sec[s], t_sec[e-1], colors='#FF7043', 
                   linewidth=1.5, linestyle='--', alpha=0.7)
    
    ax1.set_xlabel('시간 (s)')
    ax1.set_ylabel('누적 각도 (°)')
    ax1.set_title('MAQ473 엔코더 16-Step 캘리브레이션 — 시간축 각도')
    ax1.legend(loc='upper left')
    ax1.grid(True, alpha=0.3)
    fig1.tight_layout()
    fig1.savefig(os.path.join(outdir, 'enc_cal_time_domain.png'), dpi=150)
    print(f"[PLOT] {outdir}/enc_cal_time_domain.png")

    # --- Fig 2: Step error bar chart ---
    fig2, ax2 = plt.subplots(figsize=(10, 5))
    colors = ['#66BB6A' if abs(e) < 1.0 else '#FFA726' if abs(e) < 2.0 else '#EF5350' for e in errors]
    bars = ax2.bar(steps, errors, color=colors, edgecolor='white', linewidth=0.5)
    ax2.axhline(0, color='white', linewidth=0.5)
    ax2.axhline(1.0, color='#FFA726', linewidth=0.8, linestyle='--', alpha=0.5, label='±1°')
    ax2.axhline(-1.0, color='#FFA726', linewidth=0.8, linestyle='--', alpha=0.5)
    ax2.set_xlabel('스텝 번호')
    ax2.set_ylabel('오차 (°)')
    ax2.set_title('MAQ473 각 스텝별 오차 (Expected 22.5° 간격 대비)')
    ax2.set_xticks(steps)
    ax2.legend()
    ax2.grid(True, axis='y', alpha=0.3)
    
    # Add value labels on bars
    for bar, err in zip(bars, errors):
        ax2.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 0.05,
                f'{err:+.2f}', ha='center', va='bottom', fontsize=7)
    
    fig2.tight_layout()
    fig2.savefig(os.path.join(outdir, 'enc_cal_step_error.png'), dpi=150)
    print(f"[PLOT] {outdir}/enc_cal_step_error.png")

    # --- Fig 3: Error vs angular position ---
    fig3, ax3 = plt.subplots(figsize=(10, 5))
    positions = [i * 22.5 for i in steps]
    ax3.plot(positions, errors, 'o-', color='#AB47BC', markersize=6, linewidth=1.5)
    ax3.fill_between(positions, errors, alpha=0.2, color='#AB47BC')
    ax3.axhline(0, color='gray', linewidth=0.5)
    ax3.set_xlabel('기대 각도 (°)')
    ax3.set_ylabel('오차 (°)')
    ax3.set_title('MAQ473 비선형 오차 vs 각도 위치')
    ax3.set_xticks(np.arange(0, 361, 45))
    ax3.grid(True, alpha=0.3)
    fig3.tight_layout()
    fig3.savefig(os.path.join(outdir, 'enc_cal_error_vs_position.png'), dpi=150)
    print(f"[PLOT] {outdir}/enc_cal_error_vs_position.png")

    # --- Fig 4: Step size uniformity ---
    fig4, ax4 = plt.subplots(figsize=(10, 4))
    step_sizes = [settled_rel[i+1] - settled_rel[i] for i in range(len(settled_rel)-1)]
    ax4.bar(range(1, len(step_sizes)+1), step_sizes, color='#26C6DA', edgecolor='white')
    ax4.axhline(22.5, color='#FF7043', linewidth=1.5, linestyle='--', label='이상값 22.5°')
    ax4.set_xlabel('스텝 번호')
    ax4.set_ylabel('스텝 크기 (°)')
    ax4.set_title('스텝 간 이동 크기 균일성')
    ax4.legend()
    ax4.grid(True, axis='y', alpha=0.3)
    ax4.set_xticks(range(1, len(step_sizes)+1))
    
    for i, sz in enumerate(step_sizes):
        ax4.text(i+1, sz + 0.1, f'{sz:.1f}', ha='center', va='bottom', fontsize=7)
    
    fig4.tight_layout()
    fig4.savefig(os.path.join(outdir, 'enc_cal_step_uniformity.png'), dpi=150)
    print(f"[PLOT] {outdir}/enc_cal_step_uniformity.png")

    plt.close('all')

if __name__ == "__main__":
    if len(sys.argv) < 2:
        log_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "logs")
        csvs = sorted([f for f in os.listdir(log_dir) 
                       if f.endswith("_clean.csv") and not f.startswith("~")])
        if csvs:
            csvfile = os.path.join(log_dir, csvs[-1])
            print(f"[AUTO] Using: {csvfile}")
        else:
            print("Usage: python enc_plot_error.py <clean_csv>")
            sys.exit(1)
    else:
        csvfile = sys.argv[1]

    outdir = os.path.join(os.path.dirname(csvfile), "plots")
    
    ts, deg, ma_deg, unwrap = load_csv(csvfile)
    plateaus = detect_plateaus(unwrap)
    steps, settled_rel, expected, errors = compute_step_data(unwrap, plateaus)
    
    print(f"[INFO] {len(ts)} samples, {len(plateaus)} plateaus")
    plot_all(ts, deg, ma_deg, unwrap, plateaus, steps, settled_rel, expected, errors, outdir)
    print(f"\n[OK] All plots saved to {outdir}/")
