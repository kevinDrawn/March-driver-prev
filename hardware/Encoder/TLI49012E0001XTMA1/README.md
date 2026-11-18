# TLI49012 엔코더 모듈

대상: TLI49012-E0001 / TLI49012E0001XTMA1. H723는 이 센서를, G431는 기존 MAQ473를 사용한다.
단일 센서이므로 manager/singleton 없이 전역 `sensor` 인스턴스 하나를 사용한다.

## 코드 구조

| 파일 (프로젝트 루트 기준) | 역할 |
|---|---|
| `src/encoder/Encoder.h` | 앱이 사용할 `EncoderSensor` 타입과 단일 인스턴스 선언 |
| `src/encoder/TLI49012Protocol.h` | 레지스터 이름, 32bit 프레임, 송수신 CRC, 상태 검사 |
| `src/encoder/TLI49012Sensor.h`, `.cpp` | SPI, SimpleFOC Sensor 인터페이스, 각도/자기장 읽기, 진단 |
| `src/encoder/AngleCalibration.h` | MCU와 독립적인 128점 LUT 생성·검증·선형 보간 |
| `src/encoder/EncoderCalibration.h`, `.cpp` | 수동 회전 중 SysCal 관찰, LUT 출력 |
| `src/encoder/tli49012_lut.h` | 현재 조립체 전용 저장 LUT; 기본 비활성 |
| `src/encoder/encoder_bench.cpp` | 모터 없는 엔코더 시험 펌웨어 |
| `tools/encoder/build_tli49012_lut.py` | 독립 참조각 CSV에서 MCU 보정 헤더 생성 |

기본 경로는 **센서 내부 CORDIC/SysCal/동적 보상 각도 → 라디안 → 선택적 MCU LUT**다.
`readSample()`은 MCU LUT 적용 전 각도·상태·유효성을 함께 반환한다.
`getSensorAngle()`은 오류 시 -1을 반환하여 SimpleFOC가 갱신을 건너뛰게 한다.
앱은 연속 3회 실패 또는 마지막 정상 샘플 이후 100ms 경과 시 모터를 정지시킨다.
이는 모든 통신/기계적 고장을 검출한다는 보장은 아니다.

`readField()`는 X3/Y3 또는 Xn/Yn, 벡터 크기를 읽는다. `readFieldAngle()`은
`atan2(Y,X)`로 자기장 방향을 계산한다. X/Y는 별도 프레임이어서 동시 snapshot이 아니며
예측·방향·영점·LUT 경로도 동일하지 않다. 고속 제어에는 내부 `angle_pred`를 사용한다.

## 배선 및 SPI

| 센서 핀 | 기능 | H723 |
|---|---|---|
| 5 IFA | CSN | PA4 |
| 6 IFB | SCK | PA5 |
| 7 IFC | MOSI | PD7 |
| 8 IFD | MISO | PA6 |
| 1 IFE | 동기화 입력 | 이번 코드에서 미사용; 미사용 시 GND 연결 조건 확인 |
| 3 VPROG | NVM 프로그래밍 전원 | 이번 코드에서 미사용 |

SPI mode 1, MSB first, 32 clocks/frame, 요청 속도 4MHz. 센서 최대 10MHz이며 실제 SCK는
STM32 분주값에 따른다. CS setup/hold/high에 각각 1µs를 확보한다.
CRC-8 SAE-J1850(poly=0x1D, init=0xFF, xorout=0xFF)를 사용한다.
MOSI CRC는 3byte, MISO CRC는 **MOSI 주소(R/W 제외) + MISO 3byte**로 계산한다.
CRC 오류는 1회 재시도한다. 주요 측정값은 in-frame, 나머지는 next-frame으로 읽는다.
각도 기본 레지스터는 0x0C, X3/Y3는 0x16/0x17, Xn/Yn는 0x12/0x13이다.
환산은 `radians = angle16 * 2π / 65536`이다.
MAG/DIAG/SUP/OT/RST/OFC 또는 LFOK=0인 샘플은 거부한다.
CAL=0은 초기 수렴 중일 수 있으므로 읽기는 허용하되 보정 완료와 구분한다.
초기화는 부팅의 RST/OFC 이력만 acknowledge하며 지속적인 MAG/DIAG를 반복 clear하지 않는다.

