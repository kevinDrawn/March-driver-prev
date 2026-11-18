#ifndef AUTO_TUNE_H
#define AUTO_TUNE_H

#include <Arduino.h>
#include <SimpleFOC.h>

/**
 * Auto PID Tuning via Relay Feedback Method
 *
 * Uses a relay (bang-bang) controller to excite the system, then measures
 * the resulting oscillation to estimate the ultimate gain (Ku) and
 * ultimate period (Tu). From these, PID gains are computed using
 * Ziegler-Nichols tuning rules.
 *
 * Supports tuning:
 *   - Current loop (Iq PI controller)
 *   - Velocity loop (PI controller)
 *
 * Usage:
 *   AutoTune tuner;
 *   tuner.tuneCurrentLoop(motor, currentSense);
 *   tuner.tuneVelocityLoop(motor);
 */
class AutoTune {
public:
  // Relay parameters
  float relay_amplitude = 0.5f; // [V] or [A] relay output amplitude
  int num_cycles = 5;           // Number of oscillation cycles to measure
  float timeout_ms = 10000;     // [ms] Max time before giving up

  // Results
  float Ku = 0; // Ultimate gain
  float Tu = 0; // Ultimate period [s]
  float Kp = 0; // Suggested P gain
  float Ki = 0; // Suggested I gain

  /**
   * Tune the current (Iq) control loop.
   * Applies relay control to Vq and measures Iq oscillations.
   * Motor must be enabled and stationary (low speed or locked rotor).
   * After tuning, automatically applies the computed gains.
   *
   * @param motor       BLDCMotor reference
   * @param cs          Current sense reference
   * @param apply       If true, automatically apply computed gains
   */
  void tuneCurrentLoop(BLDCMotor &motor, CurrentSense &cs,
                       bool apply = true);

  /**
   * Tune the velocity control loop.
   * Applies relay control to the velocity target and measures speed
   * oscillations. Current loop must already be tuned and stable. After tuning,
   * automatically applies the computed gains.
   *
   * @param motor       BLDCMotor reference
   * @param apply       If true, automatically apply computed gains
   */
  void tuneVelocityLoop(BLDCMotor &motor, bool apply = true);

  /** Print tuning results. */
  void printResults(const char *loopName);

private:
  /**
   * Core relay feedback algorithm.
   * Returns true if oscillation was detected and measured.
   */
  bool relayTest(float *measurement, float setpoint, float relay_amp,
                 float *out_Ku, float *out_Tu);
};

#endif // AUTO_TUNE_H
