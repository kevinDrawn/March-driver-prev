# STM32H723ZGT6 환경 이식 기록

기준일: 2026-10-20. 기존 Arduino / SimpleFOC 애플리케이션을 유지하면서
G431에서 사용자 PCB의 STM32H723ZGT6(LQFP144)로 기본 빌드 환경을 변경했다.
설정 원본은 `cubemx/260717_BLDC_STM_V2.ioc`, `cubemx/Core`, pinout CSV다.
이후 엔코더는 TLI49012로 변경했다. 아래 MAQ473 내용은 최초 이식 시점 기록이며,
현재 통신·보정·단독 시험은 [엔코더 모듈 문서](../encoder/TLI49012E0001XTMA1/README.md)를 따른다.
CubeMX 파일은 참고 자료이며 `src`와 함께 직접 빌드하지 않는다.
PE15는 회로 확인 후 IOC와 참고 초기화 코드에서도 출력/HIGH 초기값으로 수정했다.
pinout CSV는 최초 export 당시의 기록이므로 PE15 입력 표기는 오래된 정보다.

## 빌드와 업로드

내장 디버거/USB 연결, VCP UART와 PC별 COM 설정은
[ST-Link 연결 문서](../ST_Link/README.md)를 참고한다.

```sh
pio run -e custom_h723
pio run -e custom_h723 -t upload
pio device monitor -e custom_h723
```

`pio run`의 기본 환경도 `custom_h723`이다. 업로드는 ST-Link/SWD를 사용한다.
G431에 사용하던 `CPUTAPID` 강제 지정 및 `patch_hse.py`는 H723에 적용하지 않는다.
기존 `nucleo_g431rb`, `custom_board`, `custom_board_bench` 환경은 명시적으로 선택할 수 있다.
기존 bench 환경의 `setup/loop`는 원래 비어 있는 상태이므로 하드웨어 검증 도구로 사용하지 않는다.

## 변경 파일

| 경로 (프로젝트 루트 기준) | 변경 내용 |
|---|---|
| `platformio.ini` | H723 기본 환경, UART5 VCP 핀, ST-Link, LDO 전원 설정 추가 |
| `boards/custom_h723.json` | H723 MCU/Cortex-M7, generic LQFP144 variant, 1MiB Flash, 주 RAM 320KiB, 64MHz 정의 |
| `src/config.h` | `USE_H723_BOARD` 분기 및 아래 핀맵 추가 |
| `src/mcu/clock_h723.cpp` | CubeMX의 HSI/PLL1/PLL2, LDO, VOS3, 버스 클럭 반영; SPI/UART 커널 클럭 지정 |
| `src/mcu/clock.cpp` | G431 PLL 레지스터 해석을 H723에서 제외 |
| `src/mcu/current_sense_h723.cpp` | ADC1/2/3 각각의 injected 채널을 TIM1 TRGO로 구동하는 전류 측정 구현 |
| `src/mcu/current_sense.cpp`, `.h` | 기존 G431 구현 분리, H723 전류 측정 클래스 선택 |
| `src/main.cpp` | 없는 nFAULT GPIO 초기화 제외, PE15 DRVOFF 출력 및 active-low enable 연동, 초기화 실패 처리, G431 ADC 보정 제외 |
| `src/driver/drv8316_helper.cpp` | SPI2 MISO PB14 풀업, SPI 상태 기반 fault 확인, H7 SPI 레지스터 진단 |
| `src/app/serial_cmd.cpp`, `src/app/telemetry.cpp` | fault 확인을 공통 DRV 함수로 통일 |
| `src/bench/motor_bench.cpp` | ADC1 직접 접근 대신 MCU별 전류 측정 함수 사용 |
| `lib/Arduino-FOC-drivers-1.0.9/src/utilities/stm32pwm/STM32PWMInput.cpp` | H7에서 없는 G4 계열 APB 레지스터 참조를 H7 D2CFGR 분기로 수정 |

링커 스크립트와 startup은 Arduino 코어의 H723 variant 것을 사용한다.
Flash 1MiB, RAM_D1 320KiB 등 메모리 배치는 제공된 CubeMX FLASH 스크립트와 일치한다.
CubeMX의 `main.c`, MSP/interrupt 파일을 중복 컴파일하지 않으므로 Arduino/SimpleFOC와의
`main`, HAL 핸들 및 타이머 초기화 충돌을 피한다. 외부의 SimpleFOC를 중복 설치하지 않고
현재 프로젝트 `lib`에 있는 수정된 라이브러리를 사용한다.

