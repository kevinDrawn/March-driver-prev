# SWD 디버깅 전체 기록 및 인수인계

**기간:** 2026-05-04 ~ 2026-05-14  
**대상:** Custom 48-Pin BLDC Motor Driver (STM32G431CBU6)  
**프로그래머:** Nucleo-G431RB (ST-Link V3E)

---

## 1. 목표

커스텀 48핀 보드의 STM32G431CBU6에 펌웨어를 업로드하고 LED(PB6) 점멸 및 시리얼 통신을 확인하는 것.

---

## 2. 하드웨어 구성

### 커스텀 보드 (부분 납땜 상태)
- MCU: STM32G431CBU6 (UFQFPN48, 128KB Flash)
- 크리스탈: 16MHz HSE
- LED1: PB6, LED2: 3.3V 전원 표시
- DRV8316: 제거 상태
- 리셋 버튼: 미납땜
- R3 (NRST 풀업): 세션2에서 납땜 추가
- BOOT0(PB8): CAN_RX_BOOT 네트, 세션2에서 GND 연결 시도

### Nucleo-G431RB
- 온보드 MCU: STM32G431RBT6 (LQFP64)
- 패키지 레지스터(0x1FFF7500): `0x00` 확인
- CN2: 미납땜 (팩토리 상태)
- JP6: 장착 상태
- SB9(SWCLK), SB11(SWDIO): 장착 상태 — 온보드 MCU와 CN4가 물리적으로 연결됨

### SWD 배선 (CN4 → 커스텀 보드 J1)
| CN4 핀 | 신호 | J1 핀 |
|--------|------|-------|
| 3 | T_VCC | 3.3V (세션2에서 추가) |
| 4 | SWDIO | J1-4 (PA13) |
| 6 | SWCLK | J1-2 (PA14) |
| 7 | GND | GND |

---

## 3. 디버깅 타임라인

### 세션 1 (2026-05-04 ~ 05-13 전반): 초기 진단

#### 3.1 최초 업로드 시도
- `pio run -e custom_board_bench -t upload` → `Unable to reset target` 실패
- `upload_flags = reset_config none` 추가 → 업로드 성공, `Verified OK`
- **LED1(PB6) 안 깜빡임**
- 결론 당시: "리셋 설정 문제" → 이후 원인이 아니었음이 밝혀짐

#### 3.2 시리얼 통신 시도
- USART3 (PB10/PB11) 외부 배선 → 시리얼 수신 안 됨
- PA2/PA3 (Nucleo VCP 직결)로 변경 → **시리얼 수신 성공**
- 벤치테스트 출력 정상 수신 (LED, SPI, ADC 테스트 결과)
- **오류:** 이 출력을 커스텀 보드에서 나온 것으로 판단했으나, 실제로는 Nucleo 온보드 MCU에서 실행된 것이었음 (PA2/PA3가 VCP에 물리적으로 직결되어 있으므로)

#### 3.3 SPI 0xFFFF 진단 (5월 4일 보고서)
- DRV8316 SPI 레지스터 읽기 → 전부 0xFFFF 반환
- "SDO(PB4) 납땜 불량"으로 결론 → **이 진단은 무효**
- Nucleo MCU의 PB4에는 DRV8316이 연결되어 있지 않으므로 0xFFFF는 당연한 결과
- 이 오진으로 MCU 재납땜 작업이 수행됨
#### 3.4 하드웨어 전원부 미납땜 문제 발견 (Target Voltage 1.07V)
- ST-Link 연결 시 Target Voltage가 1.07V로 측정되는 현상 발생.
- 외부 파워서플라이에서 3.3V를 인가했음에도 ST-Link가 1.07V만 읽는 것은 VDD 전원이 MCU에 공급되지 않고 SWDIO/SWCLK 핀을 통해 기생 전원(Back-powering)이 들어가고 있음을 시사.
- **원인:** 확인 결과 커스텀 보드 MCU의 전원선(VDD/VSS) 패드가 납땜되지 않아 MCU 전원이 0V 상태였음.
- **조치:** MCU 전원핀 재납땜 수행. 이후 ST-Link Target Voltage가 3.3V로 정상 인식됨. (이 사건으로 하드웨어 재작업이 진행됨)

