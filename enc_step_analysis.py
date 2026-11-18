"""
enc_step_analysis.py — 16-step encoder data analysis
Detects plateaus, computes step sizes, compares to expected 22.5° intervals
"""
import csv, sys, os

def analyze(csvfile):
    ts, deg, unwrap = [], [], []
    with open(csvfile, "r") as f:
        reader = csv.DictReader(f)
        for row in reader:
            try:
                ts.append(int(row["ts_ms"]))
                deg.append(float(row["deg"]))
                unwrap.append(float(row["unwrap_deg"]))
            except (ValueError, KeyError):
                continue

    if not ts:
        print("[ERR] No data found")
        return

    print(f"[INFO] {len(ts)} samples, time range: {ts[0]}-{ts[-1]} ms ({(ts[-1]-ts[0])/1000:.1f}s)")
    print(f"[INFO] unwrap range: {unwrap[0]:.2f}° → {unwrap[-1]:.2f}° (total: {unwrap[-1]-unwrap[0]:.2f}°)")

    # --- Detect plateaus using unwrap_deg ---
    # A plateau = region where unwrap_deg changes < threshold over time
    THRESHOLD = 1.0  # deg change threshold for "moving"
    WINDOW = 20      # samples to look ahead

    plateaus = []  # list of (start_idx, end_idx)
    i = 0
    while i < len(unwrap) - WINDOW:
        # Check if this region is stable
        local_range = max(unwrap[i:i+WINDOW]) - min(unwrap[i:i+WINDOW])
        if local_range < THRESHOLD:
            # Found start of plateau, find its end
            start = i
            end = i + WINDOW
            while end < len(unwrap) - 1:
                ext_range = max(unwrap[end:min(end+5, len(unwrap))]) - min(unwrap[end:min(end+5, len(unwrap))])
                if ext_range > THRESHOLD or abs(unwrap[end] - unwrap[start]) > THRESHOLD:
                    break
                end += 1
            if end - start >= 200:  # minimum 200 samples (~2s) to count as plateau
                plateaus.append((start, end))
            i = end + 1
        else:
            i += 1

    print(f"\n[INFO] Detected {len(plateaus)} plateaus")

    if len(plateaus) < 2:
        print("[WARN] Too few plateaus to analyze steps")
        return

    # --- Compute settled value for each plateau (median of last 50%) ---
    print(f"\n{'Step':>4}  {'Settled(°)':>10}  {'Expected(°)':>11}  {'Error(°)':>9}  {'Duration(ms)':>12}")
    print("-" * 55)

    step_angles = []
    for idx, (s, e) in enumerate(plateaus):
        mid = (s + e) // 2  # use second half for settled value
        settled_vals = unwrap[mid:e]
        settled = sum(settled_vals) / len(settled_vals)
        duration = ts[e-1] - ts[s]
        step_angles.append(settled)

    # Compute errors relative to first plateau
    base = step_angles[0]
    expected_step = 22.5  # 360/16

    for idx, angle in enumerate(step_angles):
        relative = angle - base
        expected = idx * expected_step
        error = relative - expected
        duration = ts[plateaus[idx][1]-1] - ts[plateaus[idx][0]]
        print(f"{idx:4d}  {relative:10.2f}  {expected:11.2f}  {error:+9.3f}  {duration:12d}")

    # --- Summary ---
    errors = [(step_angles[i] - base) - i * expected_step for i in range(len(step_angles))]
    print(f"\n=== Summary ===")
    print(f"Plateaus detected: {len(plateaus)}")
    print(f"Max error: {max(abs(e) for e in errors):+.3f}°")
    print(f"Mean |error|: {sum(abs(e) for e in errors)/len(errors):.3f}°")
    print(f"Total rotation (unwrap): {step_angles[-1] - step_angles[0]:.2f}°")
    print(f"Expected total: {(len(step_angles)-1) * expected_step:.2f}°")
    print(f"Cumulative error: {(step_angles[-1]-step_angles[0]) - (len(step_angles)-1)*expected_step:+.3f}°")

    # Step-to-step uniformity
    if len(step_angles) >= 2:
        step_sizes = [step_angles[i+1] - step_angles[i] for i in range(len(step_angles)-1)]
        print(f"\nStep sizes: min={min(step_sizes):.2f}° max={max(step_sizes):.2f}° avg={sum(step_sizes)/len(step_sizes):.2f}°")
        print(f"Step size σ: {(sum((s - expected_step)**2 for s in step_sizes)/len(step_sizes))**0.5:.3f}°")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        log_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "logs")
        csvs = sorted([f for f in os.listdir(log_dir) if f.endswith("_clean.csv")])
        if csvs:
            csvfile = os.path.join(log_dir, csvs[-1])
            print(f"[AUTO] Using: {csvfile}")
        else:
            print("Usage: python enc_step_analysis.py <clean_csv>")
            sys.exit(1)
    else:
        csvfile = sys.argv[1]

    analyze(csvfile)