## 적용한 핀맵

| 기능 | H723 핀 / 주변장치 |
|---|---|
| A상 PWM H/L | PE9 / PE8, TIM1 CH1 / CH1N |
| B상 PWM H/L | PE11 / PB0, TIM1 CH2 / CH2N |
| C상 PWM H/L | PE13 / PB1, TIM1 CH3 / CH3N |
| MAQ473 SCK / MISO / MOSI / CS | PA5 / PA6 / PD7 / PA4, SPI1 |
| DRV8316 SCK / MISO / MOSI / CS | PB10 / PB14 / PC3_C / PC0, SPI2 |
| A상 전류 | PF12, ADC1 INP6 |
| B상 전류 | PF14, ADC2 INP6 |
| C상 전류 | PC2_C, ADC3 INP0 |
| 명령/로그 UART | PC12 TX / PD2 RX, UART5 → Micro-B ST-Link VCP, 115200 8N1 |
| DRV_DRVOFF | PE15 push-pull 출력, HIGH=출력 차단 / LOW=동작 허용 |

PC2_C/PC3_C는 일반 PC2/PC3와 구분해야 한다. 코어의 dual-pad 핀 정의를 사용하며
ADC3의 PC2 analog switch는 열어서 전용 아날로그 입력으로 사용한다.

## CubeMX 설정과 런타임의 차이

- SYSCLK/HCLK/APB는 **HSI 64MHz**다. H723 최대 동작 주파수로 자동 변경하지 않았다.
  PLL1/PLL2는 HSI / 32 × 129 / 2 = 129MHz이며 SPI와 ADC 커널 클럭에 사용한다.
- PWM은 기존 애플리케이션의 **50kHz, 상보 6PWM**을 사용한다. CubeMX TIM1의
  초기 period/pulse 값을 그대로 쓰지 않고 SimpleFOC가 설정한다.
- 기존 SimpleFOC STM32 backend는 세 전류 핀에 공통 ADC를 요구한다. 이 PCB는
  ADC가 각각 다르므로 별도 H723 클래스로 세 ADC를 초기화했다. G431의
  JSQR 강제 수정은 적용하지 않는다.
- ADC1/2의 CubeMX regular 16bit/software-trigger 설정을 전류 제어용
  **12bit injected / TIM1 TRGO**로 변경한다. ADC3도 12bit injected를 사용한다.
  전압 환산은 3.3V / 4096, CSA 감도는 기존 150mV/A다. 샘플 시간은
  ADC1/2 8.5 cycles, ADC3 12.5 cycles다. 세 ADC는 같은 trigger를 받지만
  세 레지스터 읽기는 원자적인 동시 snapshot은 아니다.
- MAQ473은 기존 SPI mode 3, 최대 요청 10MHz, DRV8316은 기존 라이브러리의
  SPI mode 1 설정을 유지한다. 실제 SCK는 129MHz 커널 클럭의 분주 가능한 값으로 정해진다.
  CubeMX의 SPI 기본 4bit/mode 0 설정은 이 장치의 통신 코드에 적용하지 않는다.
- IOC의 ENC_CS(PA4)는 입력으로 되어 있으나 SPI master의 소프트웨어 CS이므로
  기존 센서 드라이버가 출력/idle HIGH로 설정한다.
- **nFAULT 핀이 IOC에 없다.** 임의 GPIO를 할당하지 않고 DRV8316 SPI status의
  FAULT 비트를 읽는다. 로그의 `nFAULT`/`nF` 명칭도 H723에서는 이 SPI 상태를 뜻한다.
  기존 GPIO 조회보다 통신 비용과 응답 지연이 있다.
- PE15는 MCU가 DRVOFF를 직접 구동하는 것으로 확인되어 출력으로 변경했다.
  setup 진입 시 HIGH를 먼저 기록한 뒤 출력으로 전환한다. SimpleFOC의 `enable_pin`과
  `enable_active_high=false`를 사용하여 `motor/driver.enable()`은 LOW,
  `motor/driver.disable()`은 HIGH가 된다. 명령, 센서 정렬, bench, MotorID,
  전류/스톨 오류 정지에도 같은 드라이버 경로가 적용된다. MCU reset부터 setup 전까지는
  이 코드가 핀 레벨을 보장하지 않으므로 그 구간은 회로의 pull 저항 상태에 따른다.

## nFAULT 미연결과 SPI 상태 확인

