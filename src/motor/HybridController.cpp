#include "HybridController.h"

// =============================================================================
//  Hybrid Impedance/Admittance Controller — Implementation
// =============================================================================

void HybridController::update(BLDCMotor &motor, ImpedanceController &ic,
                              float target_position, float iq_measured,
                              float dt) {

  float abs_iq = fabs(iq_measured);

  switch (phase) {

  // =====================================================================
  //  Phase 1: APPROACH — Impedance control (virtual spring toward target)
  //  Motor in torque (FOC current) mode.
  //  τ = K×(θ_ref − θ) + B×(0 − ω)
  // =====================================================================
  case PHASE_APPROACH: {
    // Ensure motor is in torque mode
    if (motor.controller != MotionControlType::torque) {
      motor.controller = MotionControlType::torque;
      motor.torque_controller = TorqueControlType::foc_current;
    }

    // Compute impedance torque: virtual spring pulling toward target
    float torque_nm = ic.computeImpedance(
        target_position, 0.0f, motor.shaft_angle, motor.shaft_velocity);

    // Convert Nm → Amps and clamp to approach limits
    float iq_target = 0.0f;
    if (ic.Kt > 1e-6f) {
      iq_target = torque_nm / ic.Kt;
    }

    // Clamp velocity via current limit proportional to approach_velocity
    float speed_error = fabs(motor.shaft_velocity) - approach_velocity;
    if (speed_error > 0.0f) {
      // Reduce current to limit velocity (soft clamp)
      float reduction = speed_error * 0.5f; // damping factor
      if (iq_target > 0)
        iq_target = fmax(0.0f, iq_target - reduction);
      else
        iq_target = fmin(0.0f, iq_target + reduction);
    }

    // Hard clamp to current limit
    iq_target =
        _constrain(iq_target, -motor.current_limit, motor.current_limit);
    motor.move(iq_target);

    // Check for contact: |Iq| > threshold
    if (abs_iq > contact_threshold) {
      // Transition to CONTACT phase (admittance)
      phase = PHASE_CONTACT;
      _contact_time_ms = millis();

      // Initialize admittance at current position
      ic.resetAdmittance(motor.shaft_angle);

      // Switch to position mode for admittance output
      motor.controller = MotionControlType::angle;

      Serial.print("[Hybrid] CONTACT! Iq=");
      Serial.print(iq_measured, 3);
      Serial.println("A → Admittance mode");
    }
    break;
  }

  // =====================================================================
  //  Phase 2: CONTACT — Admittance control (compliant force response)
  //  Motor in position (angle) mode.
  //  Senses Iq (external force) → virtual dynamics → position output.
  // =====================================================================
  case PHASE_CONTACT: {
    // Ensure motor is in position mode
    if (motor.controller != MotionControlType::angle) {
      motor.controller = MotionControlType::angle;
    }

    // Compute admittance: Iq → virtual dynamics → position target
    float target_pos = ic.computeAdmittance(iq_measured, motor.shaft_angle, dt);
    motor.move(target_pos);

    // Check for release: |Iq| < release_threshold AND dwell elapsed
    unsigned long elapsed = millis() - _contact_time_ms;
    if (abs_iq < release_threshold &&
        elapsed > (unsigned long)contact_dwell_ms) {
      // Transition back to APPROACH phase (impedance)
      phase = PHASE_APPROACH;

      // Switch back to torque mode for impedance
      motor.controller = MotionControlType::torque;
      motor.torque_controller = TorqueControlType::foc_current;

      Serial.println("[Hybrid] RELEASE → Impedance approach");
    }
    break;
  }
  }
}

void HybridController::reset() {
  phase = PHASE_APPROACH;
  _contact_time_ms = 0;
  Serial.println("[Hybrid] Reset → APPROACH (impedance)");
}

void HybridController::printState() {
  Serial.println("\n--- Hybrid Impedance/Admittance ---");
  Serial.print("  Phase: ");
  Serial.println(phase == PHASE_APPROACH ? "APPROACH (impedance)"
                                         : "CONTACT (admittance)");
  Serial.print("  approach_velocity: ");
  Serial.print(approach_velocity, 2);
  Serial.println(" rad/s");
  Serial.print("  contact_threshold: ");
  Serial.print(contact_threshold, 3);
  Serial.println(" A");
  Serial.print("  release_threshold: ");
  Serial.print(release_threshold, 3);
  Serial.println(" A");
  Serial.print("  contact_dwell:     ");
  Serial.print(contact_dwell_ms, 0);
  Serial.println(" ms");
  Serial.println("---");
}
