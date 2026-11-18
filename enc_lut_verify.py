"""
enc_lut_verify.py -- Apply LUT in post-processing and compare raw vs corrected
Uses the same 16-step analysis to show before/after LUT correction.
"""
import csv, sys, os
import numpy as np
import matplotlib
import matplotlib.pyplot as plt
from scipy.interpolate import CubicSpline

matplotlib.rcParams['font.family'] = 'Malgun Gothic'
matplotlib.rcParams['axes.unicode_minus'] = False

# --- Load LUT from sensor_lut_128.h ---
def load_lut(lut_file):
    values = []
    with open(lut_file, "r") as f:
        for line in f:
            line = line.strip()
            if line.startswith("//") or line.startswith("const") or line == "};":
                continue
            # Parse float values from C array lines
            for token in line.replace("{", "").replace("}", "").replace(";", "").split(","):
                token = token.strip()
                if token:
                    try:
                        values.append(float(token))
                    except ValueError:
                        pass
    return np.array(values)

# --- Apply LUT correction (same logic as firmware) ---
def apply_lut(deg_array, lut_rad):
    LUT_SIZE = len(lut_rad)
    corrected = np.zeros_like(deg_array)
    for i, deg in enumerate(deg_array):
        rad = deg * np.pi / 180.0
        idx_f = rad * LUT_SIZE / (2 * np.pi)
        idx0 = int(idx_f) % LUT_SIZE
        idx1 = (idx0 + 1) % LUT_SIZE
        frac = idx_f - int(idx_f)
        error = lut_rad[idx0] * (1.0 - frac) + lut_rad[idx1] * frac
        corr_rad = rad - error
        if corr_rad < 0: corr_rad += 2 * np.pi
        if corr_rad >= 2 * np.pi: corr_rad -= 2 * np.pi
        corrected[i] = corr_rad * 180.0 / np.pi
    return corrected

# --- Detect plateaus ---
def detect_plateaus(unwrap, min_samples=200, threshold=1.0):
    plateaus = []
    i, window = 0, 20
    while i < len(unwrap) - window:
        local_range = max(unwrap[i:i+window]) - min(unwrap[i:i+window])
        if local_range < threshold:
            start, end = i, i + window
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

# --- Unwrap angle array ---
def unwrap_deg(deg_array):
    result = np.zeros_like(deg_array)
    result[0] = 0.0
    for i in range(1, len(deg_array)):
        delta = deg_array[i] - deg_array[i-1]
        if delta > 180: delta -= 360
        if delta < -180: delta += 360
        result[i] = result[i-1] + delta
    return result

# --- Compute step errors ---
def compute_errors(unwrap, plateaus):
    step_angles = []
    for s, e in plateaus:
        mid = (s + e) // 2
        step_angles.append(np.mean(unwrap[mid:e]))
    base = step_angles[0]
    errors = [(step_angles[i] - base) - i * 22.5 for i in range(len(step_angles))]
    return step_angles, errors

