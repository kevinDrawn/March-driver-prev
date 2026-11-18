# 내장 ST-Link를 통한 업로드와 UART

하드웨어 자료: [STLINK-V3MODS Data brief](DB_stlink-v3mods.pdf).
사용자 확인 배선에 맞춘 **Micro-B STLINK-V3MODS** 연결 설정과 사용 절차다.
모듈 SWDIO/SWCLK는 H723 PA13/PA14에 연결되며, VCP는 UART5를 사용한다.
UART5 핀은 제공된 CubeMX 설정의 PC12 TX / PD2 RX를 따른다.

## 연결 구조

```text
PC USB → Micro-B 연결
    └─ STLINK-V3MODS
         ├─ SWDIO/SWCLK/NRST → STM32H723: Flash 업로드, 디버그
         └─ VCP UART        ↔ H723 UART5(PC12 TX / PD2 RX): 로그, 명령, 엔코더 데이터
```

Micro-B 케이블 하나로 연결하고 내부 프로그래밍에는 확인된 ST-Link/SWD 배선을 사용한다.
H723 펌웨어에 USB CDC/DFU 스택을 추가하거나 USB 핀을 할당하는 구조는 아니다.
UART 데이터를 USB COM으로 바꾸는 동작은 ST-Link 쪽 펌웨어가 담당한다.

## 현재 프로젝트 설정

| 항목 | 설정 |
|---|---|
| 일반 앱 | `custom_h723` |
| 모터 없는 엔코더 시험 | `custom_h723_encoder` |
| 업로드 / 디버그 도구 | `stlink` / `stlink` |
| SWD 속도 | `debug_speed = 4000` (kHz) |
| 현재 콘솔 | UART5, PC12 TX / PD2 RX, 115200 8N1 |
| 로그 | `send_on_enter, log2file` |
| PC별 COM 설정 | 프로젝트 루트의 `platformio.local*.ini`로 override |

`platformio.ini`의 `PIN_SERIAL_TX/RX`와 `src/config.h`의 `PIN_UART_TX/RX`를
UART5의 PC12/PD2로 맞췄다. UART5 커널 클럭은 기존 H723 클럭 설정에 포함되어 있다.
`SERIAL_UART_INSTANCE=5`도 명시하여 Arduino의 `Serial`이 UART5 인스턴스를 선택한다.
`Serial.begin(SERIAL_BAUD)`를 사용하는 일반 앱과 엔코더 단독 앱에 같은 설정이 적용된다.

## STLINK-V3MODS 신호 구분

아래는 ST 공식 UM2502 Rev 3의 **V3MODS 32pin edge connector** 기준이다.
SWDIO/SWCLK와 UART5 사용은 사용자 확인 사항이며, 아래 나머지 핀은 상세 배선 확인용이다.

| 모듈 핀 | 신호 / 방향 | 연결 대상 |
|---|---|---|
| 4 | T_JTMS/T_SWDIO, 양방향 | H723 PA13 SWDIO |
| 13 | T_JCLK/T_SWCLK, 출력 | H723 PA14 SWCLK |
| 31 | T_NRST, 출력 | H723 전용 NRST |
| 30 | T_VCC, 입력 | 타깃 I/O 전압 감지, 3.3V |
| GND 핀 | 공통 접지 | H723 GND |
| 12 | T_VCP_TX, **모듈 입력** | H723 PC12, UART5 TX |
| 15 | T_VCP_RX, **모듈 출력** | H723 PD2, UART5 RX |

`T_VCP_TX/RX` 이름은 타깃 방향 기준이라 일반 USB-UART의 TX/RX 이름만 보고
교차 연결한다고 판단하면 안 된다. 데이터 방향을 기준으로 연결한다.
자료의 지원 타깃 I/O 전압은 3.0–3.6V이며 T_VCC 감지 입력을 전원 출력으로 취급하지 않는다.

CubeMX에 있는 PB2/PC10/PC11의 STLINK_MOSI/SCK/MISO는 별도의 USB-SPI bridge용
라벨이다. SWD 업로드나 VCP UART에 필요한 핀이 아니므로 SPI3를 자동 초기화하지 않는다.
PD1의 STLINK_NRST GPIO 라벨도 H723의 전용 NRST 핀과 구분해야 한다.

## 사용 방법

1. 보드의 디버거용 USB를 연결하고 `pio device list` 또는 Windows 장치 관리자에서
   **STLink Virtual COM Port**의 실제 COM 번호를 확인한다.
2. [로컬 설정 예시](platformio.local.ini.example)를 프로젝트 루트에
   `platformio.local.ini`로 복사하고 COM12를 실제 포트로 바꾼다.
   이 로컬 파일은 Git에서 제외된다. 예시 파일 자체는 자동 로드되지 않는다.
3. 처음에는 모터 없는 엔코더 환경으로 업로드하고 로그/명령 왕복을 확인한다.

```sh
pio run -e custom_h723_encoder -t upload
pio device monitor -e custom_h723_encoder
```

단독 시험에서 `d`/`s`로 진단/스트리밍이 동작하면 UART 양방향 통신을 확인할 수 있다.
포트를 명령에서 직접 지정할 수도 있다.

```sh
pio device monitor -e custom_h723_encoder --port COM12
```

일반 모터 앱은 다음 명령을 사용한다. 일반 앱에는 부팅 시 FOC 정렬 동작이 있다.

```sh
pio run -e custom_h723 -t upload
pio device monitor -e custom_h723
```

VS Code PlatformIO 디버거에서는 `custom_h723` 또는 `custom_h723_encoder` 환경을
선택한다. COM 포트는 시리얼 모니터용이며 Flash 업로드의 포트로 지정할 필요가 없다.
`log2file` 필터가 콘솔 로그를 파일로 남기므로 출력되는 저장 경로를 확인한다.

ST-Link USB 인식이 안 되면 ST의 STSW-LINK009 드라이버와 STSW-LINK007 업데이트
도구를 확인한다. 이 작업은 타깃 H723 앱 업로드와 별개이며 이번 변경에서 실행하지 않았다.

## 확인 상태와 근거

- 업로드/디버그 및 모니터 설정의 상속 확인, 두 H723 환경의 빌드에 성공했다.
- 사용자 확인: Micro-B 커넥터, PA13/PA14 SWD 연결, UART5 VCP 사용.
- UART5 전환 후 두 H723 환경의 빌드 성공 및 각 ELF의 `Serial5` 인스턴스를 확인했다.
- 현재 PC 장치 조회에는 COM1만 보였으며 ST-Link VCP를 확인하지 못했다.
- 실제 Flash 업로드, 중단점, UART 송수신과 보드 전원/USB 배선은 아직 검증하지 않았다.
- 상세 핀 방향: [ST 공식 UM2502, §6.1.2 Table 3](https://www.st.com/resource/en/user_manual/um2502-stlinkv3mods-and-stlinkv3mini-mini-debuggersprogrammers-for-stm32-stmicroelectronics.pdf).
