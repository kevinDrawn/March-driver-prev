#include "ImpedanceController.h"

// =============================================================================
//  Impedance & Admittance Controller — Implementation
// =============================================================================

float ImpedanceController::computeImpedance(float theta_ref, float omega_ref,
                                            float theta, float omega) {
  // Impedance control law:
  //   τ = K × (θ_ref - θ) + B × (ω_ref - ω) + τ_ff
  //
  // - K term: spring force pulling toward reference position
  // - B term: damping force opposing velocity deviation
  // - τ_ff:   feedforward (gravity compensation, friction, etc.)

  float position_error = theta_ref - theta;
  float velocity_error = omega_ref - omega;

  float torque = K * position_error + B * velocity_error + tau_ff;

  return torque;
}

float ImpedanceController::computeAdmittance(float iq_measured,
                                             float theta_actual, float dt) {
  // Admittance control:
  //   Simulate virtual dynamics: M × ẍ + B × ẋ + K × x = τ_ext
  //
  // 1. Estimate external torque from measured current
  //    τ_ext = Kt × iq  (simplified — assumes quasi-static, ignoring motor
  //    dynamics)
  //
  // 2. Solve virtual dynamics for acceleration
  //    ẍ = (τ_ext - B × ẋ - K × x) / M
  //
  // 3. Integrate to get virtual velocity and position
  //    ẋ = ẋ + ẍ × dt
  //    x = x + ẋ × dt
  //
  // 4. Output virtual position as the position target for the motor

  // Step 1: External torque estimation
  float tau_ext = Kt * iq_measured;

  // Step 2: Virtual dynamics
  float position_error = adm_position - theta_actual;
  float accel = 0.0f;

  if (M_virtual > 1e-6f) {
    accel = (tau_ext - B_virtual * adm_velocity - K_virtual * position_error) /
            M_virtual;
  }

  // Step 3: Integration (semi-implicit Euler for stability)
  adm_velocity += accel * dt;
  adm_position += adm_velocity * dt;

  // Safety: clamp virtual velocity to prevent runaway
  if (adm_velocity > 100.0f)
    adm_velocity = 100.0f;
  if (adm_velocity < -100.0f)
    adm_velocity = -100.0f;

  // Step 4: Return target position
  return adm_position;
}

void ImpedanceController::resetAdmittance(float initial_position) {
  adm_velocity = 0.0f;
  adm_position = initial_position;
  Serial.print("[Impedance] Admittance state reset. Position: ");
  Serial.println(initial_position, 4);
}

void ImpedanceController::printParams() {
  Serial.println("\n--- Impedance/Admittance Parameters ---");
  Serial.println("  [Impedance Mode]");
  Serial.print("    K (stiffness):  ");
  Serial.print(K, 4);
  Serial.println(" Nm/rad");
  Serial.print("    B (damping):    ");
  Serial.print(B, 4);
  Serial.println(" Nm/(rad/s)");
  Serial.print("    tau_ff:         ");
  Serial.print(tau_ff, 4);
  Serial.println(" Nm");
  Serial.println("  [Admittance Mode]");
  Serial.print("    M (virt.mass):  ");
  Serial.print(M_virtual, 4);
  Serial.println(" kg*m^2");
  Serial.print("    B (virt.damp):  ");
  Serial.print(B_virtual, 4);
  Serial.println(" Nm/(rad/s)");
  Serial.print("    K (virt.spring):");
  Serial.print(K_virtual, 4);
  Serial.println(" Nm/rad");
  Serial.print("    Kt:             ");
  Serial.print(Kt, 5);
  Serial.println(" Nm/A");
  Serial.println("---");
}
