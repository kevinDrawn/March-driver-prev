#include "MotorID.h"
#include "../config.h"  // for PIN_DRVOFF

// =============================================================================
//  Motor Parameter Identification — Implementation
// =============================================================================

void MotorID::measureResistance(BLDCMotor &motor, CurrentSense &cs,
                                BLDCDriver &driver) {
  Serial.println("\n[MotorID] === Measuring Phase Resistance ===");
  Serial.print("[MotorID] Test voltage: ");
  Serial.print(test_voltage, 2);
  Serial.println(" V");

  // Disable FOC control temporarily
  motor.disable();
  delay(100);

  // ★ Ensure DRV8316 hardware output is enabled
  #ifdef PIN_DRVOFF
  digitalWrite(PIN_DRVOFF, LOW);
  #endif
  delay(10);

  // Apply DC voltage: Phase A high, Phase B low, Phase C floating
  // This creates a current path through 2 phase windings in series
  driver.enable();
  driver.setPwm(
      driver.voltage_power_supply / 2.0f + test_voltage / 2.0f, // Phase A
      driver.voltage_power_supply / 2.0f - test_voltage / 2.0f, // Phase B
      driver.voltage_power_supply / 2.0f // Phase C (mid-point)
  );

  // Wait for current to settle (several L/R time constants)
  Serial.println("[MotorID] Waiting for current to settle...");
  delay((unsigned long)test_duration_ms);

  // Sample current multiple times and average
  float sum_current = 0;
  const int NUM_SAMPLES = 100;
  for (int i = 0; i < NUM_SAMPLES; i++) {
    PhaseCurrent_s currents = cs.getPhaseCurrents();
    sum_current += fabs(currents.a);
    delayMicroseconds(500);
  }
  float avg_current = sum_current / NUM_SAMPLES;

  // Turn off
  driver.setPwm(driver.voltage_power_supply / 2.0f,
                driver.voltage_power_supply / 2.0f,
                driver.voltage_power_supply / 2.0f);
  driver.disable();

  if (avg_current > 0.01f) {
    // V = I * 2R (two phases in series) → R = V / (2*I)
    phase_resistance = test_voltage / (2.0f * avg_current);
    Serial.print("[MotorID] Measured current: ");
    Serial.print(avg_current, 4);
    Serial.println(" A");
    Serial.print("[MotorID] Phase Resistance R = ");
    Serial.print(phase_resistance, 4);
    Serial.println(" Ohm");
  } else {
    Serial.println("[MotorID] ERROR: No current detected. Check wiring!");
    phase_resistance = 0;
  }
}

void MotorID::measureInductance(BLDCMotor &motor, CurrentSense &cs,
                                BLDCDriver &driver) {
  Serial.println("\n[MotorID] === Measuring Phase Inductance ===");

  if (phase_resistance < 0.001f) {
    Serial.println("[MotorID] R not measured yet — measuring first...");
    measureResistance(motor, cs, driver);
    if (phase_resistance < 0.001f) {
      Serial.println("[MotorID] ERROR: Cannot measure L without valid R.");
      return;
    }
  }

  motor.disable();
  delay(100);

  // ★ Ensure DRV8316 hardware output is enabled
  #ifdef PIN_DRVOFF
  digitalWrite(PIN_DRVOFF, LOW);
  #endif
  delay(10);

  // Inductance measurement via current rise time
  // Apply voltage step and measure time to reach 63.2% of steady-state current
  // τ = L/R → L = τ * R

  float I_steady =
      test_voltage / (2.0f * phase_resistance); // Expected steady-state
  float I_target = I_steady * 0.632f;           // 63.2% = one time constant

  Serial.print("[MotorID] Expected I_steady: ");
  Serial.print(I_steady, 4);
  Serial.println(" A");
  Serial.print("[MotorID] Target I (63.2%%): ");
  Serial.print(I_target, 4);
  Serial.println(" A");

  driver.enable();

  // Start with zero voltage
  driver.setPwm(driver.voltage_power_supply / 2.0f,
                driver.voltage_power_supply / 2.0f,
                driver.voltage_power_supply / 2.0f);
  delay(50);

  // Record start time, apply voltage step
  unsigned long t_start = micros();
  driver.setPwm(driver.voltage_power_supply / 2.0f + test_voltage / 2.0f,
                driver.voltage_power_supply / 2.0f - test_voltage / 2.0f,
                driver.voltage_power_supply / 2.0f);

  // Sample current rapidly until it reaches 63.2% target
  unsigned long t_tau = 0;
  bool found = false;
  for (int i = 0; i < 10000; i++) {
    PhaseCurrent_s currents = cs.getPhaseCurrents();
    float I_now = fabs(currents.a);
    if (I_now >= I_target && !found) {
      t_tau = micros() - t_start;
      found = true;
      break;
    }
    delayMicroseconds(5); // 5μs sampling → 200 kHz
  }

  // Turn off
  driver.setPwm(driver.voltage_power_supply / 2.0f,
                driver.voltage_power_supply / 2.0f,
                driver.voltage_power_supply / 2.0f);
  driver.disable();

  if (found && t_tau > 0) {
    float tau_seconds = (float)t_tau / 1e6f;
    // L = τ × 2R (two phases in series)
    phase_inductance = tau_seconds * 2.0f * phase_resistance;
    Serial.print("[MotorID] Time constant τ = ");
    Serial.print(t_tau);
    Serial.println(" μs");
    Serial.print("[MotorID] Phase Inductance L = ");
    Serial.print(phase_inductance * 1000.0f, 4);
    Serial.println(" mH");
  } else {
    Serial.println("[MotorID] WARNING: Could not detect time constant.");
    Serial.println("[MotorID] Current may have risen too fast (L very small).");
    // Estimate from typical BLDC: L ≈ 10-100 μH for small motors
    phase_inductance = 0.0001f; // 100 μH default
    Serial.println("[MotorID] Using default estimate: L = 0.1 mH");
  }
}

