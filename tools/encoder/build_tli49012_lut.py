"""Build a HOST angle-error LUT from independent reference measurements.

CSV columns: measured_deg,reference_deg. measured_deg must be the uncorrected
HOST input (angle_pred / 65536 * 360), with the intended sensor calibration active.
Does not program the sensor or operate a motor. Uses Python standard library only.
"""
import argparse
import csv
import math
from pathlib import Path

SIZE = 128
TAU = 2 * math.pi


def delta(a, b):
    # 원형 최단 오차(rad). 0/360도 경계를 통과하는 표본도 연속적으로 처리한다.
    return (a - b + math.pi) % TAU - math.pi


def build(pairs):
    # MCU의 LutBuilder와 같은 기준: 참조각이 아닌 측정각을 가장 가까운 노드로 모은다.
    sine, cosine, counts = [0.0] * SIZE, [0.0] * SIZE, [0] * SIZE
    for measured, reference in pairs:
        if not math.isfinite(measured) or not math.isfinite(reference):
            raise ValueError("Non-finite angle")
        slot = int((measured % TAU) * SIZE / TAU + 0.5) % SIZE
        error = delta(measured, reference)
        # 산술평균 대신 원형 평균을 사용하기 위해 sin/cos를 누적한다.
        sine[slot] += math.sin(error)
        cosine[slot] += math.cos(error)
        counts[slot] += 1
    coverage = sum(n > 0 for n in counts)
    # 일부 구간만 측정한 데이터로 한 바퀴 전체를 추정하지 않도록 최소 점유율을 요구한다.
    if coverage < 3 * SIZE // 4:
        raise ValueError(f"Incomplete revolution: {coverage}/{SIZE} bins populated")
    values = [math.atan2(s, c) if n else None
              for s, c, n in zip(sine, cosine, counts)]
    for i, count in enumerate(counts):
        if count:
            if math.hypot(sine[i], cosine[i]) / count < 0.98:
                raise ValueError(f"Inconsistent reference samples at bin {i}")
            continue
        left = right = 1
        # 실제 표본이 있는 좌우 노드를 찾아 짧은 빈 구간만 거리 비례로 메운다.
        while not counts[(i - left) % SIZE]:
            left += 1
        while not counts[(i + right) % SIZE]:
            right += 1
        if left + right - 1 > 4:
            raise ValueError("Gap in reference measurements exceeds 4 bins")
        a, b = values[(i - left) % SIZE], values[(i + right) % SIZE]
        values[i] = a + delta(b, a) * left / (left + right)
    step = TAU / SIZE
    # 영점/방향 불일치로 보이는 큰 오차와, 보정 후 역행/평탄해지는 구간을 거부한다.
    for i, value in enumerate(values):
        if abs(value) >= math.pi / 2:
            raise ValueError("Correction >=90 degrees: align the reference zero/direction first")
        if step - delta(values[(i + 1) % SIZE], value) <= step * 0.05:
            raise ValueError(f"Non-monotonic correction at bin {i}")
    return values, coverage


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("csv", type=Path)
    parser.add_argument("output", type=Path, help="Header destination; choose a temporary path to review first")
    args = parser.parse_args()
    with args.csv.open(newline="", encoding="utf-8-sig") as source:
        # CSV의 degree 입력을 rad로 통일한다. utf-8-sig는 Excel 등의 BOM도 허용한다.
        pairs = [(math.radians(float(row["measured_deg"])),
                  math.radians(float(row["reference_deg"]))) for row in csv.DictReader(source)]
    values, coverage = build(pairs)
    # 품질 검사가 끝난 표만 MCU가 include할 수 있는 헤더로 출력한다. 장치를 직접 쓰지 않는다.
    lines = ["#pragma once", "// TLI49012 host LUT, radians; measured - reference.",
             "constexpr bool TLI49012_LUT_VALID=true;", "constexpr float TLI49012_LUT[128]={"]
    for first in range(0, SIZE, 8):
        lines.append("  " + ", ".join(f"{v:.9f}f" for v in values[first:first + 8]) + ",")
    lines.append("};")
    args.output.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"Validated {len(pairs)} pairs, {coverage}/{SIZE} bins. Wrote {args.output}")


if __name__ == "__main__":
    main()
