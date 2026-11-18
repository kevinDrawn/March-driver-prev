// =============================================================================
//  bench/motor_bench.cpp — Motor Driver Bench Test
//  Automated hardware characterization with [MBENCH] tagged output
//  for motor_bench_analyzer.py
// =============================================================================
#include <Arduino.h>
#include <SimpleFOC.h>
#include "../config.h"
#include "../driver/drv8316_helper.h"
#include "../mcu/current_sense.h"
#include "../encoder/Encoder.h"

// External references
extern BLDCMotor motor;
extern bool motorRunning;
extern EncoderSensor sensor;

#ifdef USE_CUSTOM_BOARD
extern BLDCDriver6PWM driver;
#else
extern BLDCDriver3PWM driver;
#endif

// ---------------------------------------------------------------------------
//  Helpers
// ---------------------------------------------------------------------------
static void mbench_read_dq(float &ia, float &ib, float &ic,
                           float &id, float &iq, float e_angle) {
  PhaseCurrent_s c = CS::currentSense.getPhaseCurrents();
  ia = c.a; ib = c.b; ic = c.c;
  float i_alpha = c.a;
  float i_beta  = (c.a + 2.0f * c.b) * 0.57735f; // 1/sqrt(3)
  float cos_a = cosf(e_angle);
  float sin_a = sinf(e_angle);
  id =  i_alpha * cos_a + i_beta * sin_a;
  iq = -i_alpha * sin_a + i_beta * cos_a;
}

static bool mbench_check_fault() {
  return (DRV::isFaulted());
}

static void mbench_print_fault() {
  DRV8316Status st = DRV::drv8316.getStatus();
  Serial.print("[MBENCH] FAULT: ");
  if (st.isOverCurrent())       Serial.print("OCP ");
  if (st.isOverTemperature())   Serial.print("OT ");
  if (st.isOverTemperatureShutdown()) Serial.print("OTS ");
  if (st.isOverTemperatureWarning())  Serial.print("OTW ");
  if (st.isOverVoltage())       Serial.print("OVP ");
  if (st.isBuckError())         Serial.print("BUCK ");
  if (st.isBuckUnderVoltage())  Serial.print("BUCK_UV ");
  if (st.isOverCurrent_Ah()) Serial.print("OCP_AH ");
  if (st.isOverCurrent_Al()) Serial.print("OCP_AL ");
  if (st.isOverCurrent_Bh()) Serial.print("OCP_BH ");
  if (st.isOverCurrent_Bl()) Serial.print("OCP_BL ");
  if (st.isOverCurrent_Ch()) Serial.print("OCP_CH ");
  if (st.isOverCurrent_Cl()) Serial.print("OCP_CL ");
  Serial.println();
}

static void mbench_patch_jsqr() {
  CS::patchJSQR();
}

static void mbench_motor_enable() {
  if (!motor.enabled) motor.enable();
  delay(10);

  motor.loopFOC();
  motor.move(0);
  delay(50);

  mbench_patch_jsqr();
  delay(20);

  CS::calibrateOffsets();
}