void MotorID::measureKe(BLDCMotor &motor, Sensor &sensor, BLDCDriver &driver) {
  Serial.println("\n[MotorID] === Measuring Back-EMF Constant (Ke) ===");
  Serial.println("[MotorID] Dual-point method: cancels I*R drop");

  // ★ MUST use voltage torque mode for open-loop!
  auto saved_torque = motor.torque_controller;
  auto saved_vlim = motor.voltage_limit;
  motor.torque_controller = TorqueControlType::voltage;
  motor.controller = MotionControlType::velocity_openloop;

  #ifdef PIN_DRVOFF
  digitalWrite(PIN_DRVOFF, LOW);
  #endif
  delay(10);

  // --- Helper lambda: spin at given voltage+speed, return avg velocity ---
  auto measurePoint = [&](float voltage, float speed, const char* label) -> float {
    motor.voltage_limit = voltage;
    motor.enable();

    // Spin up for 2 seconds
    Serial.print("[MotorID] Point ");
    Serial.print(label);
    Serial.print(": V="); Serial.print(voltage, 1);
    Serial.print("V, cmd="); Serial.print(speed, 1);
    Serial.println(" rad/s");

    unsigned long t = millis();
    while (millis() - t < 2000) {
      motor.loopFOC();
      motor.move(speed);
    }

    // Average velocity over 500ms
    float vel_sum = 0;
    int cnt = 0;
    t = millis();
    while (millis() - t < 500) {
      motor.loopFOC();
      motor.move(speed);
      sensor.update();
      vel_sum += sensor.getVelocity();
      cnt++;
      delayMicroseconds(100);
    }
    float avg_vel = (cnt > 0) ? vel_sum / cnt : 0;
    Serial.print("[MotorID]   → measured: ");
    Serial.print(avg_vel, 2);
    Serial.print(" rad/s (");
    Serial.print(cnt);
    Serial.println(" samples)");

    motor.move(0);
    motor.loopFOC();
    return avg_vel;
  };

  // Point 1: low speed
  float V1 = 1.5f;
  float speed1 = 5.0f;
  float w1 = measurePoint(V1, speed1, "1");

  // Brief pause between points
  delay(500);

  // Point 2: high speed
  float V2 = 3.5f;
  float speed2 = 20.0f;
  float w2 = measurePoint(V2, speed2, "2");

  // Calculate Ke using dual-point method
  // V1 = Ke * w1 + I_f * R
  // V2 = Ke * w2 + I_f * R
  // Ke = (V2 - V1) / (w2 - w1)   ← I*R cancels out!
  float dV = V2 - V1;
  float dw = fabs(w2) - fabs(w1);

  Serial.print("[MotorID] dV = "); Serial.print(dV, 3);
  Serial.print("V, dw = "); Serial.print(dw, 2);
  Serial.println(" rad/s");

  if (fabs(dw) > 1.0f) {
    Ke = dV / fabs(dw);
    Kt = Ke;

    // Also estimate friction current from either point
    float I_friction = (V1 - Ke * fabs(w1)) / phase_resistance;

    Serial.print("[MotorID] Ke (Back-EMF) = ");
    Serial.print(Ke, 5);
    Serial.println(" V/(rad/s)");
    Serial.print("[MotorID] Kt (Torque)   = ");
    Serial.print(Kt, 5);
    Serial.println(" Nm/A");
    Serial.print("[MotorID] I_friction ≈ ");
    Serial.print(I_friction, 2);
    Serial.println(" A");
  } else {
    Serial.println("[MotorID] ERROR: Speed difference too small for Ke calculation.");
    Serial.println("[MotorID] Motor may not be tracking open-loop speed.");
  }

  // Stop and restore
  motor.move(0);
  delay(500);
  motor.disable();
  motor.torque_controller = saved_torque;
  motor.voltage_limit = saved_vlim;
}

