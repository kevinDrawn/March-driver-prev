#ifndef IMPEDANCE_CONTROLLER_H
#define IMPEDANCE_CONTROLLER_H

#include <Arduino.h>
#include <SimpleFOC.h>

/**
 * Impedance & Admittance Controller
 *
 * Implements two complementary compliance control strategies on top of
 * SimpleFOC's torque (FOC current) control loop.
 *
 * ── Impedance Control ──
 *   Motor acts as a virtual spring-damper system:
 *     τ = K × (θ_ref - θ) + B × (ω_ref - ω) + τ_ff
 *   Input:  desired position/velocity (reference trajectory)
 *   Output: torque command applied to motor
 *   Feel:   "springy" — pushes back to reference position
 *   Use:    Haptic feedback, compliant robot joints, force control
 *
 * ── Admittance Control ──
 *   Motor follows external force by simulating virtual dynamics:
 *     M × ẍ + B × ẋ + K × x = τ_ext
 *   Input:  measured external torque (estimated from motor current)
 *   Output: position/velocity command applied to motor
 *   Feel:   "floaty" — moves in the direction you push
 *   Use:    Collaborative robots, gravity compensation, teach-by-hand
 *
 * Both controllers output to the motor's torque loop (Iq target in Amps).
 */
class ImpedanceController {
public:
  // --- Impedance parameters ---
  float K = 2.0f;      // [Nm/rad] Stiffness (spring constant)
  float B = 0.1f;      // [Nm/(rad/s)] Damping coefficient
  float tau_ff = 0.0f; // [Nm] Feedforward torque (e.g., gravity compensation)

  // --- Admittance parameters ---
  float M_virtual = 0.01f; // [kg·m²] Virtual inertia (mass)
  float B_virtual = 0.5f;  // [Nm/(rad/s)] Virtual damping
  float K_virtual = 0.0f;  // [Nm/rad] Virtual spring (0 = free-floating)

  // --- Torque estimation ---
  float Kt = 0.01f; // [Nm/A] Torque constant (from MotorID or manual)

  // --- Admittance state ---
  float adm_velocity = 0.0f; // Virtual velocity state [rad/s]
  float adm_position = 0.0f; // Virtual position state [rad]

  /**
   * Compute impedance control output.
   *
   * @param theta_ref   Desired position [rad]
   * @param omega_ref   Desired velocity [rad/s]  (typically 0 for a setpoint)
   * @param theta       Actual position [rad]
   * @param omega       Actual velocity [rad/s]
   * @return Torque command [Nm], to be converted to Iq = τ / Kt
   */
  float computeImpedance(float theta_ref, float omega_ref, float theta,
                         float omega);

  /**
   * Compute admittance control output.
   * Updates internal velocity/position states based on virtual dynamics.
   *
   * @param iq_measured   Measured Iq current [A]
   * @param theta_actual  Actual motor position [rad] (for reference tracking)
   * @param dt            Time step [s]
   * @return Target position [rad] for the motor's position controller
   */
  float computeAdmittance(float iq_measured, float theta_actual, float dt);

  /**
   * Reset admittance integrator states.
   * Call this when switching to admittance mode or after a large disturbance.
   */
  void resetAdmittance(float initial_position = 0.0f);

  /**
   * Print current parameters to Serial.
   */
  void printParams();
};

#endif // IMPEDANCE_CONTROLLER_H
