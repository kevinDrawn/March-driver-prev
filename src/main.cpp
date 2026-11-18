// =============================================================================
//  SimpleFOC Motor Controller — Modular Build
//  Hardware: STM32G431 + DRV8316 + MAQ473
//
//  Module Structure:
//    driver/   — DRV8316 SPI driver wrapper
//    encoder/  — MAQ473 angle sensor
//    mcu/      — Clock, ADC/current sense, GPIO diagnostics
//    motor/    — Motor control, MotorID, trajectory, controllers
//    safety/   — SW current limiting & stall detection
//    bench/    — HW bench (no motor) + motor bench
//    app/      — Serial commands + telemetry
// =============================================================================
#include <Arduino.h>
#include <SPI.h>
#include <SimpleFOC.h>

// Module headers
#include "app/serial_cmd.h"
#include "app/telemetry.h"
#include "config.h"
#include "driver/drv8316_helper.h"
#include "encoder/Encoder.h"
#ifdef ENCODER_TLI49012
#include "encoder/EncoderCalibration.h"
#include "encoder/tli49012_lut.h"
// 보정 후 전기각 기준이 달라질 수 있으므로 명시적 재정렬 전에는 e 명령을 막는다.
bool encoderNeedsAlignment = false;
#endif
#include "mcu/clock.h"
#include "mcu/current_sense.h"
#include "motor/MotorID.h"
#include "motor/TrajectoryGen.h"
#include "safety/safety.h"

// =============================================================================
//  BENCH TEST MODE — Motor-free hardware verification
// =============================================================================
#ifdef BENCH_TEST_MODE
// (Bench test code stays in bench/hw_bench — TODO: extract)
// For now, minimal bench setup/loop that delegates
#include "bench/hw_bench.h"
void setup() { /* will be filled when hw_bench is complete */ }
void loop() {}

#else // ── Normal FOC mode ──────────────────────────────────────────────────

// =============================================================================
//  Global Hardware Objects
// =============================================================================
#ifdef USE_CUSTOM_BOARD
BLDCDriver6PWM driver(PIN_PWMA_H, PIN_PWMA_L, PIN_PWMB_H, PIN_PWMB_L,
                      PIN_PWMC_H, PIN_PWMC_L);
#else
BLDCDriver3PWM driver(PIN_INHA, PIN_INHB, PIN_INHC, PIN_INLA, PIN_INLB,
                      PIN_INLC);
#endif

BLDCMotor motor(MOTOR_POLE_PAIRS);
#ifdef USE_CUSTOM_BOARD
// Encoder and DRV8316 use separate SPI buses on the custom boards.
SPIClass spiENC(PIN_ENCODER_MOSI, PIN_ENCODER_MISO, PIN_ENCODER_SCK, -1);
EncoderSensor sensor(PIN_ENCODER_CS, &spiENC, ENCODER_SPI_CLOCK);
#else
// Nucleo: both share SPI1
EncoderSensor sensor(PIN_MAQ_CS, &DRV::spiDRV, MAQ473_SPI_CLOCK);
#endif
TrajectoryGen traj;
MotorID motorID;

// =============================================================================
//  Global State
// =============================================================================
float manual_target = 0.0f;
float target = 0.0f;
float ramped_target = 0.0f;
float pos_offset = 0.0f;
bool motorRunning = false;
bool logMode = false;
bool showIdIq = false;
int opMode = 0; // 0:VelOL 1:AngOL 2:VelCL 3:AngCL 4:Torque  ★ Start open-loop
float current_v = 0.0f;

#define CLOSEDLOOP_VOLTAGE_LIMIT MOTOR_VOLTAGE_LIMIT