void MotorID::runFullID(BLDCMotor &motor, Sensor &sensor,
                        CurrentSense &cs, BLDCDriver &driver) {
  Serial.println("\n============================================");
  Serial.println("  Motor Parameter Identification — START");
  Serial.println("============================================");

  measureResistance(motor, cs, driver);
  delay(500);
  measureInductance(motor, cs, driver);
  delay(500);
  measureKe(motor, sensor, driver);

  valid = (phase_resistance > 0.001f && phase_inductance > 0.0f);

  Serial.println("\n============================================");
  Serial.println("  Motor Parameter Identification — COMPLETE");
  Serial.println("============================================");
  printResults();
  if (valid)
    suggestPIDGains();
}

void MotorID::printResults() {
  Serial.println("\n--- Motor ID Results ---");
  Serial.print("  R (phase):  ");
  Serial.print(phase_resistance, 4);
  Serial.println(" Ohm");
  Serial.print("  L (phase):  ");
  Serial.print(phase_inductance * 1000.0f, 4);
  Serial.println(" mH");
  Serial.print("  Ke:         ");
  Serial.print(Ke, 5);
  Serial.println(" V/(rad/s)");
  Serial.print("  Kt:         ");
  Serial.print(Kt, 5);
  Serial.println(" Nm/A");
  Serial.print("  Valid:      ");
  Serial.println(valid ? "YES" : "NO");
  Serial.println("---");
}

void MotorID::suggestPIDGains() {
  if (!valid || phase_resistance < 0.001f || phase_inductance < 1e-7f) {
    Serial.println("[MotorID] Cannot suggest gains — invalid parameters.");
    return;
  }

  // Current loop bandwidth: ω_bw = R / L
  float bw_current = phase_resistance / phase_inductance; // rad/s
  float bw_current_hz = bw_current / (2.0f * PI);

  // PI gains for current loop (pole-placement):
  //   P = L × ω_bw_target
  //   I = R × ω_bw_target
  // Target bandwidth: 1/5 of electrical bandwidth (conservative)
  float target_bw = bw_current * 0.2f;

  float suggested_Kp_current = phase_inductance * target_bw;
  float suggested_Ki_current = phase_resistance * target_bw;

  Serial.println("\n--- Suggested PID Gains ---");
  Serial.print("  Electrical bandwidth: ");
  Serial.print(bw_current_hz, 1);
  Serial.println(" Hz");
  Serial.print("  Target current BW:    ");
  Serial.print(target_bw / (2.0f * PI), 1);
  Serial.println(" Hz");
  Serial.println("  Current PID:");
  Serial.print("    P = ");
  Serial.println(suggested_Kp_current, 4);
  Serial.print("    I = ");
  Serial.println(suggested_Ki_current, 2);
  Serial.println("  Velocity PID:");
  Serial.println("    Start with P=0.2, I=20 and tune up.");
  Serial.println("  To apply: Q<P> q<I>");
  Serial.println("---");
}

void MotorID::applyToMotor(BLDCMotor &motor) {
  if (!valid) {
    Serial.println("[MotorID] Cannot apply — parameters not valid.");
    return;
  }

  // Write R and L to motor object (SimpleFOC uses these internally)
  motor.phase_resistance = phase_resistance;
  motor.phase_inductance = phase_inductance;

  // Set axis inductances (Ld ≈ Lq for surface-mount PMSM)
  // This activates SimpleFOC's built-in DQ cross-coupling compensation:
  //   Vd -= ω × Lq × Iq  (d-axis lag compensation)
  //   Vq += ω × Ld × Id  (q-axis cross-coupling compensation)
  motor.axis_inductance = {phase_inductance, phase_inductance};

  // Set KV_rating if Ke is available (for back-EMF estimation)
  if (Ke > 0.001f) {
    // KV = 1/Ke in (rad/s)/V, SimpleFOC uses KV_rating
    motor.KV_rating = 1.0f / Ke;
  }

  Serial.println("[MotorID] Applied to motor object:");
  Serial.print("  motor.phase_resistance = ");
  Serial.print(phase_resistance, 4);
  Serial.println(" Ohm");
  Serial.print("  motor.phase_inductance = ");
  Serial.print(phase_inductance * 1000.0f, 4);
  Serial.println(" mH");
  Serial.print("  motor.axis_inductance  = {");
  Serial.print(phase_inductance * 1000.0f, 4);
  Serial.print(", ");
  Serial.print(phase_inductance * 1000.0f, 4);
  Serial.println("} mH");
  Serial.println("[MotorID] DQ cross-coupling compensation → ENABLED");
}