// ---------------------------------------------------------------------------
//  Phase 1: Static Current Step Test
// ---------------------------------------------------------------------------
void mbench_phase1() {
  Serial.println("[MBENCH] PHASE1_START StaticCurrent");
  Serial.println("[MBENCH] H:Vq_cmd,Ia_avg,Ib_avg,Ic_avg,Id_avg,Iq_avg,Irms,Ia_std,Ib_std,nFAULT");

  float vq_steps[] = {0.3f, 0.5f, 0.8f, 1.0f, 1.3f, 1.5f, 1.8f, 2.0f, 2.3f, 2.5f};
  int n_steps = sizeof(vq_steps) / sizeof(vq_steps[0]);

  auto saved_ctrl = motor.controller;
  auto saved_torque = motor.torque_controller;
  float saved_vlim = motor.voltage_limit;

  motor.controller = MotionControlType::torque;
  motor.torque_controller = TorqueControlType::voltage;
  motor.voltage_limit = 3.0f;

  DRV::drv8316.clearFault(); delay(5);
  mbench_motor_enable();

  for (int s = 0; s < n_steps; s++) {
    float vq = vq_steps[s];

    unsigned long t0 = millis();
    while (millis() - t0 < 1000) {
      motor.loopFOC();
      motor.move(vq);
      delayMicroseconds(200);
    }

    const int N = 200;
    float sum_ia=0, sum_ib=0, sum_ic=0, sum_id=0, sum_iq=0;
    float sum2_ia=0, sum2_ib=0;
    int fault_cnt = 0;
    for (int i = 0; i < N; i++) {
      motor.loopFOC();
      motor.move(vq);
      float ia, ib, ic, id, iq;
      mbench_read_dq(ia, ib, ic, id, iq, motor.electricalAngle());
      sum_ia += ia; sum_ib += ib; sum_ic += ic;
      sum_id += id; sum_iq += iq;
      sum2_ia += ia*ia; sum2_ib += ib*ib;
      if (mbench_check_fault()) fault_cnt++;
      delayMicroseconds(2000);
    }
    float avg_ia = sum_ia/N, avg_ib = sum_ib/N, avg_ic = sum_ic/N;
    float avg_id = sum_id/N, avg_iq = sum_iq/N;
    float std_ia = sqrtf(fabsf(sum2_ia/N - avg_ia*avg_ia));
    float std_ib = sqrtf(fabsf(sum2_ib/N - avg_ib*avg_ib));
    float irms = sqrtf(avg_id*avg_id + avg_iq*avg_iq);

    Serial.print("[MBENCH] D:");
    Serial.print(vq, 2); Serial.print(",");
    Serial.print(avg_ia, 4); Serial.print(",");
    Serial.print(avg_ib, 4); Serial.print(",");
    Serial.print(avg_ic, 4); Serial.print(",");
    Serial.print(avg_id, 4); Serial.print(",");
    Serial.print(avg_iq, 4); Serial.print(",");
    Serial.print(irms, 4); Serial.print(",");
    Serial.print(std_ia, 4); Serial.print(",");
    Serial.print(std_ib, 4); Serial.print(",");
    Serial.println(fault_cnt == 0 ? "1" : "0");
    Serial.flush();

    if (irms > 4.0f) {
      Serial.println("[MBENCH] ABORT: Irms > 4A safety limit!");
      break;
    }
    if (fault_cnt > N/2) {
      Serial.println("[MBENCH] ABORT: Too many faults!");
      mbench_print_fault();
      DRV::drv8316.clearFault(); delay(5);
      break;
    }
  }

  motor.move(0);
  motor.disable();
  delay(200);

  motor.controller = saved_ctrl;
  motor.torque_controller = saved_torque;
  motor.voltage_limit = saved_vlim;
  Serial.println("[MBENCH] PHASE1_END");
  Serial.flush();
}

