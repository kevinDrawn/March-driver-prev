# Custom Board 디버깅 최종 포스트모템 (Part 2/2)

## Phase 3: 해결 (5/14)

### 3.1 Nucleo-F767ZI로 프로그래머 교체

**왜 F767ZI인가:**  
F767ZI는 **Nucleo-144** 규격(기판 MB1137)으로, ST-Link와 온보드 MCU 사이에 **CN4 점퍼 2개**가 있다.
이 점퍼를 손으로 뽑으면 ST-Link가 온보드 MCU로부터 완전히 분리되어, CN6 헤더를 통해 **외부 보드 전용 프로그래머**로 동작한다.  
MB1367(G431RB)에는 이 점퍼가 없어서 불가능했던 것이 여기서는 손쉽게 해결된다.

**접속 절차:**
1. F767ZI의 CN4 점퍼 2개를 손으로 뽑는다 (SWDIO, SWCLK 분리)
2. CN6 헤더(6핀)에서 커스텀 보드 J1으로 점퍼선 연결:

| F767ZI CN6 | 신호 | 커스텀 보드 J1 | 설명 |
|------------|------|---------------|------|
| 1번 | VDD_TARGET | J1-1 (3.3V) | **전압 감지용** — 전원 공급 아님! ST-Link가 타겟 전압을 읽는 핀 |
| 2번 | SWCLK | J1-2 (PA14) | SWD 클럭 |
| 3번 | GND | J1-3 (GND) | 공통 그라운드 |
| 4번 | SWDIO | J1-4 (PA13) | SWD 데이터 |
| 5번 | NRST | 미연결 | 커스텀 보드 J1에 NRST 핀 없음 |

3. 커스텀 보드에 3.3V 전원을 별도로 공급 (F767ZI의 3V3 핀에서 점퍼선으로)
4. F767ZI에 USB 연결

**VDD_TARGET 함정:**  
CN6-1에 연결해도 커스텀 보드에 전기가 공급되지 않아 LED2가 안 켜졌다.  
이 핀은 ST-Link가 "타겟이 몇 V로 동작하는가"를 **감지(Sense)**하기 위한 입력 핀이지, 전원 출력 핀이 아니다.

---

### 3.2 첫 업로드 성공 — Device Name으로 MCU 식별

**테스트:** CubeIDE에서 Debug 버튼 클릭 → ST-Link Programmer 로그 확인.

**결과 (핵심 로그):**
```
Device name : STM32G43x/G44x    ← 커스텀 보드의 G431!
Flash size  : 128 KBytes         ← G431CBU6 맞음
Board       : NUCLEO-F767ZI      ← 프로그래머로 쓰는 보드
Download verified successfully
```

**해석:**  
F767ZI 자체 MCU는 STM32F767(Flash 2MB). 그런데 ST-Link가 읽어온 타겟은 **G43x, 128KB** → 이것은 명백히 커스텀 보드의 STM32G431CBU6이다.  
2주 만에 처음으로 커스텀 보드 MCU에 **직접 접속한 순간.**

**교훈:** 업로드 로그의 `Device name`과 `Flash size`를 항상 확인하면 어느 MCU에 접속 중인지 즉시 알 수 있다.

---

### 3.3 HSE 크리스탈 미동작 확인

**테스트 1 (HSE 활성):**  
CubeMX에서 RCC → HSE = Crystal/Ceramic Resonator, PLL Source = HSE, HCLK = 170MHz 설정. 업로드.

**결과:**
```
Download verified successfully
Target is not responding, retrying...  (×19회)
Lost target connection, Exiting...
```

**해석:** 펌웨어는 플래시에 정상 기록됨. 하지만 MCU가 리셋 후 `SystemClock_Config()`에서 HSE 오실레이터 시작을 기다리다가 타임아웃 → `Error_Handler()` 무한루프 → SWD 클럭도 불안정해져 디버거 연결 끊김.

**테스트 2 (HSI 전용):**  
CubeMX에서 HSE = Disable, PLL Source = HSI, HCLK = 16MHz. 업로드.

**결과:** `Download verified successfully` + `Shutting down... Exit.` (깨끗하게 종료)  
전원 재투입 후 **LED1(PB6) 깜빡임 확인!** 🎉

**결론:** 커스텀 보드의 16MHz 크리스탈 또는 부하 캐패시터에 납땜 불량이 있어 HSE가 발진하지 않는다. HSI(내부 16MHz)로 대체하면 정상 동작.

---

### 3.4 CubeIDE Debug 모드 Halt 이슈