#### 3.5 HSE 클럭 문제 발견
- `variant_NUCLEO_G431RB.h`가 HSE_VALUE를 24MHz로 하드코딩
- `patch_hse.py`로 `#ifndef` 가드 추가하여 16MHz 설정 우선 적용
- **문제:** HSE_VALUE define만 바꾸는 것이고 PLL 분주비는 안 바뀜
- Nucleo의 SystemClock_Config: `PLL_M=/6, N=x85, R=/2` → 24MHz 기준
- 16MHz 입력 시: VCO 입력 2.67MHz → 범위 밖 → PLL 실패 → Error_Handler 무한루프
- HSI16 오버라이드를 추가했으나 LED1은 여전히 안 깜빡임

#### 3.6 Nucleo 보드 1호 고장
- CN4 3번 핀(T_VCC)에 외부 3.3V 연결 시 0.97A 소모
- ST-Link 내부 회로 소손 추정
- 신품 Nucleo-G431RB로 교체

### 세션 2 (2026-05-13 ~ 05-14): SWD 버스 분석

#### 3.7 SWD 버스 이중 타겟 문제 인지
- 신품 Nucleo에서도 동일 현상: 업로드 성공 + LED1 안 깜빡임
- PKG 레지스터 읽기: `0x00` (LQFP64 = Nucleo 온보드 MCU)
- **깨달음:** SWD가 항상 Nucleo 온보드 MCU에만 연결되고 있었음
- SB9/SB11이 CN4와 온보드 MCU를 물리적으로 연결하고 있음

#### 3.8 소프트웨어 SWD 격리 시도 (실패)
- PKG 레지스터 기반 격리 펌웨어 작성: `if (pkg == 0x00)` → PA13/PA14를 GPIO로 전환
- **1차 시도 (PKG=0x02 가정):** PKG 값 오류 → 양쪽 SWD 전부 비활성화
- `connect_assert_srst`로 Nucleo MCU 복구 (mass erase) 성공
- **2차 시도 (PKG=0x00, PULLDOWN):** Nucleo SWD 비활성화 성공, 커스텀 보드 접근 불가 (풀다운이 SWDIO 풀업과 충돌)
- **3차 시도 (PKG=0x00, ANALOG):** Nucleo SWD 비활성화 성공, 커스텀 보드 여전히 접근 불가

#### 3.9 JP6 제거 시도 (실패)
- JP6 제거 → 온보드 MCU 전원 차단
- `init mode failed` → ESD 다이오드가 SWD 버스를 클램핑하는 것으로 추정
- JP6 복구

#### 3.10 T_VCC(CN4-3) 연결
- CN4 3번 핀에 커스텀 보드 3.3V 연결
- Nucleo LDO가 CN4 → 커스텀 보드로 전원 공급 가능 (외부 전원 불필요)
- SWD 동작에 변화 없음

#### 3.11 SWD 직접 GPIO 제어 테스트 (핵심 테스트)
- OpenOCD로 MCU를 halt한 상태에서 GPIOB 레지스터에 직접 기록
- RCC_AHB2ENR에 GPIOB 클럭 활성화 → GPIOB_MODER에 PB6 출력 설정 → GPIOB_BSRR에 PB6 HIGH
- **결과:** Nucleo MCU의 PB6만 반응, 커스텀 보드 PB6 = 0V
- **의미:** 커스텀 보드 MCU는 SWD 명령을 수신하지 못하고 있음. 펌웨어(코드) 문제가 아님.