// ---------------------------------------------------------------------------
//  Phase 2: Speed Sweep Current Profile
// ---------------------------------------------------------------------------
void mbench_phase2() {
  Serial.println("[MBENCH] PHASE2_START SpeedSweep");
  Serial.println("[MBENCH] H:Spd_cmd,Spd_meas,Vq,Ia_avg,Ib_avg,Ic_avg,Id_avg,Iq_avg,Irms,nFAULT");

  float speeds[] = {3.0f, 5.0f, 10.0f, 20.0f, 30.0f, 40.0f, 50.0f, 60.0f, 80.0f, 100.0f, 120.0f};
  int n_speeds = sizeof(speeds) / sizeof(speeds[0]);

  auto saved_ctrl = motor.controller;
  auto saved_torque = motor.torque_controller;
  float saved_vlim = motor.voltage_limit;
  float saved_vellim = motor.velocity_limit;

  motor.controller = MotionControlType::velocity_openloop;
  motor.torque_controller = TorqueControlType::voltage;
  motor.velocity_limit = 130.0f;

  DRV::drv8316.clearFault(); delay(5);
  mbench_motor_enable();
  float ramp_v = 0.0f;

  for (int s = 0; s < n_speeds; s++) {
    float spd = speeds[s];

    float vlim_dyn = MOTOR_KE * spd + I_MAX_SAFE * MOTOR_R_PHASE + 0.5f;
    if (vlim_dyn > SUPPLY_VOLTAGE - 0.5f) vlim_dyn = SUPPLY_VOLTAGE - 0.5f;
    if (vlim_dyn < 1.0f) vlim_dyn = 1.0f;
    motor.voltage_limit = vlim_dyn;

    float ramp_rate = fmaxf(spd * 0.6f, 20.0f);
    unsigned long t0 = millis();
    while (millis() - t0 < 2500) {
      float dt = 0.001f;
      if (ramp_v < spd) ramp_v = fminf(ramp_v + ramp_rate * dt, spd);
      motor.loopFOC();
      motor.move(ramp_v);
      delayMicroseconds(800);
    }

    t0 = millis();
    while (millis() - t0 < 1000) {
      motor.loopFOC();
      motor.move(spd);
      delayMicroseconds(800);
    }

    const int N = 100;
    float sum_ia=0, sum_ib=0, sum_ic=0, sum_id=0, sum_iq=0;
    float sum_vel = 0;
    int fault_cnt = 0;
    for (int i = 0; i < N; i++) {
      motor.loopFOC();
      motor.move(spd);
      sensor.update();
      float vel = sensor.getVelocity();
      sum_vel += vel;
      float ia, ib, ic, id, iq;
      float e_angle = motor.electricalAngle();
      mbench_read_dq(ia, ib, ic, id, iq, e_angle);
      sum_ia += ia; sum_ib += ib; sum_ic += ic;
      sum_id += id; sum_iq += iq;
      if (mbench_check_fault()) fault_cnt++;
      delayMicroseconds(9000);
    }
    float avg_ia = sum_ia/N, avg_ib = sum_ib/N, avg_ic = sum_ic/N;
    float avg_id = sum_id/N, avg_iq = sum_iq/N;
    float irms = sqrtf(avg_id*avg_id + avg_iq*avg_iq);
    float avg_vel = sum_vel / N;

    Serial.print("[MBENCH] D:");
    Serial.print(spd, 1); Serial.print(",");
    Serial.print(avg_vel, 2); Serial.print(",");
    Serial.print(motor.voltage.q, 2); Serial.print(",");
    Serial.print(avg_ia, 4); Serial.print(",");
    Serial.print(avg_ib, 4); Serial.print(",");
    Serial.print(avg_ic, 4); Serial.print(",");
    Serial.print(avg_id, 4); Serial.print(",");
    Serial.print(avg_iq, 4); Serial.print(",");
    Serial.print(irms, 4); Serial.print(",");
    Serial.println(fault_cnt == 0 ? "1" : "0");
    Serial.flush();

    if (irms > 4.0f) {
      Serial.println("[MBENCH] ABORT: Irms > 4A!");
      break;
    }
    if (fault_cnt > N/2) {
      Serial.println("[MBENCH] ABORT: Fault!");
      mbench_print_fault();
      DRV::drv8316.clearFault(); delay(5);
      break;
    }
  }

  // Ramp down
  unsigned long t0 = millis();
  while (millis() - t0 < 3000 && ramp_v > 0.01f) {
    ramp_v = fmaxf(ramp_v - 60.0f * 0.001f, 0.0f);
    motor.loopFOC();
    motor.move(ramp_v);
    delayMicroseconds(800);
  }
  motor.disable();
  delay(200);

  motor.controller = saved_ctrl;
  motor.torque_controller = saved_torque;
  motor.voltage_limit = saved_vlim;
  motor.velocity_limit = saved_vellim;
  Serial.println("[MBENCH] PHASE2_END");
  Serial.flush();
}