**현상:** Debug 버튼으로 업로드 후 LED가 안 깜빡이고 핀이 플로팅(0.49V).

**원인:** CubeIDE Debug 모드는 펌웨어를 다운로드한 후 MCU를 `main()` 함수 첫 줄에서 **일시정지(Halt)** 시킨다. 사용자가 **Resume(F8)** 버튼을 눌러야 코드 실행이 시작된다.

**해결:** Resume 클릭 또는 Run 모드로 업로드 또는 전원 재투입.

---

### 3.5 TP Ia/Ib/Ic 플로팅 — DRV8316 경유 경로

**테스트:** CubeMX에서 PA8을 GPIO_Output으로 설정하고 토글. 멀티미터로 TP Ia 측정.

**결과:** TP Ia = 0.49V 고정. 토글 안 됨.

**원인:** 커스텀 보드의 신호 경로:
```
MCU PA8 (INHC) → DRV8316 입력 핀 → DRV8316 내부 FET → 모터 출력 → 션트 저항 → TP Ia
```
DRV8316이 **제거된 상태**이므로 PA8 → TP Ia 경로가 물리적으로 끊어져 있다.  
PA8은 MCU에서 정상 토글 중이었으나, TP까지 도달할 길이 없었다.

**해결:** PB6(LED1)을 같이 토글 → LED가 깜빡임 확인 → MCU 코드 실행 정상.

---

### 3.6 CPUTAPID 불일치

**현상:** PlatformIO(OpenOCD)에서 업로드 시도 → 에러:
```
UNEXPECTED idcode: 0x2ba01477
Error: expected 1 of 1: 0x0ba04477
```

**해석:**  
- `0x0ba04477`: OpenOCD의 STM32G4 기본 설정에 하드코딩된 TAP ID
- `0x2ba01477`: ST-Link V2(HLA 모드)를 통해 실제로 읽힌 TAP ID
- ST-Link V2와 V3E는 내부적으로 다른 DAP 프로토콜을 사용하여 ID가 다르게 보고됨

**조치:** `platformio.ini`에서 `set CPUTAPID 0x2ba01477` 으로 수정 → 업로드 성공.

---

### 3.7 시리얼 통신 경로 (PB10 → F767ZI PD8)

**문제:** F767ZI의 ST-Link VCP는 F767ZI 자체 MCU의 UART에 연결되어 있어, 커스텀 보드의 시리얼 출력을 볼 수 없다.

**해결:** F767ZI 보드의 Morpho 헤더에서 **PD8** 핀(STLINK VCP의 RX 입력)을 찾아, 커스텀 보드의 **PB10**(UART TX)과 점퍼선 1개로 연결.

```
커스텀 보드 PB10(TX) ──→ F767ZI PD8 (STLINK VCP RX) ──→ USB ──→ PC 시리얼 모니터
```

이 연결로 PlatformIO의 `pio device monitor`에서 벤치테스트 출력을 수신할 수 있게 됨.

---

### 3.8 벤치테스트 결과 해석 (최종)

#### MAQ473 엔코더 SPI (TEST 3)
```
[MAQ473] Init OK — raw angle: 28196
Raw angle = 19156 (0..16383 = 0..360°) ✓
Magnet = OUT OF RANGE
```

- **`Init OK`**: SPI1 버스(PA5/PA6/PA7)와 CS(PA4)를 통해 MAQ473 칩과 정상 통신됨
- **`raw angle = 19156`**: 14비트 각도 값(0~16383). 0이나 0xFFFF가 아닌 실제 값 → SPI 데이터 정상
- **자석 없을 때** 값이 매번 랜덤: 센서가 자기장을 감지하지 못해 노이즈를 읽는 것 (정상)
- **자석 올렸을 때** raw ≈ 6100 ±90: 편차 0.5% 미만으로 안정적 → 센서 동작 정상
- **`OUT OF RANGE`**: 자석~센서 간 에어갭이 넓어서. 모터에 장착하면 해결

#### DRV8316 SPI (TEST 2)
```
addr=0x0  result=0x808    ← IC_Status 레지스터
addr=0x2  result=0x880    ← Control_2 레지스터
PWM mode = 0 → PWM6 ✓
Slew rate = 200V/us ✓
CSA gain = 0.15V/A ✓
OCP mode = AutoRetry ✓
[DRV] SPI PASS ✓
```

