#include "AutoTune.h"

// =============================================================================
//  Auto PID Tuning — Relay Feedback Implementation
// =============================================================================

void AutoTune::tuneCurrentLoop(BLDCMotor &motor, CurrentSense &cs,
                               bool apply) {
  Serial.println("\n============================================");
  Serial.println("  Auto-Tune: Current Loop (Iq)");
  Serial.println("============================================");
  Serial.print("[AutoTune] Relay amplitude: ");
  Serial.print(relay_amplitude, 2);
  Serial.println(" V");
  Serial.print("[AutoTune] Cycles to measure: ");
  Serial.println(num_cycles);

  // We operate the motor in voltage mode and observe current response
  MotionControlType prevMode = motor.controller;
  motor.controller = MotionControlType::torque;
  motor.torque_controller = TorqueControlType::voltage;

  // Variables for relay feedback
  float output = relay_amplitude; // Start with positive output
  float setpoint = 0.0f;          // Target current = 0
  float peak_high = 0, peak_low = 0;
  int zero_crossings = 0;
  unsigned long last_crossing_us = 0;
  float period_sum = 0;
  int period_count = 0;
  float amplitude_sum = 0;
  float prev_error = 0;
  bool prev_sign_positive = true;
  float max_val = -1e6f, min_val = 1e6f;

  unsigned long start_ms = millis();
  int target_crossings = num_cycles * 2; // Each cycle has 2 zero crossings

  Serial.println("[AutoTune] Running relay test...");

  while (zero_crossings < target_crossings) {
    // Check timeout
    if (millis() - start_ms > (unsigned long)timeout_ms) {
      Serial.println("[AutoTune] TIMEOUT — oscillation not detected.");
      motor.controller = prevMode;
      return;
    }

    // Apply relay output as voltage
    motor.move(output);
    motor.loopFOC();

    // Read actual Iq (Park-transformed) — available after loopFOC()
    float iq_measured = motor.current.q;

    // Track min/max for amplitude
    if (iq_measured > max_val)
      max_val = iq_measured;
    if (iq_measured < min_val)
      min_val = iq_measured;

    // Relay switching logic
    float error = setpoint - iq_measured;
    bool sign_positive = (error >= 0);

    if (sign_positive != prev_sign_positive) {
      // Zero crossing detected
      unsigned long now_us = micros();
      zero_crossings++;

      if (last_crossing_us > 0) {
        float half_period = (float)(now_us - last_crossing_us) / 1e6f;
        period_sum += half_period;
        period_count++;
      }

      // Record amplitude at crossing
      if (!sign_positive) {
        // Crossed from + to - → was at peak high
        peak_high = max_val;
        max_val = -1e6f;
      } else {
        // Crossed from - to + → was at peak low
        peak_low = min_val;
        min_val = 1e6f;
      }
      amplitude_sum += (peak_high - peak_low);

      last_crossing_us = now_us;

      // Switch relay
      output = sign_positive ? relay_amplitude : -relay_amplitude;
    }

    prev_error = error;
    prev_sign_positive = sign_positive;

    delayMicroseconds(100); // ~10 kHz sampling
  }

  // Calculate results
  if (period_count > 0) {
    Tu = (period_sum / period_count) * 2.0f; // Full period = 2 × half period
    float avg_amplitude = amplitude_sum / (float)(zero_crossings / 2);

    if (avg_amplitude > 0.001f) {
      Ku = (4.0f * relay_amplitude) / (PI * avg_amplitude);
    }

    // Ziegler-Nichols PI rules: Kp = 0.45 * Ku, Ki = 1.2 * Kp / Tu
    Kp = 0.45f * Ku;
    Ki = 1.2f * Kp / Tu;

    printResults("Current (Iq)");

    if (apply) {
      motor.PID_current_q.P = Kp;
      motor.PID_current_q.I = Ki;
      motor.PID_current_d.P = Kp;
      motor.PID_current_d.I = Ki;
      Serial.println("[AutoTune] Applied gains to Iq and Id controllers.");
    }
  } else {
    Serial.println("[AutoTune] ERROR: No valid periods measured.");
  }

  // Restore previous mode
  motor.move(0);
  motor.controller = prevMode;
  motor.torque_controller = TorqueControlType::foc_current;
}