// =============================================================================
//  Setup
// =============================================================================
void setup() {
#ifdef USE_H723_BOARD
  // DRVOFF is active HIGH. Preload the latch before enabling the GPIO output.
  digitalWrite(PIN_DRVOFF, HIGH);
  pinMode(PIN_DRVOFF, OUTPUT);
#endif
  Serial.begin(SERIAL_BAUD);
  Serial.setTimeout(1);
  SimpleFOCDebug::enable(&Serial);
  delay(1500);
  Serial.println("\n========================================");
  Serial.println("  FOC Controller — Modular Build");
  Serial.println("========================================\n");

  // --- GPIO ---
#ifdef PIN_nFAULT
  pinMode(PIN_nFAULT, INPUT);
#endif
#ifdef USE_CUSTOM_BOARD
  delay(100);
#else
  pinMode(PIN_nSLEEP, OUTPUT);
  pinMode(PIN_DRVOFF, OUTPUT);
  digitalWrite(PIN_nSLEEP, HIGH);
  digitalWrite(PIN_DRVOFF, LOW);
  delay(100);
#endif

  // --- DRV8316 ---
  DRV::init();
  DRV::configure();

  // --- Driver init ---
  driver.voltage_power_supply = SUPPLY_VOLTAGE;
  driver.pwm_frequency = FOC_PWM_FREQUENCY;
  driver.voltage_limit = SUPPLY_VOLTAGE;
#ifdef USE_H723_BOARD
  // Let every driver/motor enable and disable path control DRVOFF,
  // including alignment, bench tests, MotorID and safety shutdown.
  driver.enable_pin = PIN_DRVOFF;
  driver.enable_active_high = false;
#endif
  if (!driver.init()) { Serial.println("[INIT] PWM failed"); while (true) delay(1000); }

  // --- Current sense ---
  CS::currentSense.linkDriver(&driver);
  CS::currentSense.skip_align = true;
  if (!CS::currentSense.init()) { driver.disable(); Serial.println("[INIT] ADC failed"); while (true) delay(1000); }

  // --- Encoder ---
#ifdef ENCODER_TLI49012
  // 저장된 TLI 전용 MCU 보정을 센서 이력 초기화/FOC 정렬 전에 적용한다.
  if (TLI49012_LUT_VALID && !sensor.loadLUT(TLI49012_LUT,128)) {
    driver.disable(); Serial.println("[ENC] Saved LUT invalid"); while (true) delay(1000);
  }
#endif
  sensor.init();
#ifdef ENCODER_TLI49012
  if (!sensor.healthy()) {
    // 통신이 확인되지 않은 각도로 FOC 정렬을 시작하지 않는다. 단독 시험 환경에서 진단한다.
    driver.disable();
    Serial.println("[ENC] TLI49012 invalid: DRVOFF HIGH; use custom_h723_encoder diagnostics.");
    while (true) delay(1000);
  }
#endif
  Serial.println("[SEN] Post-init reads:");
  for (int i = 0; i < 3; i++) {
    float a = sensor.getSensorAngle();
    Serial.print("  ["); Serial.print(i); Serial.print("] angle=");
    Serial.println(a, 4);
    delay(50);
  }

  // --- Motor ---
  motor.linkDriver(&driver);
  motor.linkCurrentSense(&CS::currentSense);
  motor.linkSensor(&sensor);

  motor.torque_controller = TorqueControlType::foc_current;
  motor.controller = MotionControlType::velocity_openloop;
  motor.voltage_limit = OPENLOOP_VOLTAGE_LIMIT;
  motor.current_limit = MOTOR_CURRENT_LIMIT;
  motor.velocity_limit = 120.0f;

  // --- Current PID (innermost loop) ---
  motor.PID_current_q.P = FOC_CURRENT_P;
  motor.PID_current_q.I = FOC_CURRENT_I;
  motor.PID_current_q.D = FOC_CURRENT_D;
  motor.PID_current_q.output_ramp = FOC_CURRENT_RAMP;
  motor.PID_current_q.limit = FOC_CURRENT_LIM;
  motor.PID_current_d.P = FOC_CURRENT_P;
  motor.PID_current_d.I = FOC_CURRENT_I;
  motor.PID_current_d.D = FOC_CURRENT_D;
  motor.PID_current_d.output_ramp = FOC_CURRENT_RAMP;
  motor.PID_current_d.limit = FOC_CURRENT_LIM;
  motor.LPF_current_q.Tf = FOC_LPF_TF;
  motor.LPF_current_d.Tf = FOC_LPF_TF;

  // --- Velocity PID (middle loop) ---
  motor.PID_velocity.P = VEL_PID_P;
  motor.PID_velocity.I = VEL_PID_I;
  motor.PID_velocity.D = VEL_PID_D;
  motor.PID_velocity.output_ramp = VEL_PID_RAMP;
  motor.PID_velocity.limit = MOTOR_CURRENT_LIMIT;  // Amps (foc_current mode)
  motor.LPF_velocity.Tf = VEL_LPF_TF;

  // --- Angle/Position PID (outermost loop) ---
  motor.P_angle.P = ANG_PID_P;
  motor.P_angle.I = ANG_PID_I;
  motor.P_angle.D = ANG_PID_D;
  motor.P_angle.limit = ANG_PID_LIM;

  motor.init();

  // --- initFOC ---
  // Default voltage_sensor_align=3V; EC-i direct drive needs less
  // 2V → ~2.6A → ~82mNm (more than enough without gearbox)
  motor.voltage_sensor_align = 2.0f;
  Serial.println("[FOC] initFOC (sensor alignment, Valign=2V)...");
#ifdef ENCODER_TLI49012
  // 정렬 실패를 기억해 나중에 e 명령만으로 잘못된 전기각을 사용하지 못하게 한다.
  encoderNeedsAlignment = !motor.initFOC() || !sensor.healthy();
#else
  motor.initFOC();
#endif
  Serial.println("[FOC] Open-loop ready.");

  #ifndef ENCODER_TLI49012
  // 아래 표는 MAQ473 조립체의 측정값이므로 새 센서에는 적용하지 않는다.
  // --- Load encoder non-linearity LUT ---
  {
    #include "encoder/sensor_lut_128.h"
    for (int i = 0; i < 128; i++) {
      sensor.lut_error[i] = SENSOR_LUT[i];
    }
    sensor.use_lut = true;
    Serial.println("[CAL] LUT loaded (128 slots, spline-interpolated).");
  }

  #endif // TLI49012 must never load the MAQ473 LUT

  // --- JSQR patch ---
  CS::patchJSQR();

  #ifndef USE_H723_BOARD
  // --- ADC sampling time fix ---
  {
    uint32_t smp_val = 1; // 6.5 ADC cycles
    uint32_t smpr1 = ADC1->SMPR1;
#ifdef USE_CUSTOM_BOARD
    smpr1 &= ~(0x7 << 3);
    smpr1 |= (smp_val << 3); // CH1
    smpr1 &= ~(0x7 << 6);
    smpr1 |= (smp_val << 6); // CH2
    smpr1 &= ~(0x7 << 9);
    smpr1 |= (smp_val << 9); // CH3
    ADC1->SMPR1 = smpr1;
#else
    smpr1 &= ~(0x7 << 6);
    smpr1 |= (smp_val << 6); // CH2
    ADC1->SMPR1 = smpr1;
    uint32_t smpr2 = ADC1->SMPR2;
    smpr2 &= ~(0x7 << 15);
    smpr2 |= (smp_val << 15); // CH15
    ADC1->SMPR2 = smpr2;
#endif
  }

  #endif

  // --- Clear post-initFOC faults ---
  DRV::clearAndCheckFault();

  // --- Recalibrate current sense offsets ---
  motor.disable();
  delay(200);
  CS::calibrateOffsets();

  // --- Safety module ---
  Safety::init({
      .max_current_A    = SAFETY_MAX_CURRENT_A,
      .stall_current_A  = SAFETY_STALL_CURRENT_A,
      .stall_time_ms    = SAFETY_STALL_TIME_MS,
      .warn_current_A   = SAFETY_WARN_CURRENT_A,
  });

  motor.disable();
  Serial.println("\n=== READY === (send '?' for help)");
  Cmd::printHelp();
}

