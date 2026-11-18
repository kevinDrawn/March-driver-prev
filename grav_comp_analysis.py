"""
grav_comp_analysis.py — Gravity Compensation Experiment Analysis
Usage: python grav_comp_analysis.py <log_file_or_dir>

Analyzes Tg test logs to evaluate:
1. Hold stability (position drift during 30s test)
2. Vq-torque linearity across different weights
3. Kt calibration from measured Vq values
"""
import csv, sys, os, glob
import numpy as np

# ── Experiment configuration ──
PULLEY_RADIUS_MM = 20.0
G = 9.81  # m/s^2

# Expected experiments: (label, mass_g)
EXPERIMENTS = [
    ("G1", 50),
    ("G2", 100),
    ("G3", 200),
    ("G4", 300),
    ("G5", 10),
    ("G6", 150),
]


def torque_mNm(mass_g):
    """Calculate torque in mNm from mass and pulley radius."""
    return mass_g / 1000.0 * G * PULLEY_RADIUS_MM / 1000.0 * 1000.0  # mNm


def load_tg_log(filepath):
    """Load a Tg CSV log file. Returns dict of arrays."""
    ts, vq_set, angle_rel, velocity, vq_actual, angle_abs = [], [], [], [], [], []
    with open(filepath, "r") as f:
        for line in f:
            line = line.strip()
            # Skip non-CSV lines
            if line.startswith("[") or not line or not line[0].isdigit():
                continue
            parts = line.split(",")
            if len(parts) < 5:
                continue
            try:
                ts.append(int(parts[0]))
                vq_set.append(float(parts[1]))
                angle_rel.append(float(parts[2]))
                velocity.append(float(parts[3]))
                vq_actual.append(float(parts[4]))
                if len(parts) > 5:
                    angle_abs.append(float(parts[5]))
            except (ValueError, IndexError):
                continue
    return {
        "ts_ms": np.array(ts),
        "vq_set": np.array(vq_set),
        "angle_rel": np.array(angle_rel),
        "velocity": np.array(velocity),
        "vq_actual": np.array(vq_actual),
        "angle_abs": np.array(angle_abs) if angle_abs else None,
    }


def analyze_hold_stability(data, label=""):
    """Analyze position hold stability during Tg test."""
    angle = data["angle_rel"]
    vel = data["velocity"]
    vq = data["vq_set"]
    ts = data["ts_ms"]

    if len(angle) < 10:
        print(f"  [WARN] Too few samples ({len(angle)})")
        return None

    # Use the settled portion (after first 2 seconds)
    if len(ts) > 0:
        t0 = ts[0]
        mask = ts - t0 > 2000  # After 2 seconds
        if np.sum(mask) < 10:
            mask = np.ones(len(ts), dtype=bool)
    else:
        mask = np.ones(len(angle), dtype=bool)

    settled_angle = angle[mask]
    settled_vel = vel[mask]

    # Statistics
    mean_pos = np.mean(settled_angle)
    std_pos = np.std(settled_angle)
    max_drift = np.max(np.abs(settled_angle - settled_angle[0]))
    ripple_pkpk = np.max(settled_angle) - np.min(settled_angle)
    mean_vel = np.mean(np.abs(settled_vel))
    vq_val = vq[0] if len(vq) > 0 else 0

    result = {
        "label": label,
        "vq": vq_val,
        "samples": len(settled_angle),
        "mean_pos": mean_pos,
        "std_pos": std_pos,
        "max_drift": max_drift,
        "ripple_pkpk": ripple_pkpk,
        "mean_abs_vel": mean_vel,
    }

    print(f"\n{'='*50}")
    print(f"  {label}: Vq = {vq_val:.4f} V")
    print(f"{'='*50}")
    print(f"  Samples (settled):  {len(settled_angle)}")
    print(f"  Mean position:      {mean_pos:.3f}°")
    print(f"  Std deviation:      {std_pos:.3f}°")
    print(f"  Max drift:          {max_drift:.3f}°")
    print(f"  Ripple (pk-pk):     {ripple_pkpk:.3f}°")
    print(f"  Mean |velocity|:    {mean_vel:.4f} rad/s")

    return result


