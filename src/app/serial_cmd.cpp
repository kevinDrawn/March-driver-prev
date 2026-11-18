// =============================================================================
//  app/serial_cmd.cpp — Serial Command Processor
//  All serial command parsing and dispatch
// =============================================================================
#include "serial_cmd.h"
#include "telemetry.h"
#include "../config.h"
#include "../driver/drv8316_helper.h"
#include "../encoder/Encoder.h"
#include "../mcu/clock.h"
#include "../mcu/current_sense.h"
#include "../safety/safety.h"
#include "../motor/TrajectoryGen.h"
#include "../motor/MotorID.h"
#include <SimpleFOC.h>

// External references to shared objects (defined in main.cpp)
extern BLDCMotor motor;
extern bool motorRunning;
extern bool logMode;
extern bool showIdIq;
extern int opMode;
extern float manual_target;
extern float target;
extern float ramped_target;
extern float pos_offset;
extern float current_v;

#ifdef USE_CUSTOM_BOARD
extern BLDCDriver6PWM driver;
#else
extern BLDCDriver3PWM driver;
#endif

extern EncoderSensor sensor;
#ifdef ENCODER_TLI49012
extern bool encoderNeedsAlignment;
#endif
extern TrajectoryGen traj;
extern MotorID motorID;

// Mode voltage limits (OPENLOOP_VOLTAGE_LIMIT is in config.h)
#define CLOSEDLOOP_VOLTAGE_LIMIT MOTOR_VOLTAGE_LIMIT

// Gravity compensation calibration table: {mass_g, equilibrium_Iq}
// Empirically measured at r=20mm. See config.h for GRAV_CAL_N, GRAV_CAL_RADIUS_MM.
static const float GRAV_CAL_TABLE[GRAV_CAL_N][2] = {
  {  50.0f,  0.471f },   // tested: drift -0.028 mm/s
  { 100.0f,  1.150f },   // tested: drift -0.023 mm/s
  { 200.0f,  2.500f },   // tested: drift +0.044 mm/s
  { 250.0f,  3.000f },   // tested: drift -0.194 mm/s (Ilim clamped)
};

// Forward declarations of global functions (defined in main.cpp)
extern void mbench_phase1();
extern void mbench_phase2();
extern void mbench_phase3();
extern void mbench_phase4();
extern void mbench_run_all();
extern void runSensorCalibration();

