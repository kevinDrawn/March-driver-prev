#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// =============================================================================
//  FOC Motor Controller — Hardware Configuration
//  Dual board support: Nucleo-G431RB (default) / Custom 48-pin board
//
//  To select the custom board, define USE_CUSTOM_BOARD in platformio.ini
//  build_flags or uncomment the line below:
//
//  #define USE_CUSTOM_BOARD
// =============================================================================

// -----------------------------------------------------------------------------
//  Motor Parameters — Maxon EC-i 460720 (36V, 84.8mNm, 14-pole, direct drive)
//  ALL voltage/current limits are defined HERE. Do not duplicate elsewhere.
// -----------------------------------------------------------------------------
#define MOTOR_POLE_PAIRS      7       // 14 poles = 7 pole pairs

// --- Supply & Voltage Limits ---
#define SUPPLY_VOLTAGE        24.0f   // [V] power supply (below 36V rated)
#define MOTOR_VOLTAGE_LIMIT   12.0f   // [V] closed-loop max voltage
#define OPENLOOP_VOLTAGE_LIMIT 2.0f   // [V] open-loop (no gearbox — low torque needed)

// --- Current Limits ---
#define MOTOR_CURRENT_LIMIT   3.0f    // [A] SimpleFOC internal limit (above 2.47A rated)
#define I_MAX_SAFE            2.5f    // [A] rated continuous = 2.47A

// --- Safety module thresholds (software overcurrent protection) ---
#define SAFETY_MAX_CURRENT_A  5.0f    // [A] 3× consecutive → immediate shutdown
#define SAFETY_STALL_CURRENT_A 4.5f   // [A] sustained for SAFETY_STALL_TIME_MS → shutdown (raised for grav comp tests)
#define SAFETY_STALL_TIME_MS  1000    // [ms] stall detection window
#define SAFETY_WARN_CURRENT_A 3.0f    // [A] periodic warning (no shutdown)

// --- Motor electrical parameters (from Maxon datasheet #460720) ---
#define MOTOR_KT      0.0315f // [Nm/A] torque constant (31.5 mNm/A)
#define MOTOR_KE      0.0315f // [V/(rad/s)] back-EMF constant (= Kt in SI)
#define MOTOR_R_PHASE 0.766f  // [Ω] terminal resistance (line-to-line)
#define MOTOR_L_PHASE 0.644f  // [mH] terminal inductance

// --- FOC Current PID defaults (derived from motor parameters) ---
// Target bandwidth ~1kHz: P = L·ωc, I = R·ωc
#define FOC_CURRENT_P   3.0f    // [V/A] proportional (tested: 1.5 worse ripple, 3.0 optimal)
#define FOC_CURRENT_I   300.0f  // [V/A/s] integral (R×2π×1000 ≈ 4800, start conservatively)
#define FOC_CURRENT_D   0.0f    // [V·s/A] derivative (not needed for current loop)
#define FOC_CURRENT_RAMP 0.0f   // [V/s] output ramp (0 = no ramp limit)
#define FOC_CURRENT_LIM 12.0f   // [V] PID output voltage limit
#define FOC_LPF_TF     0.001f  // [s] current measurement LPF time constant

// --- Velocity PID defaults (outer loop, tested on Maxon EC-i 460720) ---
// Boot defaults — m3 mode overrides with tuned values (see serial_cmd.cpp)
#define VEL_PID_P     0.3f    // [A/(rad/s)] proportional
#define VEL_PID_I     2.0f    // [A/(rad/s)/s] integral
#define VEL_PID_D     0.002f  // [A·s/(rad/s)] derivative
#define VEL_PID_RAMP  10.0f   // [A/s] output ramp
#define VEL_LPF_TF    0.01f   // [s] velocity measurement LPF

// --- Angle/Position PID defaults ---
// Boot defaults — m3 overrides angP to 4.5
#define ANG_PID_P     6.0f    // [(rad/s)/rad] proportional
#define ANG_PID_I     0.0f    // [(rad/s)/rad/s] integral (disabled)
#define ANG_PID_D     0.0f    // [(rad/s)·s/rad] derivative (not used)
#define ANG_PID_LIM   20.0f   // [rad/s] output velocity limit