// =============================================================================
//  Loop
// =============================================================================
unsigned long last_time_us = 0;

void loop() {
  unsigned long now = millis();
  unsigned long now_us = micros();
  float dt = (now_us - last_time_us) / 1e6f;
  last_time_us = now_us;

  // --- Heartbeat ---
  Telem::heartbeat(now);

  // --- FOC loop ---
  if (motorRunning) {
    motor.loopFOC();
#ifdef ENCODER_TLI49012
    if (!sensor.healthy()) {
      // 일부 잘못된 표본은 Sensor가 건너뛰고, 연속 오류/오래된 표본은 여기서 정지로 처리한다.
      motor.disable(); motorRunning = false;
      Serial.println("[ENC] Invalid/stale angle: motor disabled.");
      return;
    }
#endif

    // Target processing
    if (traj.active()) {
      target = traj.compute(manual_target);
      motor.move(target);
    } else if (motor.controller == MotionControlType::velocity) {
      target = manual_target;
      motor.move(target);
    } else if (motor.controller == MotionControlType::angle) {
      target = manual_target + pos_offset;
      motor.move(target);
    } else if (motor.controller == MotionControlType::torque) {
      motor.move(manual_target);
    } else if (motor.controller == MotionControlType::velocity_openloop) {
      target = manual_target;
      motor.move(manual_target);
    } else if (motor.controller == MotionControlType::angle_openloop) {
      target = manual_target;
      motor.move(target);
    } else {
      motor.move(target);
    }

    // ★ Safety check — every loop iteration
    Safety::check();

    // Id/Iq display
    float e_angle = motor.shaft_angle * MOTOR_POLE_PAIRS;
    Telem::printMotorState(e_angle);
  }

  // --- CSV logging ---
  Telem::csvLog(now_us);

  // --- Serial commands ---
  Cmd::process();
}