void AutoTune::tuneVelocityLoop(BLDCMotor &motor, bool apply) {
  Serial.println("\n============================================");
  Serial.println("  Auto-Tune: Velocity Loop");
  Serial.println("============================================");
  Serial.print("[AutoTune] Relay amplitude: ");
  Serial.print(relay_amplitude, 2);
  Serial.println(" A");
  Serial.print("[AutoTune] Cycles to measure: ");
  Serial.println(num_cycles);

  // Operate in torque mode, relay on current target
  MotionControlType prevMode = motor.controller;
  motor.controller = MotionControlType::torque;
  motor.torque_controller = TorqueControlType::foc_current;

  float output = relay_amplitude;
  float setpoint = 0.0f;
  int zero_crossings = 0;
  unsigned long last_crossing_us = 0;
  float period_sum = 0;
  int period_count = 0;
  float amplitude_sum = 0;
  bool prev_sign_positive = true;
  float max_val = -1e6f, min_val = 1e6f;

  unsigned long start_ms = millis();
  int target_crossings = num_cycles * 2;

  Serial.println("[AutoTune] Running relay test on velocity...");

  while (zero_crossings < target_crossings) {
    if (millis() - start_ms > (unsigned long)timeout_ms) {
      Serial.println("[AutoTune] TIMEOUT — oscillation not detected.");
      motor.controller = prevMode;
      return;
    }

    motor.loopFOC();
    motor.move(output);

    float vel = motor.shaft_velocity;

    if (vel > max_val)
      max_val = vel;
    if (vel < min_val)
      min_val = vel;

    float error = setpoint - vel;
    bool sign_positive = (error >= 0);

    if (sign_positive != prev_sign_positive) {
      unsigned long now_us = micros();
      zero_crossings++;

      if (last_crossing_us > 0) {
        float half_period = (float)(now_us - last_crossing_us) / 1e6f;
        period_sum += half_period;
        period_count++;
      }

      if (!sign_positive) {
        amplitude_sum += max_val;
        max_val = -1e6f;
      } else {
        amplitude_sum += fabs(min_val);
        min_val = 1e6f;
      }

      last_crossing_us = now_us;
      output = sign_positive ? relay_amplitude : -relay_amplitude;
    }

    prev_sign_positive = sign_positive;
    delayMicroseconds(500); // ~2 kHz (velocity loop is slower)
  }

  if (period_count > 0) {
    Tu = (period_sum / period_count) * 2.0f;
    float avg_amplitude = amplitude_sum / (float)(zero_crossings);

    if (avg_amplitude > 0.01f) {
      Ku = (4.0f * relay_amplitude) / (PI * avg_amplitude);
    }

    Kp = 0.45f * Ku;
    Ki = 1.2f * Kp / Tu;

    printResults("Velocity");

    if (apply) {
      motor.PID_velocity.P = Kp;
      motor.PID_velocity.I = Ki;
      Serial.println("[AutoTune] Applied gains to velocity controller.");
    }
  } else {
    Serial.println("[AutoTune] ERROR: No valid periods measured.");
  }

  motor.move(0);
  motor.controller = prevMode;
}

void AutoTune::printResults(const char *loopName) {
  Serial.print("\n--- Auto-Tune Results: ");
  Serial.print(loopName);
  Serial.println(" ---");
  Serial.print("  Ultimate Gain (Ku): ");
  Serial.println(Ku, 4);
  Serial.print("  Ultimate Period (Tu): ");
  Serial.print(Tu * 1000.0f, 2);
  Serial.println(" ms");
  Serial.print("  Suggested Kp: ");
  Serial.println(Kp, 4);
  Serial.print("  Suggested Ki: ");
  Serial.println(Ki, 2);
  Serial.println("---");
}