// --- Gravity Compensation Calibration (empirically measured, r=20mm) ---
// Datasheet Kt (31.5 mNm/A) underestimates actual Iq by 50-100% due to
// cogging, bearing friction, and current sense offset.
// These values were experimentally tuned to achieve <0.05 mm/s drift.
// Condition: Maxon EC-i 460720, pulley r=20mm, foc_current mode.
#define GRAV_CAL_RADIUS_MM  20.0f   // [mm] calibration radius
#define GRAV_CAL_N          4       // number of calibration points
// Calibration data defined in serial_cmd.cpp (GRAV_CAL_TABLE[N][2])
// To recalibrate: use m5, adjust with g+/g- until drift ≈ 0.

// -----------------------------------------------------------------------------
//  PWM Frequency (common)
// -----------------------------------------------------------------------------
#define FOC_PWM_FREQUENCY                                                      \
  50000 // [Hz] 50kHz — optimal for this motor's inductance
        // 32kHz: P_total=0.66W (ripple dominates) → OTS!
        // 50kHz: P_total=0.49W (balanced) → safest

// -----------------------------------------------------------------------------
//  Serial Debug Baud Rate (common)
// -----------------------------------------------------------------------------
#define SERIAL_BAUD 115200

// =============================================================================
//  Board-Specific Configuration
// =============================================================================

#if defined(USE_H723_BOARD)
// Pin assignment from 260717_BLDC_STM_V2.ioc (LQFP144).
#define PIN_PWMA_H PE9
#define PIN_PWMA_L PE8
#define PIN_PWMB_H PE11
#define PIN_PWMB_L PB0
#define PIN_PWMC_H PE13
#define PIN_PWMC_L PB1
#define PIN_INHA PIN_PWMA_H
#define PIN_INLA PIN_PWMA_L
#define PIN_INHB PIN_PWMB_H
#define PIN_INLB PIN_PWMB_L
#define PIN_INHC PIN_PWMC_H
#define PIN_INLC PIN_PWMC_L
#define USE_6PWM 1
#define PIN_SPI1_SCLK PA5
#define PIN_SPI1_MISO PA6
#define PIN_SPI1_MOSI PD7
#define PIN_ENCODER_CS PA4
#define PIN_SPI_DRV_SCLK PB10
#define PIN_SPI_DRV_MISO PB14
#define PIN_SPI_DRV_MOSI PC3_C
#define PIN_DRV_CS PC0
#define PIN_ISENA PF12
#define PIN_ISENB PF14
#define PIN_ISENC PC2_C
#define CSA_GAIN_MV_PER_AMP 150.0f
// 사용자 확인 배선: Micro-B ST-Link VCP를 UART5에 연결한다.
// 모듈 T_VCP_TX(입력) <- PC12(TX), T_VCP_RX(출력) -> PD2(RX).
#define PIN_UART_TX PC12
#define PIN_UART_RX PD2
#define PIN_DRVOFF PE15
#define PIN_LED PG9
// No nFAULT signal assigned in the supplied IOC: use DRV SPI status.
#elif defined(USE_CUSTOM_BOARD)
// ─────────────────────────────────────────────────────────────────────────────
//  Custom 48-Pin Board
//  - 6-PWM driver (MCU controls both high and low FETs)
//  - UART4 for serial communication
//  - Separate SPI buses for encoder and DRV8316
//  - nSLEEP & DRVOFF hardwired (no GPIO control)
// ─────────────────────────────────────────────────────────────────────────────

// PWM — TIM1, 6×PWM Mode
//   Phase A: INHA=PA10(CH3), INLA=PB15(CH3N)
//   Phase B: INHB=PA9 (CH2), INLB=PB14(CH2N)
//   Phase C: INHC=PA8 (CH1), INLC=PB13(CH1N)
#define PIN_PWMA_H PA10
#define PIN_PWMA_L PB15
#define PIN_PWMB_H PA9
#define PIN_PWMB_L PB14
#define PIN_PWMC_H PA8
#define PIN_PWMC_L PB13
// Aliases for uniform main.cpp access
#define PIN_INHA PA10
#define PIN_INHB PA9
#define PIN_INHC PA8
#define PIN_INLA PB15
#define PIN_INLB PB14
#define PIN_INLC PB13

#define USE_6PWM 1

// SPI1 — MAQ473 Angle Sensor
#define PIN_SPI1_SCLK PA5
#define PIN_SPI1_MISO PA6
#define PIN_SPI1_MOSI PA7
#define PIN_MAQ_CS PA4

#define MAQ473_SPI_CLOCK 10000000 // [Hz] 10 MHz
#define MAQ473_SPI_MODE SPI_MODE3