// =============================================================================
//  Sensor Calibration (complex routine — kept in main for now)
// =============================================================================
void runSensorCalibration() {
#ifdef ENCODER_TLI49012
  // TLI의 c는 자동 모터 sweep이 아니라 수동 회전 중 내부 SysCal 상태 관찰이다.
  // 성공/취소와 무관하게 모터는 꺼두고, 이후 Ea에서 전기각을 다시 맞춘다.
  motorRunning = false;
  motor.disable();
  encoderNeedsAlignment = true;
  EncoderCalibration::observeSysCal(sensor);
  Serial.println("[ENC] Motor remains disabled. Ea = explicit electrical alignment before enabling.");
#else
  Serial.println("\n[CAL] === Sensor Calibration Starting ===");
  Serial.println("[CAL] Motor will rotate slowly for ~20 seconds.");
  Serial.println("[CAL] Remove any load and keep hands clear!");
  delay(2000);

  motorRunning = false;
  motor.disable();
  sensor.use_lut = false;
  delay(100);

  const int N = EncoderSensor::LUT_SIZE;
  const int PP = MOTOR_POLE_PAIRS;
  const float V_calib = 1.2f;
  const int settle_ms = 80;

  int dir = (motor.sensor_direction == Direction::CW) ? 1 : -1;
  Serial.print("[CAL] Sensor direction: ");
  Serial.println(dir == 1 ? "CW" : "CCW");

  float fwd_sensor[N], rev_sensor[N], expected[N];

  // Forward sweep
  Serial.println("[CAL] Forward sweep...");
  motor.enable();
  for (int s = 0; s < 50; s++) {
    motor.setPhaseVoltage(V_calib, 0, 0.0f);
    delay(20);
  }
  delay(500);

  sensor.update();
  float ref_angle = sensor.getMechanicalAngle();

  for (int i = 0; i < N; i++) {
    float mech_frac = (float)i / (float)N;
    float elec_angle = mech_frac * _2PI * PP;
    motor.setPhaseVoltage(V_calib, 0, elec_angle);
    delay(settle_ms);
    sensor.update();
    float sensor_mech = sensor.getMechanicalAngle();
    float exp_angle = ref_angle + dir * mech_frac * _2PI;
    while (exp_angle < 0)
      exp_angle += _2PI;
    while (exp_angle >= _2PI)
      exp_angle -= _2PI;
    expected[i] = exp_angle;
    fwd_sensor[i] = sensor_mech;
    if (i % 16 == 0) {
      Serial.print("[CAL] Fwd ");
      Serial.print(i);
      Serial.print("/128");
      float e = sensor_mech - exp_angle;
      if (e > _PI)
        e -= _2PI;
      if (e < -_PI)
        e += _2PI;
      Serial.print(" err=");
      Serial.println(e * 180.0f / _PI, 2);
    }
  }

  // Reverse sweep
  Serial.println("[CAL] Reverse sweep...");
  delay(500);
  for (int i = N - 1; i >= 0; i--) {
    float elec_angle = (float)i / (float)N * _2PI * PP;
    motor.setPhaseVoltage(V_calib, 0, elec_angle);
    delay(settle_ms);
    sensor.update();
    rev_sensor[i] = sensor.getMechanicalAngle();
  }
  motor.setPhaseVoltage(0, 0, 0);
  motor.disable();
  delay(200);

  // Compute error table
  float temp_lut[N];
  int slot_count[N];
  for (int i = 0; i < N; i++) {
    temp_lut[i] = 0;
    slot_count[i] = 0;
  }
  float max_err = 0;

  for (int i = 0; i < N; i++) {
    float diff = rev_sensor[i] - fwd_sensor[i];
    if (diff > _PI)
      diff -= _2PI;
    if (diff < -_PI)
      diff += _2PI;
    float avg_sensor = fwd_sensor[i] + diff * 0.5f;
    if (avg_sensor < 0)
      avg_sensor += _2PI;
    if (avg_sensor >= _2PI)
      avg_sensor -= _2PI;

    float err = avg_sensor - expected[i];
    if (err > _PI)
      err -= _2PI;
    if (err < -_PI)
      err += _2PI;
    if (fabsf(err) > max_err)
      max_err = fabsf(err);

    int slot = (int)(avg_sensor * N / _2PI);
    if (slot < 0)
      slot += N;
    if (slot >= N)
      slot = N - 1;
    temp_lut[slot] += err;
    slot_count[slot]++;
  }

  for (int i = 0; i < N; i++) {
    if (slot_count[i] > 0)
      temp_lut[i] /= slot_count[i];
  }
  for (int i = 0; i < N; i++) {
    if (slot_count[i] == 0) {
      int prev = (i - 1 + N) % N;
      while (slot_count[prev] == 0 && prev != i)
        prev = (prev - 1 + N) % N;
      int next = (i + 1) % N;
      while (slot_count[next] == 0 && next != i)
        next = (next + 1) % N;
      if (prev != i && next != i)
        temp_lut[i] = (temp_lut[prev] + temp_lut[next]) * 0.5f;
    }
  }

  // 5-tap smooth
  for (int i = 0; i < N; i++) {
    float sum = 0;
    for (int k = -2; k <= 2; k++)
      sum += temp_lut[(i + k + N) % N];
    sensor.lut_error[i] = sum / 5.0f;
  }

  // Remove DC offset
  float lut_sum = 0;
  for (int i = 0; i < N; i++)
    lut_sum += sensor.lut_error[i];
  float mean_err = lut_sum / N;
  for (int i = 0; i < N; i++)
    sensor.lut_error[i] -= mean_err;

  // Enable LUT + re-align zero angle
  sensor.update();
  float mech_before = sensor.getMechanicalAngle();
  sensor.use_lut = true;
  sensor.update();
  float mech_after = sensor.getMechanicalAngle();
  float delta_mech = mech_after - mech_before;
  if (delta_mech > _PI)
    delta_mech -= _2PI;
  if (delta_mech < -_PI)
    delta_mech += _2PI;
  motor.zero_electric_angle += (float)(dir * PP) * delta_mech;
  motor.zero_electric_angle = _normalizeAngle(motor.zero_electric_angle);

  Serial.print("[CAL] Max err: ");
  Serial.print(max_err * 180.0f / _PI, 2);
  Serial.println(" deg");
  Serial.print("[CAL] New zero_e_angle: ");
  Serial.println(motor.zero_electric_angle, 4);

  // Print C array
  Serial.println("\n// Copy-paste for permanent calibration:");
  Serial.println("const float SENSOR_LUT[128] = {");
  for (int i = 0; i < N; i++) {
    Serial.print("  ");
    Serial.print(sensor.lut_error[i], 6);
    if (i < N - 1)
      Serial.print(",");
    if ((i + 1) % 8 == 0)
      Serial.println();
  }
  Serial.println("};");
  Serial.println("\n[CAL] LUT ACTIVE + zero angle adjusted.");
#endif
}

// Motor bench functions are in bench/motor_bench.cpp
// Declarations needed by serial_cmd.cpp are satisfied by that file.

#endif // !BENCH_TEST_MODE