DRV8316R은 IC_Status(0x00)의 FAULT와 Status_1/Status_2를 SPI로 제공한다.
현재 `DRV::isFaulted()`는 라이브러리의 세 status register 조회 결과 중 FAULT를 사용하고,
진단 함수는 상세 원인을 출력한다. CLR_FLT를 통한 오류 해제도 SPI로 가능하다.
이것은 MCU의 오류 상태 조회를 대체하며 GPIO interrupt와 같은 비동기 알림은 아니다.
짧은 자동 복구 fault는 polling 사이에 놓칠 수 있고, SPI 고장 시에는 조회 자체가
신뢰되지 않을 수 있다. 현재 코드는 모든 SPI 통신 단선을 완전하게 진단하지 않는다.
내부 보호는 각 보호 모드의 동작에 따르며 SPI 조회와 별개다.

TI는 nFAULT를 사용하지 않아도 외부 pull-up을 권장한다(내부 test mode 진입 방지).
MCU 미연결과 nFAULT 완전 floating은 구분해야 한다. 외부 전원 pull-up의 경우
전원 인가 시 2.2V 초과 조건도 확인해야 한다.
근거: [TI DRV8316 데이터시트](https://www.ti.com/lit/ds/symlink/drv8316.pdf),
8.4.1, 8.4.2, 8.6.1.

## PWM / ADC 검증 순서

PWM은 TIM1, 전류 측정은 ADC1/2/3이며 ADC는 TIM1 TRGO를 받아 변환한다.
ADC 전용 타이머로 분리할 필요는 없다. 별도 타이머를 쓰더라도 PWM과 동기화해야 한다.
TIM1 update의 실제 발생 위치와 전류가 안정된 low-side 측정 구간을 대조하는 것이 핵심이다.
회전 성공만으로 ADC timing이 검증되지는 않는다. 먼저 DRVOFF HIGH에서 MCU PWM
상보 파형/dead time을 확인하고, 이어 제한된 전류/낮은 출력 조건에서 출력 파형과
전류 측정을 확인한 다음 open-loop 저속 회전, 마지막으로 전류 closed-loop를 검증한다.
필요하면 ADC trigger를 update에서 적절한 compare/TRGO2 이벤트로 조정할 수 있다.

## 아직 애플리케이션에 연결하지 않은 CubeMX 기능

기존 모터 애플리케이션에서 사용하지 않던 아래 기능은 원본 IOC에 보존되어 있으며
런타임 초기화/통신 기능은 이번 빌드에 추가하지 않았다.

| 기능 | IOC 핀 / 상태 |
|---|---|
| UART4 | PA0 TX, PA11 RX; IOC에 보존, VCP 콘솔에는 사용하지 않음 |
| SPI3 (STLINK 라벨) | PC10 SCK, PC11 MISO, PB2 MOSI; PD0 소프트웨어 CS, PD1 EXTI 입력 |
| Ethernet RMII | IOC의 ETH 핀; PHY 드라이버, 네트워크 스택, DMA 메모리 배치 추가 필요 |
| 브레이크 PWM | PC6 TIM3 CH1 |
| 온도 입력 | PF4 ADC3 INP9; IOC에는 핀만 있으며 ADC3 regular 채널 설정이 없음 |
| 기타 GPIO | PC9 ENC_SYNC, PD8 RESET_ETH, PG9 LED |

UART4 RTS PA15는 핀은 지정되어 있지만 IOC의 flow control은 NONE이므로 사용하지 않는다.
Ethernet DMA를 추가할 때는 CubeMX MPU/descriptor 배치와 cache 일관성을 별도로 구성해야 한다.

## 검증 및 실보드 확인

컴파일/링크 검증 결과:

- `pio run -e custom_h723`: PE15 출력 변경 후 성공. Flash 137,712 bytes / RAM 5,860 bytes.
- `pio run -e custom_board`: 성공 (기존 G431 환경 호환성 확인).
- `git diff --check`: 통과.

실보드 업로드와 모터 구동 검증은 수행하지 않았다.
전원을 인가한 뒤 UART5 VCP 로그, SPI 응답, DRVOFF 회로 방향을 먼저 확인하고,
오실로스코프로 6PWM의 상 순서/극성/dead time 및 ADC trigger가 유효한 low-side
샘플 구간에 있는지 확인해야 한다. ADC1/2/3 원시값과 무전류 offset,
전류 극성/스케일도 실제 측정값과 대조해야 한다.

기존 `setup()`은 `initFOC()`에서 센서 정렬을 수행하므로 부팅 중 PWM이 발생할 수 있다.
64MHz에서 기존 제어 주기와 SPI fault polling이 요구 성능을 만족하는지도 측정해야 한다.