namespace Cmd {

void init() {
  // Nothing extra needed
}

void printHelp() {
  Serial.println("\n=== Commands ===");
  Serial.println(" e/d    : Enable/Disable motor");
  Serial.println(" m0-m4  : Mode (VelOL, AngOL, VelCL, AngCL, Torque)");
  Serial.println(" tX     : Set target (e.g. t10)");
  Serial.println(" uX     : Set Uq voltage in Torque mode (e.g. u2.5)");
  Serial.println(" VX/+/- : Set/adjust voltage limit");
  Serial.println(" L      : Toggle CSV logging");
  Serial.println(" S      : Full status");
  Serial.println(" C      : Clear DRV8316 faults");
  Serial.println(" I      : Toggle Id/Iq display");
  Serial.println(" --- Diagnostics ---");
  Serial.println(" Dd     : DRV8316 full register dump (decoded)");
  Serial.println(" Da     : ADC register dump (decoded)");
  Serial.println(" De     : Encoder register dump (decoded)");
  Serial.println(" Ds     : Safety module status");
  Serial.println(" Dc     : Clock info dump");
  Serial.println(" D*     : ALL dumps");
  Serial.println(" --- Tuning ---");
  Serial.println(" pX/iX  : PID vel P/I");
  Serial.println(" aX     : PID angle P");
  Serial.println(" rX     : Output ramp");
  Serial.println(" fX     : LPF Tf");
  Serial.println(" --- Trajectory ---");
  Serial.println(" wX     : Waveform (0=off, 1=sine, 2=ramp, 3=step)");
  Serial.println(" AX/FX  : Amplitude/Frequency");
  Serial.println(" --- Advanced ---");
#ifdef ENCODER_TLI49012
  Serial.println(" c      : Observe SysCal; motor disabled, rotate by hand");
  Serial.println(" Ea     : Electrical alignment after calibration (motor moves)");
#else
  Serial.println(" c      : Sensor calibration (LUT)");
#endif
  Serial.println(" Es     : Encoder stream (raw + moving avg, 100Hz)");
  Serial.println(" R      : Motor parameter ID");
  Serial.println(" X      : CSA alignment test");
  Serial.println(" B1-B4  : Motor bench (Static/Sweep/OCP/Sustain)");
  Serial.println(" Ba     : Motor bench ALL");
  Serial.println(" Sr     : Reset safety shutdown");
}

void process() {
  if (!Serial.available()) return;

  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  if (cmd.length() == 0) return;

  char c = cmd.charAt(0);
  switch (c) {

  // ── Enable ──
  case 'e':
#ifdef ENCODER_TLI49012
    // 저장된 각도가 아니라 지금 읽은 정상 샘플인지 확인한 뒤 구동을 허용한다.
    if (encoderNeedsAlignment) { Serial.println("[ENC] Run Ea electrical alignment first."); break; }
    if (sensor.getSensorAngle()<0) { Serial.println("[ENC] Invalid sensor; enable rejected."); break; }
#endif
    if (Safety::isShutdown()) {
      Serial.println("[CMD] Safety shutdown active! Use 'Sr' to reset first.");
      break;
    }
#ifndef USE_CUSTOM_BOARD
    digitalWrite(PIN_DRVOFF, LOW);
#endif
    DRV::drv8316.clearFault();
    delay(5);
    DRV::drv8316.clearFault();
    delay(5);
    if (motor.controller == MotionControlType::angle) {
      pos_offset = motor.shaft_angle;
      target = 0; manual_target = 0;
    } else if (motor.controller == MotionControlType::angle_openloop) {
      target = motor.shaft_angle;
      manual_target = motor.shaft_angle;
    }
    motor.enable();
    motorRunning = true;
    current_v = 0.0f;
    ramped_target = 0.0f;
    Serial.print("[CMD] ENABLED. nFAULT=");
    Serial.print((!DRV::isFaulted()) ? "OK" : "FAULT");
    if (motor.controller == MotionControlType::angle) {
      Serial.print(" pos=0 (offset="); Serial.print(pos_offset, 1); Serial.print(")");
    }
    Serial.println();
    break;

  // ── Disable ──
  case 'd':
    motorRunning = false;
    motor.disable();
#ifndef USE_CUSTOM_BOARD
    digitalWrite(PIN_DRVOFF, HIGH);
#endif
    Serial.println("[CMD] DISABLED");
    break;

  // ── Mode select ──
  case 'm':
    if (cmd.length() > 1) {
      opMode = cmd.substring(1).toInt();
      if (opMode == 0) {
        motor.controller = MotionControlType::velocity_openloop;
        motor.torque_controller = TorqueControlType::voltage;
        motor.voltage_limit = OPENLOOP_VOLTAGE_LIMIT;
        target = 0; current_v = 0.0f;
        motor.velocity_limit = 50.0f;
        Serial.print("[CMD] MODE: VelOL Vlim=");
        Serial.println(motor.voltage_limit, 1);
      } else if (opMode == 1) {
        motor.controller = MotionControlType::angle_openloop;
        motor.torque_controller = TorqueControlType::voltage;
        motor.voltage_limit = OPENLOOP_VOLTAGE_LIMIT;
        target = motor.shaft_angle;
        motor.velocity_limit = 3.0f;
        Serial.print("[CMD] MODE: AngOL Vlim=");
        Serial.println(motor.voltage_limit, 1);
      } else if (opMode == 2) {
        motor.controller = MotionControlType::velocity;
        motor.torque_controller = TorqueControlType::foc_current;
        motor.voltage_limit = 12.0f;
        target = 0; manual_target = 0; ramped_target = 0;
        motor.velocity_limit = 120.0f;
        motor.PID_velocity.P = 0.2f;
        motor.PID_velocity.I = 1.0f;
        motor.PID_velocity.D = 0.0f;
        motor.PID_velocity.output_ramp = 10.0f;
        motor.PID_velocity.limit = MOTOR_CURRENT_LIMIT;
        Serial.println("[CMD] MODE: VelCL (foc_current)");
      } else if (opMode == 3) {
        motor.controller = MotionControlType::angle;
        motor.torque_controller = TorqueControlType::foc_current;
        motor.voltage_limit = 12.0f;
        pos_offset = motor.shaft_angle;
        target = 0; manual_target = 0;
        motor.velocity_limit = 40.0f;
        // Tuned PID for EC-i 460720 direct drive (output is now Iq in Amps)
        motor.PID_velocity.P = 0.3f;
        motor.PID_velocity.I = 2.0f;
        motor.PID_velocity.D = 0.008f;
        motor.PID_velocity.output_ramp = 60.0f;
        motor.PID_velocity.limit = MOTOR_CURRENT_LIMIT;
        motor.LPF_velocity.Tf = 3.0f;
        motor.P_angle.P = 4.5f;
        motor.P_angle.I = 0.0f;
        motor.P_angle.D = 0.0f;
        Serial.print("[CMD] MODE: AngCL (foc_current) Ilim=");
        Serial.print(MOTOR_CURRENT_LIMIT, 1);
        Serial.print("A pos=0 (offset=");
        Serial.print(pos_offset, 1); Serial.println(")");
        Serial.println("[CMD] PID: velP=0.3 velI=2.0 velD=0.008 angP=4.5 ramp=60 LPF=3.0");
      } else if (opMode == 4) {
        motor.controller        = MotionControlType::torque;
        motor.torque_controller = TorqueControlType::foc_current;
        motor.voltage_limit     = 12.0f;
        target = 0; manual_target = 0;
        Serial.print("[CMD] MODE: Torque (foc_current) Ilim=");
        Serial.print(MOTOR_CURRENT_LIMIT, 1); Serial.println("A");
      } else if (opMode == 5) {
        // Gravity compensation mode — TRUE FOC current control
        // Usage: m5 or m5 <mass_g> or m5 <mass_g> <radius_mm>
        motor.controller        = MotionControlType::torque;
        motor.torque_controller = TorqueControlType::foc_current;
        motor.voltage_limit     = 12.0f;
        pos_offset = motor.shaft_angle;  // Record starting position

        // Parse: "m5 100" (mass only, default r=20mm) or "m5 100 15" (mass + radius)
        float compIq = 0;
        const float default_r_mm = GRAV_CAL_RADIUS_MM;
        int sp1 = cmd.indexOf(' ');       // space between "m5" and mass
        if (sp1 > 0) {
          float mass_g = 0;
          float radius_mm = default_r_mm;
          int sp2 = cmd.indexOf(' ', sp1 + 1);  // space between mass and radius
          if (sp2 > 0) {
            mass_g = cmd.substring(sp1 + 1, sp2).toFloat();
            radius_mm = cmd.substring(sp2 + 1).toFloat();
          } else {
            mass_g = cmd.substring(sp1 + 1).toFloat();  // mass only
          }

          // Use calibration table (interpolation) if radius matches cal radius
          bool useCal = (fabsf(radius_mm - GRAV_CAL_RADIUS_MM) < 0.1f);
          if (useCal && mass_g > 0) {
            // Linear interpolation in GRAV_CAL_TABLE
            if (mass_g <= GRAV_CAL_TABLE[0][0]) {
              // Below lowest cal point: scale proportionally from first point
              compIq = GRAV_CAL_TABLE[0][1] * (mass_g / GRAV_CAL_TABLE[0][0]);
            } else if (mass_g >= GRAV_CAL_TABLE[GRAV_CAL_N-1][0]) {
              // Above highest cal point: extrapolate from last two points
              float m0 = GRAV_CAL_TABLE[GRAV_CAL_N-2][0], i0 = GRAV_CAL_TABLE[GRAV_CAL_N-2][1];
              float m1 = GRAV_CAL_TABLE[GRAV_CAL_N-1][0], i1 = GRAV_CAL_TABLE[GRAV_CAL_N-1][1];
              compIq = i0 + (i1 - i0) * (mass_g - m0) / (m1 - m0);
            } else {
              // Interpolate between two bracketing points
              for (int k = 0; k < GRAV_CAL_N - 1; k++) {
                if (mass_g >= GRAV_CAL_TABLE[k][0] && mass_g <= GRAV_CAL_TABLE[k+1][0]) {
                  float m0 = GRAV_CAL_TABLE[k][0],   i0 = GRAV_CAL_TABLE[k][1];
                  float m1 = GRAV_CAL_TABLE[k+1][0], i1 = GRAV_CAL_TABLE[k+1][1];
                  float t = (mass_g - m0) / (m1 - m0);
                  compIq = i0 + (i1 - i0) * t;
                  break;
                }
              }
            }
            // Clamp to motor current limit
            if (compIq > MOTOR_CURRENT_LIMIT) compIq = MOTOR_CURRENT_LIMIT;
            Serial.print("[CMD] GravComp(CAL): mass="); Serial.print(mass_g, 0);
            Serial.print("g Iq="); Serial.print(compIq, 3);
            Serial.println("A (interpolated from calibration table)");
          } else {
            // Fallback: theoretical calculation (non-calibrated radius)
            float torque_Nm = (mass_g / 1000.0f) * 9.81f * (radius_mm / 1000.0f);
            compIq = torque_Nm / MOTOR_KT;
            Serial.print("[CMD] GravComp(CALC): mass="); Serial.print(mass_g, 0);
            Serial.print("g r="); Serial.print(radius_mm, 1);
            Serial.print("mm Iq="); Serial.print(compIq, 3);
            Serial.println("A (theoretical Kt — uncalibrated radius)");
          }
        }
        manual_target = compIq;
        target = compIq;
        Serial.println("[CMD] MODE: GravComp (foc_current)");
        Serial.println("[CMD] Adjust: g<Iq> (set), g+/g- (±0.01A)");
        Serial.print("[CMD] Current Iq="); Serial.print(compIq, 3); Serial.println("A");
      }
    }
    break;

  // ── Target ──
  case 't':
    if (cmd.length() > 1) {
      manual_target = cmd.substring(1).toFloat();
      Serial.print("[CMD] TGT="); Serial.println(manual_target);
    }
    break;

  // ── Direct voltage (torque mode) ──
  case 'u':
    if (cmd.length() > 1) {
      if (opMode == 4) {
        manual_target = cmd.substring(1).toFloat();
        Serial.print("[CMD] Uq="); Serial.println(manual_target, 2);
      } else {
        Serial.println("[CMD] u: only valid in m4 (torque mode)");
      }
    }
    break;

  // ── Voltage limit ──
  case 'V':
    if (cmd.length() > 1) {
      motor.voltage_limit = fminf(fmaxf(cmd.substring(1).toFloat(), 0.5f), SUPPLY_VOLTAGE);
      Serial.print("[CMD] Vlim="); Serial.println(motor.voltage_limit);
    }
    break;
  case '+':
    motor.voltage_limit = fminf(motor.voltage_limit + 1.0f, SUPPLY_VOLTAGE);
    Serial.print("[CMD] Vlim="); Serial.println(motor.voltage_limit);
    break;
  case '-':
    motor.voltage_limit = fmaxf(motor.voltage_limit - 1.0f, 0.5f);
    Serial.print("[CMD] Vlim="); Serial.println(motor.voltage_limit);
    break;

  // ── Gravity compensation Iq ──
  case 'g':
    if (opMode != 5) {
      Serial.println("[CMD] g: only valid in m5 (GravComp) mode");
    } else if (cmd.length() == 1) {
      // Just 'g' — print current Iq
      Serial.print("[CMD] GravComp Iq="); Serial.print(manual_target, 4);
      Serial.println("A");
    } else if (cmd.charAt(1) == '+') {
      manual_target += 0.01f;
      Serial.print("[CMD] Iq="); Serial.print(manual_target, 4); Serial.println("A");
    } else if (cmd.charAt(1) == '-') {
      manual_target -= 0.01f;
      Serial.print("[CMD] Iq="); Serial.print(manual_target, 4); Serial.println("A");
    } else {
      manual_target = cmd.substring(1).toFloat();
      Serial.print("[CMD] Iq="); Serial.print(manual_target, 4); Serial.println("A");
    }
    break;

  // ── Trajectory ──
  case 'A':
    if (cmd.length() > 1) {
      traj.amplitude = cmd.substring(1).toFloat();
      Serial.print("[CMD] Traj Amp="); Serial.println(traj.amplitude);
    }
    break;
  case 'F':
    if (cmd.length() > 1) {
      traj.frequency = cmd.substring(1).toFloat();
      Serial.print("[CMD] Traj Freq="); Serial.println(traj.frequency);
    }
    break;
  case 'w':
    if (cmd.length() > 1) {
      int w = cmd.substring(1).toInt();
      if (w == 0) { traj.stop(); }
      else {
        traj.type = (TrajectoryGen::WaveType)w;
        if (traj.frequency == 1.0f) traj.frequency = 0.5f;
        traj.offset = 0.0f;
        traj.start();
        Serial.print("[CMD] AUTO WAVEFORM (Type "); Serial.print(w); Serial.println(")");
      }
    }
    break;

  // ── Auto position test (Tp) ──
  case 'T': {
    if (cmd.length() < 2) { Serial.println("[TEST] Use: Tp (step) or Tr (repeatability)"); break; }

    // Tg has its own mode check (m5), so only check m3 for Tp/Tr
    char sub = cmd.charAt(1);
    if (sub != 'g' && motor.controller != MotionControlType::angle) {
      Serial.println("[TEST] Error: must be in m3 (AngCL) mode first!");
      break;
    }

    // Enable motor if not running
    if (!motorRunning) {
      DRV::drv8316.clearFault(); delay(5);
      pos_offset = motor.shaft_angle;
      target = 0; manual_target = 0;
      motor.enable();
      motorRunning = true;
      Serial.println("[TEST] Motor enabled.");
      delay(500);
    }

    // Start logging
    logMode = true;
    Serial.println("ts_ms,target_deg,angle_deg,error_deg,velocity,Vq");

    // Helper lambda: run one step with logging
    auto runStep = [&](float tgt_rad, unsigned long hold_ms) {
      manual_target = tgt_rad;
      unsigned long stepStart = millis();
      while (millis() - stepStart < hold_ms) {
        motor.loopFOC();
        target = manual_target + pos_offset;
        motor.move(target);
        static unsigned long lastLog = 0;
        unsigned long now_us = micros();
        if (now_us - lastLog >= 5000) {
          lastLog = now_us;
          float a_deg = motor.shaft_angle * (180.0f / PI);
          float t_deg = target * (180.0f / PI);
          Serial.print(millis());       Serial.print(",");
          Serial.print(t_deg, 2);       Serial.print(",");
          Serial.print(a_deg, 2);       Serial.print(",");
          Serial.print(t_deg - a_deg, 3); Serial.print(",");
          Serial.print(motor.shaft_velocity, 1); Serial.print(",");
          Serial.println(motor.voltage.q, 2);
        }
      }
    };



    if (sub == 'p') {
      // ── Tp: Step response test ──
      Serial.println("[TEST] === Step Response Test ===");
      Serial.println("[TEST] 0 -> +90 -> 0 -> +180 -> 0 -> -90 -> 0 (3s each)");
      const float steps[] = { 0, 1.5708f, 0, 3.1416f, 0, -1.5708f, 0 };
      for (int s = 0; s < 7; s++) runStep(steps[s], 3000);

    } else if (sub == 'r') {
      // ── Tr: Repeatability test (dial gauge demo) ──
      Serial.println("[TEST] === Repeatability Test (Dial Gauge) ===");
      Serial.println("[TEST] HOME(5s) -> -90(3s) -> HOME(5s) -> -180(3s) -> HOME(5s) -> -270(3s) -> HOME(5s)");
      const float angles[] = { -1.5708f, -3.1416f, -4.7124f }; // -90, -180, -270 deg
      runStep(0, 5000);  // Initial home - 5s for dial gauge zero
      for (int i = 0; i < 3; i++) {
        runStep(angles[i], 3000);  // Away position - 3s
        runStep(0, 5000);          // Return home - 5s for dial gauge reading
      }

    } else if (sub == 'g') {
      // ── Tg: Gravity compensation logging test ──
      if (opMode != 5) {
        Serial.println("[TEST] Error: must be in m5 (GravComp) mode first!");
        logMode = false;
        break;
      }
      Serial.println("[TEST] === Gravity Compensation Test ===");
      Serial.print("[TEST] Iq="); Serial.print(manual_target, 4); Serial.println("A");
      Serial.println("[TEST] 30s logging. Move weight freely, release to verify hold.");

      // Enable if not running
      if (!motorRunning) {
        DRV::drv8316.clearFault(); delay(5);
        motor.enable();
        motorRunning = true;
        Serial.println("[TEST] Motor enabled.");
        delay(500);
      }

      // Log for 30 seconds at 200Hz
      unsigned long testStart = millis();
      float startAngle = motor.shaft_angle;
      while (millis() - testStart < 30000) {
        motor.loopFOC();
        motor.move(manual_target);  // Constant Iq target
        static unsigned long lastLog = 0;
        unsigned long now_us = micros();
        if (now_us - lastLog >= 5000) {
          lastLog = now_us;
          float a_deg = (motor.shaft_angle - startAngle) * (180.0f / PI);
          Serial.print(millis());       Serial.print(",");
          Serial.print(manual_target, 4); Serial.print(",");  // Iq target
          Serial.print(a_deg, 2);       Serial.print(",");
          Serial.print(motor.shaft_velocity, 3); Serial.print(",");
          Serial.print(motor.current.q, 4); Serial.print(",");  // Actual Iq
          Serial.println(motor.shaft_angle * (180.0f / PI), 2);
        }
      }
    }

    logMode = false;
    Serial.println("[TEST] === Test Complete ===");
    break;
  }

  // ── Logging ──
  case 'L':
    logMode = !logMode;
    if (logMode) {
      Serial.println("LOG ON (200Hz)");
      Serial.println("ts_ms,target_deg,angle_deg,error_deg,velocity,Vq");
    } else {
      Serial.println("LOG OFF");
    }
    break;

  // ── Status ──
  case 'S':
    if (cmd.length() > 1 && cmd.charAt(1) == 'r') {
      // Safety reset
      Safety::reset();
    } else {
      Telem::printFullStatus();
    }
    break;

  // ── Clear faults ──
  case 'C':
    DRV::clearAndCheckFault();
    break;

  // ── Diagnostics (D sub-commands) ──
  case 'D':
    if (cmd.length() > 1) {
      char sub = cmd.charAt(1);
      switch (sub) {
        case 'd': DRV::dumpAllRegisters(); break;
        case 'a': CS::dumpADCRegisters(); break;
        case 'e': sensor.dumpAllRegisters(); break;
        case 's': DRV::dumpSPIRegisters(); break;  // SPI bus diagnostic
        case 'f': Safety::printStatus(); break;     // Safety (was 's')
        case 'c': MCU::printClockInfo(); break;
        case '*':
          DRV::dumpAllRegisters();
          CS::dumpADCRegisters();
          sensor.dumpAllRegisters();
          DRV::dumpSPIRegisters();
          Safety::printStatus();
          MCU::printClockInfo();
          // ── TIM1 PWM Register Dump ──
          {
            Serial.println("\n[TIM1] === PWM Register Dump ===");
            Serial.print("[TIM1] CR1=0x"); Serial.println(TIM1->CR1, HEX);
            Serial.print("[TIM1] CR2=0x"); Serial.println(TIM1->CR2, HEX);
            Serial.print("[TIM1] CCER=0x"); Serial.println(TIM1->CCER, HEX);
            Serial.print("[TIM1] BDTR=0x"); Serial.println(TIM1->BDTR, HEX);
            Serial.print("[TIM1] CCMR1=0x"); Serial.println(TIM1->CCMR1, HEX);
            Serial.print("[TIM1] CCMR2=0x"); Serial.println(TIM1->CCMR2, HEX);
            Serial.print("[TIM1] ARR="); Serial.println(TIM1->ARR);
            Serial.print("[TIM1] CCR1="); Serial.print(TIM1->CCR1);
            Serial.print("  CCR2="); Serial.print(TIM1->CCR2);
            Serial.print("  CCR3="); Serial.println(TIM1->CCR3);
            // Decode CCER for each channel
            uint32_t ccer = TIM1->CCER;
            Serial.print("[TIM1] CH1(PhC): EN="); Serial.print((ccer >> 0) & 1);
            Serial.print(" NEN="); Serial.println((ccer >> 2) & 1);
            Serial.print("[TIM1] CH2(PhB): EN="); Serial.print((ccer >> 4) & 1);
            Serial.print(" NEN="); Serial.println((ccer >> 6) & 1);
            Serial.print("[TIM1] CH3(PhA): EN="); Serial.print((ccer >> 8) & 1);
            Serial.print(" NEN="); Serial.println((ccer >> 10) & 1);
            Serial.print("[TIM1] MOE(main output)="); Serial.println((TIM1->BDTR >> 15) & 1);
            // GPIO alternate function check for PA9 (Phase B high)
            uint32_t afrh_a = GPIOA->AFR[1];  // AFRH: PA8-PA15
            uint32_t af_pa9 = (afrh_a >> ((9-8)*4)) & 0xF;
            Serial.print("[GPIO] PA9 AF="); Serial.print(af_pa9);
            Serial.println(af_pa9 == 6 ? " (TIM1_CH2 ✓)" : " (⚠ NOT TIM1!)");
            uint32_t afrh_b = GPIOB->AFR[1];  // AFRH: PB8-PB15
            uint32_t af_pb14 = (afrh_b >> ((14-8)*4)) & 0xF;
            Serial.print("[GPIO] PB14 AF="); Serial.print(af_pb14);
            Serial.println(af_pb14 == 6 ? " (TIM1_CH2N ✓)" : " (⚠ NOT TIM1!)");
          }
          break;
        default:
          Serial.println("[CMD] D: d=DRV a=ADC e=Encoder s=Safety c=Clock *=ALL");
          break;
      }
    } else {
      // Legacy: run old-style diagnostics
      DRV::runDiagnostics();
      sensor.runDiagnostics();
    }
    break;

  // ── PID tuning ──
  case 'p':
    if (cmd.length() > 1) {
      motor.PID_velocity.P = cmd.substring(1).toFloat();
      Serial.print("[CMD] P="); Serial.println(motor.PID_velocity.P, 4);
    }
    break;
  case 'i':
    if (cmd.length() > 1) {
      motor.PID_velocity.I = cmd.substring(1).toFloat();
      Serial.print("[CMD] I="); Serial.println(motor.PID_velocity.I, 4);
    }
    break;
  case 'k':
    if (cmd.length() > 1) {
      motor.PID_velocity.D = cmd.substring(1).toFloat();
      Serial.print("[CMD] velD="); Serial.println(motor.PID_velocity.D, 4);
    }
    break;
  case 'a':
    if (cmd.length() > 1) {
      motor.P_angle.P = cmd.substring(1).toFloat();
      Serial.print("[CMD] aP="); Serial.println(motor.P_angle.P, 4);
    }
    break;
  case 'j':  // angI tuning
    if (cmd.length() > 1) {
      motor.P_angle.I = cmd.substring(1).toFloat();
      Serial.print("[CMD] aI="); Serial.println(motor.P_angle.I, 4);
    }
    break;
  case 'r':
    if (cmd.length() > 1) {
      motor.PID_velocity.output_ramp = cmd.substring(1).toFloat();
      Serial.print("[CMD] ramp="); Serial.println(motor.PID_velocity.output_ramp, 1);
    }
    break;
  case 'f':
    if (cmd.length() > 1) {
      motor.LPF_velocity.Tf = cmd.substring(1).toFloat();
      Serial.print("[CMD] Tf="); Serial.println(motor.LPF_velocity.Tf, 4);
    }
    break;

  // ── Id/Iq toggle ──
  case 'I':
    showIdIq = !showIdIq;
    Serial.println(showIdIq ? "[CMD] Id/Iq display ON" : "[CMD] Id/Iq display OFF");
    break;

  // ── CSA alignment test ──
  case 'X': {
    Serial.println("\n[CSA] === Current Sense Alignment Test ===");
    motorRunning = false;
    motor.disable();
    delay(500);

    Serial.print("[CSA] Offsets: Ia_off="); Serial.print(CS::currentSense.offset_ia, 4);
    Serial.print("V  Ib_off="); Serial.println(CS::currentSense.offset_ib, 4);

    float test_v = 1.5f;
    float angles[] = {0, _2PI/3, 2*_2PI/3};
    const char* labels[] = {"0 deg", "120 deg", "240 deg"};

    for (int ii = 0; ii < 3; ii++) {
      motor.enable();
      for (int j = 0; j < 200; j++) {
        motor.setPhaseVoltage(test_v, 0, angles[ii]);
        delayMicroseconds(50);
      }
      PhaseCurrent_s pc = CS::currentSense.getPhaseCurrents();
      motor.setPhaseVoltage(0, 0, 0);
      motor.disable();

      Serial.print("[CSA] "); Serial.print(labels[ii]);
      Serial.print("  Ia="); Serial.print(pc.a, 3);
      Serial.print(" Ib="); Serial.print(pc.b, 3);
      Serial.print(" Ic="); Serial.println(pc.c, 3);
      delay(500);
    }
    break;
  }

  // ── Motor ID ──
  case 'R': {
    if (motorRunning) { Serial.println("[CMD] Disable motor first! (d)"); break; }
    Serial.println("[CMD] Running Motor ID...");
    motorID.test_voltage = 1.0f;
    motorID.test_speed = 5.0f;
    motorID.test_duration_ms = 3000;
    motorID.runFullID(motor, sensor, CS::currentSense, driver);
    if (motorID.valid) {
      motorID.applyToMotor(motor);
      if (motorID.Ke > 0.001f) {
        motor.KV_rating = 1.0f / (motorID.Ke * 1.732f * 0.10472f);
      }
      Serial.println("[CMD] Motor params applied!");
    }
    motor.disable();
    break;
  }

  // ── Motor bench ──
  case 'B': {
    if (cmd.length() > 1) {
      motorRunning = false;
      motor.move(0);
      delay(100);
      char sub = cmd.charAt(1);
      switch (sub) {
        case '1': ::mbench_phase1(); break;
        case '2': ::mbench_phase2(); break;
        case '3': ::mbench_phase3(); break;
        case '4': ::mbench_phase4(); break;
        case 'a': ::mbench_run_all(); break;
        default: Serial.println("[CMD] B1=Static B2=Sweep B3=OCP B4=Sustain Ba=All"); break;
      }
    } else {
      Serial.println("[CMD] B1=Static B2=Sweep B3=OCP B4=Sustain Ba=All");
    }
    break;
  }

  // ── Sensor calibration ──
  case 'c': {
    ::runSensorCalibration();
    break;
  }

  // ── Encoder streaming mode ──
  case 'E': {
#ifdef ENCODER_TLI49012
    if (cmd == "Ea") {
      // 수동 보정 관찰(c)과 실제로 모터가 움직이는 전기각 정렬(Ea)을 분리한다.
      motorRunning = false;
      motor.disable();
      if (Safety::isShutdown() || sensor.getSensorAngle()<0 || !sensor.calibrated()) {
        Serial.println("[ENC] Alignment rejected: check safety, sensor and SysCal status.");
        break;
      }
      Serial.println("[ENC] Electrical alignment: motor will move.");
      // 기존 방향/영점이 남아 있으면 initFOC()가 정렬을 생략할 수 있어 먼저 무효화한다.
      motor.zero_electric_angle = NOT_SET;
      motor.sensor_direction = Direction::UNKNOWN;
      motor.enable();
      encoderNeedsAlignment = !motor.initFOC() || !sensor.healthy();
      motor.disable();
      break;
    }
#endif
    if (cmd.length() >= 2 && cmd.charAt(1) == 's') {
      // --- Encoder Stream: raw + corrected + MA + unwrapped @ 100Hz ---
      const int MA_SIZE = 16;
      float ma_buf[MA_SIZE];  // in degrees (corrected)
      int ma_idx = 0;

      // Pre-fill moving average buffer
      for (int i = 0; i < MA_SIZE; i++) {
        sensor.update();
        float corr_rad = sensor.getSensorAngle();  // LUT-corrected
        ma_buf[i] = corr_rad * (float)(180.0 / M_PI);
        delay(5);
      }

      Serial.println("\n[ENC_STREAM] === Encoder Streaming Mode ===");
      Serial.print("[ENC_STREAM] LUT: ");
      Serial.println(sensor.use_lut ? "ON" : "OFF");
      Serial.println("[ENC_STREAM] Columns: ts_ms, raw16, deg, corr_deg, ma_deg, unwrap_deg");
      Serial.println("[ENC_STREAM] Press any key to stop.");
      Serial.println("ts_ms,raw16,deg,corr_deg,ma_deg,unwrap_deg");

      // Flush any pending serial input
      while (Serial.available()) Serial.read();

      unsigned long lastSample = 0;
      float prev_corr = ma_buf[MA_SIZE - 1];
      float cumulative = 0.0f;  // unwrap starts from 0°

      while (true) {
        // Check for stop key
        if (Serial.available()) {
          while (Serial.available()) Serial.read();
          break;
        }

        unsigned long now = millis();
        if (now - lastSample < 10) continue;  // 100Hz
        lastSample = now;

        // Read raw → degrees (uncorrected)
#ifdef ENCODER_TLI49012
        TLI49012Sensor::Sample sample;
        // 실패 시 마지막 각도를 조용히 재사용하지 않고 해당 출력 표본을 건너뛴다.
        if (!sensor.readSample(sample)) { Serial.println("[ENC_STREAM] Invalid sample"); continue; }
        uint16_t raw = sample.angle16;
#else
        uint16_t raw = sensor.readRawAngle();
#endif
        float deg = (float)raw / 65536.0f * 360.0f;

        // Read corrected angle via getSensorAngle (applies LUT)
        float corr_rad = (float)raw / 65536.0f * _2PI;
        if (sensor.use_lut) corr_rad = sensor.applyLUT(corr_rad);
        float corr_deg = corr_rad * (float)(180.0 / M_PI);

        // Unwrap corrected angle: detect 0/360 crossing
        float delta = corr_deg - prev_corr;
        if (delta > 180.0f)  delta -= 360.0f;
        if (delta < -180.0f) delta += 360.0f;
        cumulative += delta;
        prev_corr = corr_deg;

        // Update moving average (circular mean of corrected degrees)
        ma_buf[ma_idx] = corr_deg;
        ma_idx = (ma_idx + 1) % MA_SIZE;
        float sin_sum = 0, cos_sum = 0;
        for (int i = 0; i < MA_SIZE; i++) {
          float r = ma_buf[i] * (float)(M_PI / 180.0);
          sin_sum += sinf(r);
          cos_sum += cosf(r);
        }
        float ma_deg = atan2f(sin_sum, cos_sum) * (float)(180.0 / M_PI);
        if (ma_deg < 0) ma_deg += 360.0f;

        // Output CSV line
        Serial.print(now);         Serial.print(',');
        Serial.print(raw);         Serial.print(',');
        Serial.print(deg, 2);      Serial.print(',');
        Serial.print(corr_deg, 2); Serial.print(',');
        Serial.print(ma_deg, 2);   Serial.print(',');
        Serial.println(cumulative, 2);
      }

      Serial.println("[ENC_STREAM] Stopped.");
    } else {
      Serial.println("[CMD] Es=Encoder stream");
    }
    break;
  }

  // ── Manual Motor Step Test (bypasses SimpleFOC open-loop) ──
  case 'G': {
    Serial.println("\n[TEST] ========================================");
    Serial.println("[TEST] MOTOR DIAGNOSTIC — Phase + PP Sweep");
    Serial.println("[TEST] ========================================\n");
    
    float V_test = 5.0f;
    
    // ── Phase check: apply voltage to each phase pair and read current ──
    Serial.println("[TEST] Phase current check (V=5V, 500ms each):");
    motor.enable();
    delay(100);
    
    // Step 1: 0° electrical → Phase A energized
    motor.setPhaseVoltage(V_test, 0, 0);
    delay(500);
    PhaseCurrent_s c1 = CS::currentSense.getPhaseCurrents();
    Serial.print("[TEST] e_angle=0    Ia="); Serial.print(c1.a, 3);
    Serial.print(" Ib="); Serial.print(c1.b, 3);
    Serial.print(" Ic="); Serial.println(c1.c, 3);
    Serial.print("[TEST] TIM1: CCER=0x"); Serial.print(TIM1->CCER, HEX);
    Serial.print(" CCR1="); Serial.print(TIM1->CCR1);
    Serial.print(" CCR2="); Serial.print(TIM1->CCR2);
    Serial.print(" CCR3="); Serial.print(TIM1->CCR3);
    Serial.print(" ARR="); Serial.println(TIM1->ARR);
    
    // Step 2: 120° electrical → Phase B energized
    motor.setPhaseVoltage(V_test, 0, _2PI / 3.0f);
    delay(500);
    PhaseCurrent_s c2 = CS::currentSense.getPhaseCurrents();
    Serial.print("[TEST] e_angle=120  Ia="); Serial.print(c2.a, 3);
    Serial.print(" Ib="); Serial.print(c2.b, 3);
    Serial.print(" Ic="); Serial.println(c2.c, 3);
    Serial.print("[TEST] TIM1: CCER=0x"); Serial.print(TIM1->CCER, HEX);
    Serial.print(" CCR1="); Serial.print(TIM1->CCR1);
    Serial.print(" CCR2="); Serial.print(TIM1->CCR2);
    Serial.print(" CCR3="); Serial.print(TIM1->CCR3);
    Serial.print(" ARR="); Serial.println(TIM1->ARR);
    
    // Step 3: 240° electrical → Phase C energized
    motor.setPhaseVoltage(V_test, 0, 2.0f * _2PI / 3.0f);
    delay(500);
    PhaseCurrent_s c3 = CS::currentSense.getPhaseCurrents();
    Serial.print("[TEST] e_angle=240  Ia="); Serial.print(c3.a, 3);
    Serial.print(" Ib="); Serial.print(c3.b, 3);
    Serial.print(" Ic="); Serial.println(c3.c, 3);
    Serial.print("[TEST] TIM1: CCER=0x"); Serial.print(TIM1->CCER, HEX);
    Serial.print(" CCR1="); Serial.print(TIM1->CCR1);
    Serial.print(" CCR2="); Serial.print(TIM1->CCR2);
    Serial.print(" CCR3="); Serial.print(TIM1->CCR3);
    Serial.print(" ARR="); Serial.println(TIM1->ARR);
    
    motor.setPhaseVoltage(0, 0, 0);
    delay(200);
    
    // ── Check if any phase is dead ──
    float ia_max = fmaxf(fabsf(c1.a), fmaxf(fabsf(c2.a), fabsf(c3.a)));
    float ib_max = fmaxf(fabsf(c1.b), fmaxf(fabsf(c2.b), fabsf(c3.b)));
    float ic_max = fmaxf(fabsf(c1.c), fmaxf(fabsf(c2.c), fabsf(c3.c)));
    Serial.print("\n[TEST] Peak |Ia|="); Serial.print(ia_max, 3);
    Serial.print("  |Ib|="); Serial.print(ib_max, 3);
    Serial.print("  |Ic|="); Serial.println(ic_max, 3);
    if (ia_max < 0.1f) Serial.println("[TEST] ⚠ Phase A appears DEAD!");
    if (ib_max < 0.1f) Serial.println("[TEST] ⚠ Phase B appears DEAD!");
    if (ic_max < 0.1f) Serial.println("[TEST] ⚠ Phase C appears DEAD!");
    
    // ── Pole pair sweep: try PP=1 through 8 ──
    Serial.println("\n[TEST] Pole pair sweep (12 steps each, V=5V):");
    Serial.println("[TEST] Watch the motor shaft — which PP makes it rotate smoothly?");
    
    for (int pp = 1; pp <= 8; pp++) {
      Serial.print("\n[TEST] >>> PP="); Serial.print(pp);
      Serial.println(" — 12 steps (60° elec each)...");
      delay(1000);  // Pause so user can watch
      
      int steps_per_mech_rev = 6 * pp;
      int total_steps = steps_per_mech_rev * 2;  // 2 mechanical revolutions
      if (total_steps > 96) total_steps = 96;     // Cap at 96 steps
      
      for (int i = 0; i < total_steps; i++) {
        float e_angle = (_2PI / 6.0f) * (float)i;
        motor.setPhaseVoltage(V_test, 0, e_angle);
        delay(200);
        
        // Check for fault
        if (DRV::isFaulted()) {
          Serial.println("[TEST] FAULT! Clearing...");
          DRV::drv8316.clearFault();
          delay(100);
        }
      }
      
      motor.setPhaseVoltage(0, 0, 0);
      delay(300);
    }
    
    motor.disable();
    Serial.println("\n[TEST] ========================================");
    Serial.println("[TEST] DONE. Which PP number made the motor rotate?");
    Serial.println("[TEST] If NONE rotated → wiring or mechanical issue.");
    Serial.println("[TEST] ========================================");
    break;
  }

  // ── Help ──
  case '?':
  case 'h':
  case 'H':
    printHelp();
    break;

  default:
    Serial.print("[CMD] Unknown: '"); Serial.print(c);
    Serial.println("'. Send '?' for help.");
    break;
  }
}

} // namespace Cmd