- **`0x808` (IC_Status)**: 비트 분해하면 POR=1(Power-On Reset 플래그), 나머지 폴트 없음 → 칩이 방금 켜졌음을 의미, 정상
- **`PWM mode = PWM6`**: 레지스터에 6-PWM 모드를 쓰고 읽어서 일치 확인 → SPI 읽기/쓰기 정상
- **`CSA gain = 0.15V/A`**: 전류 센스 앰프 이득 설정 확인
- **`[DRV] SPI PASS`**: PWM, Slew, CSA, OCP 4개 항목 모두 쓰기→읽기 검증 통과

#### DRV8316 미납땜 시 결과 (참고):
```
result=0xFFFF  (모든 레지스터)
★★ ALL REGISTERS = 0xFF ★★ → DRV8316 is NOT responding
```
칩이 없으면 MISO가 풀업 저항에 의해 항상 HIGH → 0xFFFF. 이것이 정상적인 "칩 없음" 응답.

---

### 3.9 DRV8316 과열 에피소드

**현상:** DRV8316 재납땜 후 12V 인가 → 칩이 즉시 뜨거워짐.

**원인 추정:** QFN 패키지의 솔더 브릿지 (VM↔GND 또는 인접 핀 단락).

**조치:** 즉시 12V 제거, 재납땜 수행.

**재납땜 후:** 과열 없이 정상 동작 확인. 벤치테스트 전체 PASS.

---

### 3.10 LDO 납땜 → 전원 체인 완성

**전원 체인:**
```
12V 입력 → DRV8316 Buck(5V) → LDO → 3.3V → MCU
```

LDO 납땜 후:
- LED2(전원 표시등) 점등 ✅
- LED1(PB6) 깜빡임 ✅ (플래시에 남아있던 벤치테스트 코드 자동 실행)
- **보드 완전 자립 동작** — F767ZI에서 3.3V를 끌어올 필요 없음

---

## 4. 최종 하드웨어 검증 결과

| 항목 | 상태 | 검증 방법 |
|------|------|----------|
| MCU (STM32G431CBU6) | ✅ | LED1 깜빡임, Device name = G43x |
| SWD 프로그래밍 | ✅ | F767ZI CN6→J1, Verified OK |
| 시리얼 USART3 | ✅ | PB10→PD8→VCP, 벤치테스트 출력 수신 |
| SPI1 (MAQ473) | ✅ | Init OK, raw angle 안정, 33us/read |
| SPI3 (DRV8316) | ✅ | 레지스터 R/W 4항목 PASS |
| Buck (5V) | ✅ | BUCK_EN=1, BUCK_SEL=5V |
| LDO (3.3V) | ✅ | LED2 점등, MCU 자립 |
| nFAULT | ✅ | HIGH 유지 2초 |
| HSE 크리스탈 | ❌ | HSE→crash, HSI→정상 |

---

## 5. 현재 시스템 구성

```
F767ZI CN6-1 (VDD_TARGET) → J1-1 (3.3V)
F767ZI CN6-2 (SWCLK)      → J1-2 (PA14)
F767ZI CN6-3 (GND)         → J1-3 (GND)
F767ZI CN6-4 (SWDIO)       → J1-4 (PA13)
커스텀 보드 PB10 (TX)      → F767ZI PD8 (VCP RX)
```

**PlatformIO:** `upload_protocol = stlink`, `CPUTAPID 0x2ba01477`  
**클럭:** HSI 16MHz (PLL 미사용), `SystemClock_Config` 오버라이드 in main.cpp  
**업로드:** `pio run -e custom_board_bench -t upload`  
**모니터:** `pio device monitor -e custom_board_bench`

---

## 6. 미해결 사항

| # | 항목 | 우선순위 | 비고 |
|---|------|---------|------|
| 1 | HSE 크리스탈 미동작 | 중 | 크리스탈/캡 납땜 점검. HSI로 당분간 운용 가능 |
| 2 | ADC idle 전압 낮음 | 하 | 0.88~1.03V (기대 1.65V). 완비 상태 재측정 필요 |
| 3 | HSI→HSE+PLL 170MHz | 하 | FOC 성능 최적화 시 필요 |

## 7. 주의사항

1. **Nucleo-G431RB(MB1367)를 외부 프로그래머로 쓰지 말 것** — SB9/SB11 하드와이어링
2. **업로드 후 Device name 확인** — `STM32G43x/128KB` = 커스텀 보드
3. **BOOT0(PB8)은 반드시 GND에 납땜** — 플로팅 시 부트로더 모드 진입
4. **CN6-1(VDD_TARGET)은 전원이 아닌 감지 핀** — 별도 전원 필요
5. **CubeIDE Debug 모드는 자동 Halt** — Resume(F8) 또는 Run 모드 사용