// SPI3 — DRV8316R Configuration
// ★ MUST use _ALT1 pins! Default PB3/PB4/PB5 maps to SPI1(AF5).
//   _ALT1 forces AF6 → SPI3, avoiding conflict with encoder SPI1.
#define PIN_SPI_DRV_SCLK PB3_ALT1
#define PIN_SPI_DRV_MISO PB4_ALT1
#define PIN_SPI_DRV_MOSI PB5_ALT1
#define PIN_DRV_CS PB2

// ADC — Low-Side Current Sensing (3 channels)
#define PIN_ISENA PA0 // Phase A
#define PIN_ISENB PA1 // Phase B
#define PIN_ISENC PA2 // Phase C

#define CSA_GAIN_MV_PER_AMP 150.0f // [mV/A]

// Control GPIO — nSLEEP and DRVOFF are hardwired, only nFAULT is connected
#define PIN_nFAULT PC14
// No PIN_nSLEEP or PIN_DRVOFF for custom board

// Serial — USART3 (PB10/PB11)
#define PIN_UART_TX PB10
#define PIN_UART_RX PB11

#else
// ─────────────────────────────────────────────────────────────────────────────
//  Nucleo-G431RB (Default)
//  - True 3xPWM mode with Active Freewheeling
//  - DRV8316 INL pins driven HIGH by SimpleFOC as enable pins
//  - USART2 via ST-LINK V-COM
//  - nSLEEP & DRVOFF controlled via GPIO
// ─────────────────────────────────────────────────────────────────────────────

// PWM — TIM1, High-Side (SimpleFOC BLDCDriver3PWM)
#define PIN_INHA PA8      // D7 → J4-1  Phase A high-side
#define PIN_INHB PA9      // D8 → J4-5  Phase B high-side
#define PIN_INHC PA10     // D2 → J4-9  Phase C high-side

// Enable / Active Freewheeling Pins (INLx tied HIGH during operation)
#define PIN_INLA PB3      // D3 → J4-3  Phase A low-side enable
#define PIN_INLB PB10     // D6 → J4-7  Phase B low-side enable
#define PIN_INLC PC7      // D9 → J4-11 Phase C low-side enable

// SPI1 — MAQ473 Angle Sensor (Shares SPI bus with DRV8316, independent CS)
#define PIN_SPI1_SCLK PA5 // D13 — shared with DRV8316
#define PIN_SPI1_MISO PA6 // D12 — shared with DRV8316
#define PIN_SPI1_MOSI PA7 // D11 — shared with DRV8316
#define PIN_MAQ_CS PA4    // A2  — ★ independent CS for MAQ473

#define MAQ473_SPI_CLOCK 10000000 // [Hz] 10 MHz
#define MAQ473_SPI_MODE SPI_MODE3

// SPI1 — DRV8316R Configuration (Moved to reliable Arduino Uno headers D10-D13)
#define PIN_SPI_DRV_SCLK PA5 // D13
#define PIN_SPI_DRV_MISO PA6 // D12
#define PIN_SPI_DRV_MOSI PA7 // D11
#define PIN_DRV_CS PB6       // D10

// ADC — Low-Side Current Sensing (2 channels)
#define PIN_ISENA                                                              \
  PB0 // Phase A — ★ Moved from PA0 (broken injected ADC) to PB0 (A3)
#define PIN_ISENB PA1 // Phase B
// PIN_ISENC PA2 conflicts with USART2_TX on Nucleo

#define CSA_GAIN_MV_PER_AMP 150.0f // [mV/A]

// Control / Status GPIO
#define PIN_nFAULT PC4
#define PIN_nSLEEP PB5 // D4
#define PIN_DRVOFF PB4 // D5

#endif // USE_CUSTOM_BOARD

// Encoder bus belongs to the board; protocol belongs to the sensor driver.
// 앱에서 센서 제품명을 몰라도 동일한 SPI 배선을 사용하도록 공통 핀 이름을 제공한다.
#define PIN_ENCODER_SCK PIN_SPI1_SCLK
#define PIN_ENCODER_MISO PIN_SPI1_MISO
#define PIN_ENCODER_MOSI PIN_SPI1_MOSI
#ifndef PIN_ENCODER_CS
#define PIN_ENCODER_CS PIN_MAQ_CS
#endif
#ifdef ENCODER_TLI49012
// 초기 통신 검증용 보수적인 요청 속도. SPI mode/프레임 규격은 센서 클래스에서 지정한다.
#define ENCODER_SPI_CLOCK 4000000UL
#else
#define ENCODER_SPI_CLOCK MAQ473_SPI_CLOCK
#endif

#endif // CONFIG_H
