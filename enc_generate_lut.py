"""
enc_generate_lut.py — Generate 128-slot LUT from 16-step calibration data
Uses cubic spline interpolation with periodic boundary condition.
Outputs C array ready to paste into firmware.
"""
import csv, sys, os
import numpy as np
from scipy.interpolate import CubicSpline

def load_csv(csvfile):
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
    return np.array(ts), np.array(deg), np.array(unwrap)

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

def generate_lut(csvfile):
    ts, deg, unwrap = load_csv(csvfile)
    plateaus = detect_plateaus(unwrap)
    
    print(f"[INFO] {len(ts)} samples, {len(plateaus)} plateaus detected")
    
    # --- Extract settled absolute positions and errors ---
    step_abs_deg = []   # absolute sensor angle at each plateau
    step_errors = []    # error in degrees
    
    # Get settled value for each plateau (use last 50%)
    for s, e in plateaus:
        mid = (s + e) // 2
        settled_abs = np.mean(deg[mid:e])   # absolute deg (0~360)
        settled_unwrap = np.mean(unwrap[mid:e])
        step_abs_deg.append(settled_abs)
    
    # Compute error: MAQ473 - expected
    # Expected = first_position + step_number * 22.5°
    base_unwrap = np.mean(unwrap[(plateaus[0][0]+plateaus[0][1])//2 : plateaus[0][1]])
    
    for idx, (s, e) in enumerate(plateaus):
        mid = (s + e) // 2
        settled_unwrap = np.mean(unwrap[mid:e])
        expected_unwrap = base_unwrap + idx * 22.5
        error = settled_unwrap - expected_unwrap  # positive = sensor reads ahead
        step_errors.append(error)
    
    print(f"\n{'Step':>4}  {'Abs Deg':>8}  {'Error(°)':>9}")
    print("-" * 30)
    for i in range(len(step_abs_deg)):
        print(f"{i:4d}  {step_abs_deg[i]:8.2f}  {step_errors[i]:+9.3f}")
    
    # --- Prepare for periodic interpolation (PCHIP, no overshoot) ---
    # The error must be periodic: error(0°) = error(360°)
    # We have 16+1 points (step 0 and step 16 are same position)
    # Remove the duplicate last point (step 16 ≈ step 0)
    
    from scipy.interpolate import PchipInterpolator
    
    # Sort by absolute position, exclude last point (duplicate of first)
    sorted_data = sorted(zip(step_abs_deg[:-1], step_errors[:-1]))
    positions = np.array([p for p, _ in sorted_data])
    errors = np.array([e for _, e in sorted_data])
    
    # Add periodic wrap points for smooth interpolation
    positions_ext = np.concatenate([
        positions[-3:] - 360.0,  # wrap last 3 points before 0
        positions,
        positions[:3] + 360.0    # wrap first 3 points after 360
    ])
    errors_ext = np.concatenate([errors[-3:], errors, errors[:3]])
    
    # PCHIP interpolation (monotone, NO overshoot)
    pchip = PchipInterpolator(positions_ext, errors_ext)
    
    # Evaluate at 128 evenly spaced points
    LUT_SIZE = 128
    lut_positions = np.linspace(0, 360.0, LUT_SIZE, endpoint=False)
    lut_errors_deg = pchip(lut_positions)
    
    # Do NOT remove DC offset — the LUT must pass through measured points
    # The absolute offset is handled by initFOC's electrical angle alignment
    
    # Convert to radians for firmware
    lut_errors_rad = lut_errors_deg * np.pi / 180.0
    
    # --- Print statistics ---
    print(f"\n=== LUT Statistics ===")
    print(f"LUT slots: {LUT_SIZE}")
    print(f"Slot spacing: {360.0/LUT_SIZE:.4f}°")
    print(f"Max correction: {np.max(np.abs(lut_errors_deg)):.3f}° ({np.max(np.abs(lut_errors_rad)):.5f} rad)")
    print(f"Mean |correction|: {np.mean(np.abs(lut_errors_deg)):.3f}°")
    print(f"DC offset removed: {np.mean(lut_errors_deg):.6f}° (should be ~0)")
    
    # --- Output C array ---
    print(f"\n// =====================================================")
    print(f"// MAQ473 Non-Linearity LUT -- {LUT_SIZE} slots")
    print(f"// Generated from 16-step calibration data")
    print(f"// Error convention: corrected = raw - lut_error[slot]")
    print(f"// Max correction: {np.max(np.abs(lut_errors_rad)):.5f} rad ({np.max(np.abs(lut_errors_deg)):.2f}°)")
    print(f"// =====================================================")
    print(f"const float SENSOR_LUT[{LUT_SIZE}] = {{")
    for i in range(LUT_SIZE):
        comma = "," if i < LUT_SIZE - 1 else ""
        end = "\n" if (i + 1) % 8 == 0 else ""
        print(f"  {lut_errors_rad[i]:+.6f}{comma}", end=end)
    if LUT_SIZE % 8 != 0:
        print()
    print("};")
    
    # --- Also save to file ---
    outdir = os.path.dirname(csvfile) or "."
    lut_file = os.path.join(outdir, "sensor_lut_128.h")
    with open(lut_file, "w") as f:
        f.write(f"// MAQ473 Non-Linearity LUT -- {LUT_SIZE} slots\n")
        f.write(f"// Generated from 16-step calibration ({os.path.basename(csvfile)})\n")
        f.write(f"// Error convention: corrected = raw - lut_error[slot]\n")
        f.write(f"// Max correction: {np.max(np.abs(lut_errors_rad)):.5f} rad ({np.max(np.abs(lut_errors_deg)):.2f}°)\n\n")
        f.write(f"const float SENSOR_LUT[{LUT_SIZE}] = {{\n")
        for i in range(LUT_SIZE):
            comma = "," if i < LUT_SIZE - 1 else ""
            end = "\n" if (i + 1) % 8 == 0 else ""
            f.write(f"  {lut_errors_rad[i]:+.6f}{comma}" + end)
        if LUT_SIZE % 8 != 0:
            f.write("\n")
        f.write("};\n")
    
    print(f"\n[OK] LUT saved to {lut_file}")
    print(f"[OK] Copy the array into MAQ473Sensor's lut_error[] or #include it.")
    
    return lut_errors_rad

if __name__ == "__main__":
    if len(sys.argv) < 2:
        log_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "logs")
        csvs = sorted([f for f in os.listdir(log_dir)
                       if f.endswith("_clean.csv") and not f.startswith("~")])
        if csvs:
            csvfile = os.path.join(log_dir, csvs[-1])
            print(f"[AUTO] Using: {csvfile}")
        else:
            print("Usage: python enc_generate_lut.py <clean_csv>")
            sys.exit(1)
    else:
        csvfile = sys.argv[1]
    
    generate_lut(csvfile)