사용자는 표면 좌우가 N/S인 지름 방향 2극 자석과 센서가 마주보는 배치라고 확인했다.
End-of-Shaft 후보 구성으로, 실제 1회전에서 각도가 한 번 순회하고 MAG=0인지 확인해야 한다.
앞/뒷면 N/S인 축방향 자화 또는 다극 자석으로 바뀌면 이 전제를 적용할 수 없다.

## 모터 없는 첫 시험

```sh
pio run -e custom_h723_encoder
pio run -e custom_h723_encoder -t upload
pio device monitor -e custom_h723_encoder
```

이 환경은 PWM/ADC/모터를 초기화하지 않고 PE15 DRVOFF를 HIGH로 유지한다.
reset부터 setup 전까지의 레벨은 외부 회로에 따른다.

| 명령 | 동작 |
|---|---|
| `d` | 상태, CAL, CRC 오류, X3/Y3, 자기장 크기, atan2 진단 |
| `r` | 측정·상태 레지스터 조회 |
| `s` | 100Hz 스트리밍 켜기/끄기 |
| `i` | SPI/읽기 상태 재초기화; 센서 하드웨어 reset 아님 |
| `c` | 수동 회전 SysCal 관찰, 아무 키로 취소, 최대 60초 |
| `Lc` | 참조 표본 수집 초기화, MCU LUT 비활성 |
| `L measured_deg,reference_deg` | 독립 참조각과 측정각 쌍 추가 |
| `Lf` | 128점 LUT 생성·검증 후 RAM에 적용 |
| `Lp` | LUT를 C++ 헤더 형식으로 출력 |

스트림: `ts_ms,valid,angle16,sensor_rad,host_corrected_rad,status_hex`.
valid=0 행의 각도는 사용하지 않는다. sensor_rad는 센서 내부 보상 후, MCU LUT 전 값이다.

## 보정 방법

우선 **센서 내부 SysCal**을 사용한다. 기본 quadrature 모드는 회전하면서 스스로 보정한다.
`c` 실행 후 손으로 같은 방향으로 충분히 회전시킨다. 초기 수렴은 보통 3회전이며,
매뉴얼은 보정값 readback 전에 10회전 이상을 권장한다. 코드는 CAL=1과 약 10회전의
관측 이동량을 확인한다. 이동량은 추정치이므로 좁은 범위 왕복/진동으로 대체하면 안 된다.
CAL=1은 보상 활성 상태이며 실제 정확도 시험을 대신하지 않는다.

이번 코드는 **NVM 기록을 하지 않는다**. 내부 SysCal은 기본 설정을 사용하며,
초기값 저장·필터 변경·센서 내부 32점/16점 LUT 프로그래밍은 추가 구현 범위다.
센서 NVM에는 word별 1회 기록과 VPROG 조건이 있다. RAM 보정값의 전원 차단 후 유지를
가정하지 않는다.

선택적으로 **MCU의 128점 LUT**를 사용한다. 센서 내부 LUT와 별개다.
오차 정의는 `measured - reference`이며 측정각으로 인덱싱한 오차를 보간해서 뺀다.
MAQ473의 기존 `sensor_lut_128.h`는 새 센서에서 로드하지 않는다.
참조 엔코더/정밀 회전 지그로 전 범위를 측정하고 정/역방향 표본을 충분히 수집한다.
모터 명령 전기각은 실제 축각과 같다고 보장되지 않아 독립 참조각을 대체하지 못한다.
별도 참조 없이 정확한 비선형 오차표를 얻을 수 있다고 가정하지 않는다.