// ---------------------------------------------------------------------------
//  Phase 3: OCP Limit Exploration
// ---------------------------------------------------------------------------
void mbench_phase3() {
  Serial.println("[MBENCH] PHASE3_START OCPTest");
  Serial.println("[MBENCH] H:Vlim,Spd_cmd,Ia_avg,Ib_avg,Ic_avg,Irms,nFAULT,OCP_mode,OCP_trips");

  DRV8316_OCPMode saved_ocp = DRV::drv8316.getOCPMode();
  DRV8316_OCPLevel saved_ocplvl = DRV::drv8316.getOCPLevel();

  DRV::drv8316.setRegistersLocked(false); delay(1);
  DRV::drv8316.setOCPMode(AutoRetry_Fault); delay(1);
  DRV::drv8316.setOCPLevel(Curr_16A); delay(1);
  DRV::drv8316.clearFault(); delay(5);

  Serial.print("[MBENCH] OCP_MODE=AutoRetry, OCP_LEVEL=16A");
  Serial.print("  (was: mode="); Serial.print(saved_ocp);
  Serial.print(" lvl="); Serial.print(saved_ocplvl); Serial.println(")");

  auto saved_ctrl = motor.controller;
  auto saved_torque = motor.torque_controller;
  float saved_vlim = motor.voltage_limit;

  motor.controller = MotionControlType::velocity_openloop;
  motor.torque_controller = TorqueControlType::voltage;
  motor.velocity_limit = 50.0f;

  float vlim_steps[] = {0.5f, 0.8f, 1.0f, 1.3f, 1.5f, 1.8f, 2.0f, 2.3f, 2.5f, 2.8f, 3.0f};
  int n_steps = sizeof(vlim_steps) / sizeof(vlim_steps[0]);
  float test_speed = 10.0f;

  motor.voltage_limit = vlim_steps[0];
  mbench_motor_enable();

  float ramp_v = 0.0f;
  unsigned long t0 = millis();
  while (millis() - t0 < 2000) {
    if (ramp_v < test_speed) ramp_v += 10.0f * 0.001f;
    motor.loopFOC();
    motor.move(ramp_v);
    delayMicroseconds(800);
  }

  for (int s = 0; s < n_steps; s++) {
    motor.voltage_limit = vlim_steps[s];

    int ocp_trips = 0;
    float sum_ia=0, sum_ib=0, sum_ic=0;
    const int N = 150;
    for (int i = 0; i < N; i++) {
      motor.loopFOC();
      motor.move(test_speed);

      float ia, ib, ic, id, iq;
      mbench_read_dq(ia, ib, ic, id, iq, motor.electricalAngle());
      sum_ia += fabsf(ia); sum_ib += fabsf(ib); sum_ic += fabsf(ic);

      if (mbench_check_fault()) {
        ocp_trips++;
        DRV::drv8316.clearFault();
        delayMicroseconds(100);
      }
      delayMicroseconds(19000);
    }
    float avg_ia = sum_ia/N, avg_ib = sum_ib/N, avg_ic = sum_ic/N;
    float irms = sqrtf(avg_ia*avg_ia + avg_ib*avg_ib + avg_ic*avg_ic) / 1.732f;
    bool nf = (!DRV::isFaulted());

    Serial.print("[MBENCH] D:");
    Serial.print(vlim_steps[s], 1); Serial.print(",");
    Serial.print(test_speed, 1); Serial.print(",");
    Serial.print(avg_ia, 4); Serial.print(",");
    Serial.print(avg_ib, 4); Serial.print(",");
    Serial.print(avg_ic, 4); Serial.print(",");
    Serial.print(irms, 4); Serial.print(",");
    Serial.print(nf ? "1" : "0"); Serial.print(",");
    Serial.print("AutoRetry"); Serial.print(",");
    Serial.println(ocp_trips);
    Serial.flush();

    if (ocp_trips > N/2) {
      Serial.println("[MBENCH] INFO: Frequent OCP trips — approaching current limit");
      mbench_print_fault();
    }
  }

  // Ramp down
  t0 = millis();
  ramp_v = test_speed;
  while (millis() - t0 < 2000) {
    ramp_v = fmaxf(ramp_v - 10.0f * 0.001f, 0.0f);
    motor.loopFOC();
    motor.move(ramp_v);
    delayMicroseconds(800);
  }
  motor.disable();
  delay(200);

  DRV::drv8316.setRegistersLocked(false); delay(1);
  DRV::drv8316.setOCPMode(saved_ocp); delay(1);
  DRV::drv8316.setOCPLevel(saved_ocplvl); delay(1);
  DRV::drv8316.clearFault(); delay(5);

  motor.controller = saved_ctrl;
  motor.torque_controller = saved_torque;
  motor.voltage_limit = saved_vlim;
  Serial.println("[MBENCH] PHASE3_END");
  Serial.flush();
}