def calibrate_kt(results_list):
    """
    Given a list of (mass_g, measured_vq) pairs,
    calibrate Kt (Nm/V) and evaluate linearity.
    """
    if len(results_list) < 2:
        print("\n[INFO] Need at least 2 data points for Kt calibration.")
        return

    masses = np.array([r[0] for r in results_list])
    vqs = np.array([r[1] for r in results_list])
    torques = masses / 1000.0 * G * PULLEY_RADIUS_MM / 1000.0  # Nm

    # Linear fit: torque = Kt * Vq + offset
    # Ideally offset = 0 (no torque at Vq=0)
    coeffs = np.polyfit(vqs, torques, 1)
    kt_fit = coeffs[0]  # Nm/V
    offset = coeffs[1]  # Nm (should be ~0)

    # Calculate predicted Vq for each mass
    vq_predicted = torques / kt_fit

    # R² (goodness of fit)
    torque_pred = np.polyval(coeffs, vqs)
    ss_res = np.sum((torques - torque_pred) ** 2)
    ss_tot = np.sum((torques - np.mean(torques)) ** 2)
    r_squared = 1 - ss_res / ss_tot if ss_tot > 0 else 0

    print(f"\n{'='*60}")
    print(f"  Kt CALIBRATION & LINEARITY ANALYSIS")
    print(f"{'='*60}")
    print(f"  Fitted Kt:        {kt_fit*1000:.4f} mNm/V")
    print(f"  Offset:           {offset*1000:.4f} mNm (ideal: 0)")
    print(f"  R²:               {r_squared:.6f} (ideal: 1.0)")
    print()

    print(f"  {'Mass':>6}  {'τ (mNm)':>8}  {'Vq_meas':>8}  {'Vq_pred':>8}  {'Δ (mV)':>8}  {'Δ (%)':>6}")
    print(f"  {'-'*6}  {'-'*8}  {'-'*8}  {'-'*8}  {'-'*8}  {'-'*6}")
    for m, vq_m, vq_p, t in zip(masses, vqs, vq_predicted, torques):
        delta_mv = (vq_m - vq_p) * 1000
        delta_pct = (vq_m - vq_p) / vq_p * 100 if vq_p != 0 else 0
        print(f"  {m:5.0f}g  {t*1000:8.2f}  {vq_m:8.4f}  {vq_p:8.4f}  {delta_mv:+8.1f}  {delta_pct:+6.1f}")

    # Max nonlinearity
    max_err_pct = np.max(np.abs((vqs - vq_predicted) / vq_predicted)) * 100
    print(f"\n  Max nonlinearity:  {max_err_pct:.1f}%")
    if r_squared > 0.999:
        print(f"  Verdict:           ✅ EXCELLENT linearity")
    elif r_squared > 0.99:
        print(f"  Verdict:           ✅ Good linearity")
    elif r_squared > 0.95:
        print(f"  Verdict:           ⚠️ Moderate linearity")
    else:
        print(f"  Verdict:           ❌ Poor linearity")

    return kt_fit, offset, r_squared


def main():
    print("=" * 60)
    print("  GRAVITY COMPENSATION EXPERIMENT ANALYSIS")
    print(f"  Pulley radius: {PULLEY_RADIUS_MM} mm")
    print("=" * 60)

    # Print expected torques
    print(f"\n  Expected torques:")
    for label, mass in EXPERIMENTS:
        t = torque_mNm(mass)
        print(f"    {label}: {mass:4d}g → τ = {t:.2f} mNm")

    # If files provided, analyze them
    if len(sys.argv) > 1:
        files = []
        for arg in sys.argv[1:]:
            if os.path.isdir(arg):
                files.extend(sorted(glob.glob(os.path.join(arg, "*.log"))))
                files.extend(sorted(glob.glob(os.path.join(arg, "*.csv"))))
            else:
                files.append(arg)

        results = []
        for f in files:
            print(f"\n--- Loading: {os.path.basename(f)} ---")
            data = load_tg_log(f)
            if len(data["ts_ms"]) > 0:
                r = analyze_hold_stability(data, os.path.basename(f))
                if r:
                    results.append(r)

        if results:
            print(f"\n{'='*60}")
            print(f"  SUMMARY")
            print(f"{'='*60}")
            print(f"  {'File':<30} {'Vq':>8} {'Drift':>8} {'Ripple':>8} {'σ':>8}")
            for r in results:
                print(f"  {r['label']:<30} {r['vq']:8.4f} {r['max_drift']:8.3f}° {r['ripple_pkpk']:8.3f}° {r['std_pos']:8.3f}°")
    else:
        print("\n[INFO] No log files provided. Run with: python grav_comp_analysis.py <logfile(s)>")
        print("[INFO] Or provide experiment results manually below.\n")

        # Interactive: enter mass and measured Vq for Kt calibration
        print("  Enter measured (mass_g, Vq) pairs for Kt calibration.")
        print("  Format: mass_g Vq  (e.g. '50 0.98')")
        print("  Enter 'done' when finished.\n")

        pairs = []
        while True:
            line = input("  > ").strip()
            if line.lower() in ("done", "q", ""):
                break
            try:
                parts = line.split()
                mass_g = float(parts[0])
                vq = float(parts[1])
                pairs.append((mass_g, vq))
                t = torque_mNm(mass_g)
                print(f"    Added: {mass_g}g, τ={t:.2f} mNm, Vq={vq:.4f}V")
            except (ValueError, IndexError):
                print("    [ERROR] Format: mass_g Vq")

        if len(pairs) >= 2:
            calibrate_kt(pairs)


if __name__ == "__main__":
    main()