CSV 열은 `measured_deg,reference_deg`다. measured_deg는 MCU LUT 전 입력인
`angle_pred / 65536 * 360`을 사용한다.

```sh
python tools/encoder/build_tli49012_lut.py measurements.csv reviewed_tli49012_lut.h
```

결과를 검토 후 `src/encoder/tli49012_lut.h`로 복사하면 다음 빌드에서 로드한다.
최소 96/128bin, 최대 연속 빈 구간 4bin, 원형 평균 일관성, 함수 단조성,
보정 크기(<90°)를 검사한다. 기준 영점과 방향부터 맞춰야 한다.
이 검사만으로 잘못된 참조값이나 실측 정확도를 모두 판별할 수는 없다.
자석·공극·장착·보상 모드·방향·영점·필터 변경 시 재보정한다.
SysCal 수렴 후 만든 LUT가 수렴 전에도 동일한 정확도를 낸다고 가정하지 않는다.
진행 중 SysCal 업데이트와 오래된 LUT의 조합도 검증해야 한다.

## 모터 앱과의 연결

`custom_h723`는 TLI49012 내부 각도를 사용하고 저장 MCU LUT는 기본 비활성이다.
초기 통신/각도가 유효하지 않으면 DRVOFF HIGH로 정지한다.
전체 앱에는 부팅 중 `initFOC()` 정렬이 있으므로 모터 없는 시험에는 전용 환경을 사용한다.
전체 앱의 `c`도 이제 자동 LUT sweep 대신 모터를 끄고 수동 SysCal을 관찰한다.
이후 `e`를 막고 전기각 재정렬을 요구한다. `Ea`는 센서/CAL/안전 상태 확인 후
`initFOC()`를 수행하고 다시 disable한다. **Ea에서는 모터가 움직인다.**
단독 엔코더 환경에는 모터 정렬 기능이 없다.

## 근거와 검증

- [데이터시트](infineon_tli49012_e0001_datasheet_en.pdf), Rev 1.00, 2026-06-11:
  §2 pin, §4.1–4.2 측정/보정, §4.5.1 SPI/CRC/timing.
- [사용자 매뉴얼](infineon-tlx49012-usermanual-en.pdf), Rev 1.00, 2026-07-16:
  §3 SysCal/LUT, §5.1 예제, §7.1.1 CRC, §8.1 환산, §8.2 레지스터.
  [Infineon 원본](https://www.infineon.com/assets/row/public/documents/24/44/infineon-tlx49012-usermanual-en.pdf)
- `test/encoder_math.cpp`: 표준/매뉴얼 CRC golden frame, 응답 bit 손상, 주소 불일치,
  상태 비트, wrap, 정/역회전 합성 오차, 불완전/비단조 LUT 거부 테스트 통과.
  합성 LUT 잔차 최대 0.000123rad 이하이며 실센서 정확도 수치가 아니다.
- `python -B test/test_tli49012_lut_tool.py`: 합성 참조각과 오류 거부 테스트 2개 통과.
- PlatformIO `custom_h723`, `custom_h723_encoder`, 기존 G431 `custom_board` 빌드 성공.
  UART5 VCP 전환 후 엔코더 단독 빌드 Flash 54,916 bytes / RAM 4,916 bytes. `git diff --check` 통과.

호스트 C++ 테스트는 Visual Studio Developer Command Prompt에서 실행할 수 있다.

```bat
cl /nologo /std:c++17 /EHsc /Fe:.pio\encoder-tests.exe /Fo:.pio\encoder-tests.obj test\encoder_math.cpp
.pio\encoder-tests.exe
```

실보드 통신·회전·정확도 시험은 아직 하지 않았다. 배선/전압/IFE 레벨,
MAG 상태, 1회전 범위, SysCal 수렴과 실제 장착 후 정확도는 실측 확인해야 한다.
