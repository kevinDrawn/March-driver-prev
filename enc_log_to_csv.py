"""
enc_log_to_csv.py — 엔코더 스트리밍 로그 → 깨끗한 CSV + 엑셀용 차트
사용법: python enc_log_to_csv.py <로그파일>
"""
import sys, re, os

def convert(logfile):
    base = os.path.splitext(logfile)[0]
    csv_out = base + "_clean.csv"

    data_lines = []
    with open(logfile, "r", encoding="utf-8", errors="ignore") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            # 데이터 행: 4~6컬럼 CSV (숫자,숫자,...)
            if re.match(r"^\d+,\d+,[\d.+-]+,[\d.+-]+(,[\d.+-]+){0,2}$", line):
                data_lines.append(line)
            elif line.startswith("ts_ms"):
                # 헤더 행
                data_lines.insert(0, line) if not data_lines or not data_lines[0].startswith("ts_ms") else None

    # 헤더가 없으면 추가
    if not data_lines or not data_lines[0].startswith("ts_ms"):
        data_lines.insert(0, "ts_ms,raw16,angle_rad,ma_rad")

    with open(csv_out, "w", encoding="utf-8") as f:
        for line in data_lines:
            f.write(line + "\n")

    n = len(data_lines) - 1  # 헤더 제외
    print(f"[OK] {n} data rows → {csv_out}")
    print(f"\n=== 엑셀에서 그래프 만드는 법 ===")
    print(f"1. {csv_out} 파일을 엑셀로 열기")
    print(f"   (더블클릭 또는 Excel → 데이터 → 텍스트/CSV에서)")
    print(f"2. 구분 기호: 쉼표(,)")
    print(f"3. A열(ts_ms)과 C열(angle_rad) 또는 D열(ma_rad) 선택")
    print(f"4. 삽입 → 차트 → 분산형(XY Scatter) → '직선이 있는 분산형'")
    print(f"   → 계단파 확인하려면 '계단형 분산형' 선택")
    print(f"5. X축=시간(ms), Y축=각도(rad)")
    return csv_out

if __name__ == "__main__":
    if len(sys.argv) < 2:
        # 기본값: logs/ 폴더에서 가장 최근 .log 파일 자동 찾기
        log_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "logs")
        if os.path.isdir(log_dir):
            logs = sorted([f for f in os.listdir(log_dir) if f.endswith(".log")])
            if logs:
                logfile = os.path.join(log_dir, logs[-1])
                print(f"[AUTO] Using latest log: {logfile}")
            else:
                print("Usage: python enc_log_to_csv.py <logfile>")
                sys.exit(1)
        else:
            print("Usage: python enc_log_to_csv.py <logfile>")
            sys.exit(1)
    else:
        logfile = sys.argv[1]

    convert(logfile)