def main(csvfile, lut_file):
    # Load data
    ts, deg_raw = [], []
    with open(csvfile, "r") as f:
        reader = csv.DictReader(f)
        for row in reader:
            try:
                ts.append(int(row["ts_ms"]))
                deg_raw.append(float(row["deg"]))
            except (ValueError, KeyError):
                continue
    ts = np.array(ts)
    deg_raw = np.array(deg_raw)
    
    # Load and apply LUT
    lut_rad = load_lut(lut_file)
    print(f"[INFO] Loaded LUT: {len(lut_rad)} slots")
    deg_corr = apply_lut(deg_raw, lut_rad)
    
    # Unwrap both
    unwrap_raw = unwrap_deg(deg_raw)
    unwrap_corr = unwrap_deg(deg_corr)
    
    # Detect plateaus (use raw unwrap, same timing)
    plateaus = detect_plateaus(unwrap_raw)
    print(f"[INFO] {len(ts)} samples, {len(plateaus)} plateaus")
    
    # Compute errors for both
    _, errors_raw = compute_errors(unwrap_raw, plateaus)
    _, errors_corr = compute_errors(unwrap_corr, plateaus)
    
    steps = list(range(len(errors_raw)))
    
    # Print comparison table
    print(f"\n{'Step':>4}  {'Raw Err':>9}  {'Corr Err':>9}  {'Improvement':>12}")
    print("-" * 42)
    for i in range(len(steps)):
        improvement = abs(errors_raw[i]) - abs(errors_corr[i])
        print(f"{i:4d}  {errors_raw[i]:+9.3f}  {errors_corr[i]:+9.3f}  {improvement:+12.3f}")
    
    print(f"\n=== Summary ===")
    print(f"Raw:       max={max(abs(e) for e in errors_raw):.3f} deg,  mean={np.mean(np.abs(errors_raw)):.3f} deg")
    print(f"Corrected: max={max(abs(e) for e in errors_corr):.3f} deg,  mean={np.mean(np.abs(errors_corr)):.3f} deg")
    print(f"Improvement: max err {max(abs(e) for e in errors_raw):.3f} -> {max(abs(e) for e in errors_corr):.3f} deg")
    
    # --- Plot comparison ---
    outdir = os.path.join(os.path.dirname(csvfile), "plots")
    os.makedirs(outdir, exist_ok=True)
    
    # Fig 1: Side-by-side error bars
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5), sharey=True)
    
    colors_raw = ['#66BB6A' if abs(e) < 1.0 else '#FFA726' if abs(e) < 2.0 else '#EF5350' for e in errors_raw]
    colors_corr = ['#66BB6A' if abs(e) < 1.0 else '#FFA726' if abs(e) < 2.0 else '#EF5350' for e in errors_corr]
    
    ax1.bar(steps, errors_raw, color=colors_raw, edgecolor='white')
    ax1.axhline(0, color='gray', linewidth=0.5)
    ax1.axhline(1.0, color='#FFA726', linewidth=0.8, linestyle='--', alpha=0.5)
    ax1.axhline(-1.0, color='#FFA726', linewidth=0.8, linestyle='--', alpha=0.5)
    ax1.set_title('LUT 보정 전 (Raw)')
    ax1.set_xlabel('Step')
    ax1.set_ylabel('Error (deg)')
    for i, e in enumerate(errors_raw):
        ax1.text(i, e + 0.05, f'{e:+.2f}', ha='center', va='bottom', fontsize=6)
    
    ax2.bar(steps, errors_corr, color=colors_corr, edgecolor='white')
    ax2.axhline(0, color='gray', linewidth=0.5)
    ax2.axhline(1.0, color='#FFA726', linewidth=0.8, linestyle='--', alpha=0.5)
    ax2.axhline(-1.0, color='#FFA726', linewidth=0.8, linestyle='--', alpha=0.5)
    ax2.set_title('LUT 보정 후 (Corrected)')
    ax2.set_xlabel('Step')
    for i, e in enumerate(errors_corr):
        ax2.text(i, e + 0.05, f'{e:+.2f}', ha='center', va='bottom', fontsize=6)
    
    fig.suptitle('MAQ473 LUT 보정 전/후 비교', fontsize=14, fontweight='bold')
    fig.tight_layout()
    fig.savefig(os.path.join(outdir, 'lut_before_after.png'), dpi=150)
    print(f"\n[PLOT] {outdir}/lut_before_after.png")
    
    # Fig 2: Error vs position overlay
    fig2, ax3 = plt.subplots(figsize=(10, 5))
    positions = [i * 22.5 for i in steps]
    ax3.plot(positions, errors_raw, 'o-', color='#EF5350', markersize=6, linewidth=1.5, label='Raw (LUT OFF)')
    ax3.fill_between(positions, errors_raw, alpha=0.15, color='#EF5350')
    ax3.plot(positions, errors_corr, 's-', color='#66BB6A', markersize=6, linewidth=1.5, label='Corrected (LUT ON)')
    ax3.fill_between(positions, errors_corr, alpha=0.15, color='#66BB6A')
    ax3.axhline(0, color='gray', linewidth=0.5)
    ax3.set_xlabel('Expected Angle (deg)')
    ax3.set_ylabel('Error (deg)')
    ax3.set_title('MAQ473 Non-linearity: Raw vs LUT-Corrected')
    ax3.set_xticks(np.arange(0, 361, 45))
    ax3.legend()
    ax3.grid(True, alpha=0.3)
    fig2.tight_layout()
    fig2.savefig(os.path.join(outdir, 'lut_error_overlay.png'), dpi=150)
    print(f"[PLOT] {outdir}/lut_error_overlay.png")
    
    plt.close('all')

if __name__ == "__main__":
    if len(sys.argv) < 2:
        log_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "logs")
        csvs = sorted([f for f in os.listdir(log_dir) 
                       if f.endswith("_clean.csv") and not f.startswith("~")])
        csvfile = os.path.join(log_dir, csvs[-1]) if csvs else None
    else:
        csvfile = sys.argv[1]
    
    lut_file = os.path.join(os.path.dirname(os.path.abspath(__file__)), "src", "encoder", "sensor_lut_128.h")
    
    if csvfile:
        print(f"[AUTO] CSV: {csvfile}")
        print(f"[AUTO] LUT: {lut_file}")
        main(csvfile, lut_file)
    else:
        print("Usage: python enc_lut_verify.py <clean_csv>")