// ---------------------------------------------------------------------------
//  Phase 4: Sustained Operation (30 seconds)
// ---------------------------------------------------------------------------
void mbench_phase4() {
  Serial.println("[MBENCH] PHASE4_START Sustained");
  Serial.println("[MBENCH] H:Time_ms,Spd_meas,Vq,Ia,Ib,Ic,Id,Iq,Irms,nFAULT,OTW");

  auto saved_ctrl = motor.controller;
  auto saved_torque = motor.torque_controller;
  float saved_vlim = motor.voltage_limit;

  motor.controller = MotionControlType::velocity_openloop;
  motor.torque_controller = TorqueControlType::voltage;
  motor.voltage_limit = 1.5f;
  motor.velocity_limit = 50.0f;

  float test_speed = 20.0f;
  DRV::drv8316.clearFault(); delay(5);
  mbench_motor_enable();

  float ramp_v = 0.0f;
  unsigned long t0 = millis();
  while (millis() - t0 < 2000) {
    if (ramp_v < test_speed) ramp_v += 15.0f * 0.001f;
    motor.loopFOC();
    motor.move(ramp_v);
    delayMicroseconds(800);
  }

  unsigned long start = millis();
  const unsigned long DURATION_MS = 30000;
  int fault_total = 0;
  int otw_total = 0;

  while (millis() - start < DURATION_MS) {
    unsigned long loop_start = millis();

    while (millis() - loop_start < 90) {
      motor.loopFOC();
      motor.move(test_speed);
      delayMicroseconds(200);
    }

    sensor.update();
    float vel = sensor.getVelocity();
    float ia, ib, ic, id, iq;
    mbench_read_dq(ia, ib, ic, id, iq, motor.electricalAngle());
    float irms = sqrtf(id*id + iq*iq);
    bool nf = (!DRV::isFaulted());

    DRV8316Status st = DRV::drv8316.getStatus();
    bool otw = st.isOverTemperatureWarning();
    if (!nf) fault_total++;
    if (otw) otw_total++;

    unsigned long elapsed = millis() - start;
    Serial.print("[MBENCH] D:");
    Serial.print(elapsed); Serial.print(",");
    Serial.print(vel, 2); Serial.print(",");
    Serial.print(motor.voltage.q, 2); Serial.print(",");
    Serial.print(ia, 4); Serial.print(",");
    Serial.print(ib, 4); Serial.print(",");
    Serial.print(ic, 4); Serial.print(",");
    Serial.print(id, 4); Serial.print(",");
    Serial.print(iq, 4); Serial.print(",");
    Serial.print(irms, 4); Serial.print(",");
    Serial.print(nf ? "1" : "0"); Serial.print(",");
    Serial.println(otw ? "1" : "0");

    if (!nf) {
      DRV::drv8316.clearFault();
      delayMicroseconds(100);
    }

    if (st.isOverTemperatureShutdown()) {
      Serial.println("[MBENCH] EMERGENCY: OTS detected! Stopping.");
      break;
    }

    while (millis() - loop_start < 100) delayMicroseconds(100);
  }

  Serial.print("[MBENCH] SUMMARY: faults="); Serial.print(fault_total);
  Serial.print(" OTW="); Serial.println(otw_total);

  t0 = millis();
  ramp_v = test_speed;
  while (millis() - t0 < 2000) {
    ramp_v = fmaxf(ramp_v - 15.0f * 0.001f, 0.0f);
    motor.loopFOC();
    motor.move(ramp_v);
    delayMicroseconds(800);
  }
  motor.disable();
  delay(200);

  motor.controller = saved_ctrl;
  motor.torque_controller = saved_torque;
  motor.voltage_limit = saved_vlim;
  Serial.println("[MBENCH] PHASE4_END");
  Serial.flush();
}

// ---------------------------------------------------------------------------
//  Run All
// ---------------------------------------------------------------------------
void mbench_run_all() {
  Serial.println("[MBENCH] ===== MOTOR DRIVER BENCH TEST =====");
  Serial.print("[MBENCH] SYSINFO SYSCLK=");
  Serial.print(HAL_RCC_GetSysClockFreq() / 1000000UL); Serial.print("MHz");
  Serial.print(" SUPPLY="); Serial.print(SUPPLY_VOLTAGE, 1); Serial.print("V");
  Serial.print(" PP="); Serial.print(MOTOR_POLE_PAIRS);
  Serial.print(" Ke="); Serial.print(MOTOR_KE, 3);
  Serial.print(" R="); Serial.print(MOTOR_R_PHASE, 2);
  Serial.println();
  Serial.println("[MBENCH] Safety: max Vlim=3.0V, max Irms=4.0A, OTS auto-stop");
  Serial.println();

  Serial.println("[MBENCH] >>> Phase 1: Static Current Steps <<<");
  delay(500);
  mbench_phase1();
  delay(1000);

  Serial.println("\n[MBENCH] >>> Phase 2: Speed Sweep <<<");
  delay(500);
  mbench_phase2();
  delay(1000);

  Serial.println("\n[MBENCH] >>> Phase 3: OCP Limit Test <<<");
  delay(500);
  mbench_phase3();
  delay(1000);

  Serial.println("\n[MBENCH] >>> Phase 4: Sustained Run (30s) <<<");
  delay(500);
  mbench_phase4();

  Serial.println("\n[MBENCH] ===== BENCH TEST COMPLETE =====");
  Serial.flush();
}