#### 3.12 NRST 풀업(R3) 납땜
- 회로도에서 NRST → R3(풀업) → 리셋 버튼 회로 확인
- R3 미납땜 상태 → NRST가 내부 약한 풀업(~40kΩ)에만 의존
- R3 납땜 추가
- SWD 동작 변화 없음

#### 3.13 BOOT0(PB8) GND 연결
- 회로도: PB8-BOOT0이 CAN_RX_BOOT 네트에 연결, 미납땜 상태 → 플로팅
- BOOT0를 GND에 연결
- **결과:** SWD 동작이 변화함
  - 이전: `connect_assert_srst` → 정상 연결 (Nucleo MCU에)
  - 이후: `connect_assert_srst` → `examination failed` (새로운 에러)
  - 이후: 일반 연결 → `init mode failed`
- **해석:** 커스텀 보드 MCU가 SWD 버스에 올라와서 두 MCU가 충돌했을 가능성 있음. 단, 확신 없음.

---

## 4. 확인된 사실 (확실한 것)

| # | 사실 | 근거 |
|---|------|------|
| 1 | Nucleo ST-Link 정상 동작 | `STLINK V3J9M3`, Target voltage 3.3V, 프로그래밍 성공 |
| 2 | Nucleo 온보드 MCU PKG = 0x00 | `mdw 0x1FFF7500` → `ffffff00` |
| 3 | 커스텀 보드 전원 정상 | LED2 점등, SWDIO=3.29V |
| 4 | 커스텀 보드 SWDIO 풀업 활성 | J1-4에서 3.29V 측정 |
| 5 | 커스텀 보드 PB6 = 0V | 멀티미터 직접 측정, 코드 미실행 |
| 6 | SWD GPIO 직접 쓰기 → 커스텀 보드 무반응 | OpenOCD halt 후 레지스터 기록, Nucleo PB6만 반응 |
| 7 | 모든 벤치테스트 결과는 Nucleo MCU 기준 | PA2/PA3 VCP 직결, PKG=0x00 |
| 8 | 5월 4일 SPI 0xFFFF 진단은 무효 | Nucleo MCU에는 DRV8316 미연결 |
| 9 | BOOT0 GND 연결 후 SWD 동작 변화 | `examination failed` (이전에는 없던 에러) |
| 10 | SB9/SB11이 온보드 MCU와 CN4를 물리적으로 연결 | Nucleo 회로도 확인 |

---

## 5. 불확실한 것 (추정)

| # | 추정 | 신뢰도 | 비고 |
|---|------|--------|------|
| 1 | "커스텀 보드 MCU가 SWD 버스에 없다" | 중간 | GPIO 직접 쓰기 무반응이 근거이나, 트레이스 단선 가능성과 구분 불가 |
| 2 | "SWD 버스 충돌이 핵심 원인이다" | 중간 | BOOT0 전후 동작 변화가 근거이나, 다른 원인일 수도 있음 |
| 3 | "BOOT0 GND로 MCU가 SWD에 올라왔다" | 낮음 | examination failed가 근거이나, 전기적 간섭일 수도 있음 |
| 4 | "격리 펌웨어로 Nucleo SWD를 비활성화하면 해결된다" | 낮음 | 3회 시도 모두 커스텀 보드 접근 불가 |
| 5 | "PLL 클럭 불일치가 코드 미실행 원인이다" | 해당없음 | SWD 직접 제어도 안 되므로 클럭 문제 이전 단계 |

---

## 6. 시도했으나 실패한 것 전체 목록

