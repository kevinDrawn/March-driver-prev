#ifndef HYBRID_CONTROLLER_H
#define HYBRID_CONTROLLER_H

#include "ImpedanceController.h"
#include <Arduino.h>
#include <SimpleFOC.h>

/**
 * Hybrid Impedance/Admittance Controller
 *
 * Two-phase control strategy using ImpedanceController internally:
 *
 *   Phase 1 — APPROACH (Impedance control)
 *     Motor acts as a virtual spring-damper pulling toward the target.
 *     τ = K×(θ_ref − θ) + B×(0 − ω)
 *     Output: torque command (motor in FOC current mode).
 *     Contact detection: when |Iq| > contact_threshold.
 *
 *   Phase 2 — CONTACT (Admittance control)
 *     Motor senses external force (Iq) and responds compliantly.
 *     Virtual dynamics: M×ẍ + B×ẋ + K×x = τ_ext
 *     Output: position command (motor in angle mode).
 *     Release detection: when |Iq| < release_threshold + dwell elapsed.
 *
 * The ImpedanceController reference is passed from main.cpp so that
 * the same K/B/M parameters used in M4/M5 are shared with M6.
 */
class HybridController {
public:
  // --- Contact detection parameters ---

  /** Current threshold to detect contact [A]. */
  float contact_threshold = 0.5f;

  /** Current threshold to detect release [A]. */
  float release_threshold = 0.2f;

  /** Minimum dwell time in contact state before allowing release [ms]. */
  float contact_dwell_ms = 100.0f;

  /** Maximum velocity during impedance approach [rad/s]. */
  float approach_velocity = 5.0f;

  // --- State ---

  enum Phase : uint8_t {
    PHASE_APPROACH = 0, // Impedance: virtual spring toward target
    PHASE_CONTACT = 1   // Admittance: compliant force response
  };

  Phase phase = PHASE_APPROACH;

  /**
   * Main update function — call every loop iteration.
   *
   * APPROACH: Uses impedanceCtrl.computeImpedance() → torque output.
   * CONTACT: Uses impedanceCtrl.computeAdmittance() → position output.
   *
   * @param motor           BLDCMotor reference
   * @param impedanceCtrl   ImpedanceController reference (shared with M4/M5)
   * @param target_position Desired end position [rad]
   * @param iq_measured     Measured Iq current [A] (from FOC)
   * @param dt              Time step [s]
   */
  void update(BLDCMotor &motor, ImpedanceController &impedanceCtrl,
              float target_position, float iq_measured, float dt);

  /** Reset to approach phase. */
  void reset();

  /** Print current state and parameters. */
  void printState();

  /** Saved velocity_limit before M6 entry (restored on exit). */
  float _saved_vel_limit = 0;

private:
  unsigned long _contact_time_ms = 0;
};

#endif // HYBRID_CONTROLLER_H