| # | 시도 | 결과 | 배운 것 |
|---|------|------|---------|
| 1 | `reset_config none` | 업로드 성공 | Verified OK ≠ 커스텀 보드에 기록됨 |
| 2 | HSI16 SystemClock_Config 오버라이드 | 빌드/업로드 성공 | LED1 여전히 안 깜빡임 — 클럭이 근본 원인 아님 |
| 3 | minimal_test.cpp | 빌드/업로드 성공 | LED1 여전히 안 깜빡임 |
| 4 | USART3(PB10/PB11) 시리얼 | 수신 안 됨 | Nucleo MCU에서 실행 중이었으므로 외부 배선 무관 |
| 5 | PA2/PA3 시리얼 (VCP 직결) | 수신 성공 | Nucleo MCU 확인 — 커스텀 보드가 아님 |
| 6 | JP6 제거 + CN4-3 T_VCC | init mode failed | ESD 클램핑 또는 다른 원인 |
| 7 | PKG 기반 격리 1차 (PKG≠0x02) | 양쪽 SWD 비활성화 | PKG 값 오류 (실제 0x00) |
| 8 | PKG 기반 격리 2차 (PULLDOWN) | Nucleo SWD OFF, 커스텀 접근 불가 | 풀다운이 SWDIO 풀업과 충돌 |
| 9 | PKG 기반 격리 3차 (ANALOG) | Nucleo SWD OFF, 커스텀 접근 불가 | ANALOG 하이Z에서도 안 됨 |
| 10 | T_VCC(CN4-3) 연결 | 변화 없음 | 전압 레벨 매칭 문제 아님 |
| 11 | R3(NRST 풀업) 납땜 | 변화 없음 | NRST 플로팅이 원인 아님 (또는 R3만으로 부족) |
| 12 | BOOT0 GND 연결 | examination failed | 뭔가 변화했으나 해결은 안 됨 |
| 13 | SWD GPIO 직접 레지스터 기록 | Nucleo PB6만 반응 | 커스텀 보드 MCU가 SWD에 없음을 확인 |

---

## 7. 아직 시도하지 않은 것

| # | 방법 | 난이도 | 확실성 |
|---|------|--------|--------|
| 1 | **SB9/SB11 제거** (0Ω 저항 2개 디솔더) | 인두기 30초 | 높음 — 물리적으로 온보드 MCU를 SWD에서 분리 |
| 2 | **독립 ST-Link 동글** 사용 | 4~5만원 구매 | 높음 — 버스에 타겟 하나뿐 |
| 3 | **J1↔MCU 트레이스 통전 테스트** | 멀티미터 삐 모드 | 확인용 — MCU까지 신호가 도달하는지 검증 |
| 4 | BOOT0 GND + 격리 펌웨어 조합 | 소프트웨어 | 낮음 — 이전 격리 시도 3회 실패 |

---

## 8. 코드베이스 현재 상태

| 파일 | 상태 |
|------|------|
| `src/main.cpp` | BENCH_TEST_MODE에 HSI16 SystemClock_Config 오버라이드 포함 |
| `platformio.ini` | `custom_board_bench` 환경: PA2/PA3 시리얼 |
| `patch_hse.py` | HSE_VALUE `#ifndef` 가드 |
| `src/swd_isolator.cpp` | 삭제됨 |
| Nucleo MCU 플래시 | bench test 펌웨어가 들어있는 상태 |

---

## 9. 최종 판단

커스텀 보드 MCU에 코드가 실행되지 않는 원인은 **SWD 프로그래밍이 커스텀 보드 MCU에 도달하지 못하기 때문**임. 이는 SWD GPIO 직접 쓰기 테스트(3.10)에서 확인됨.

원인 후보:
1. SWD 버스에 두 MCU가 병렬 → Nucleo MCU만 응답 (SB9/SB11 때문)
2. J1 헤더 → MCU 사이 PCB 트레이스 단선
3. MCU 납땜 불량 (VDD/VSS 핀 미접촉)
4. MCU 자체 불량

**1번이 가장 가능성 높으나, 2~4번을 배제하려면 SB9/SB11 제거 또는 독립 ST-Link으로 1번을 먼저 제거한 뒤 재테스트 필요.**

이전 보고서(5월 4일 hardware_diagnostic_report.md)의 SPI, ADC, LED 테스트 결과는 전부 Nucleo 온보드 MCU에서 실행된 것이며, 커스텀 보드 하드웨어 검증 자료로서 무효임.
