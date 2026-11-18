#ifndef TRAJECTORY_GEN_H
#define TRAJECTORY_GEN_H

#include <Arduino.h>

/**
 * Trajectory Generator — Built-in waveform presets + user parameters
 *
 * Generates time-varying target values for performance testing.
 * When active, overrides the manual 'T' target in the main loop.
 *
 * Presets:
 *   0 = OFF (manual target via T command)
 *   1 = SINE      A × sin(2π × f × t) + offset
 *   2 = RAMP      offset ± A  (triangle wave at frequency f)
 *   3 = STEP      square wave: toggles between offset±A at frequency f
 *   4 = CHIRP     sine with linearly increasing frequency (f → f_end)
 *
 * Usage (Commander):
 *   X0 = off, X1 = sine, X2 = ramp, X3 = step, X4 = chirp
 *   x<val> = amplitude, y<val> = frequency, z<val> = offset
 */
class TrajectoryGen {
public:
  enum WaveType : uint8_t {
    WAVE_OFF = 0,
    WAVE_SINE = 1,
    WAVE_RAMP = 2,
    WAVE_STEP = 3,
    WAVE_CHIRP = 4
  };

  // --- Parameters (user-configurable via Commander) ---
  WaveType type = WAVE_OFF;
  float amplitude = 1.0f;    // [units depend on mode: rad, rad/s, A]
  float frequency = 1.0f;    // [Hz]
  float offset = 0.0f;       // DC offset added to waveform
  float chirp_f_end = 10.0f; // [Hz] end frequency for chirp mode

  // --- State ---
  bool active() const { return type != WAVE_OFF; }

  /**
   * Start the trajectory (resets internal phase timer).
   */
  void start();

  /**
   * Stop the trajectory and return to manual target mode.
   */
  void stop();

  /**
   * Compute the current target value based on elapsed time.
   * Call every loop iteration. Returns the generated target value.
   *
   * @param manual_target  The user's manual 'T' target (returned if OFF)
   * @return Generated target value, or manual_target if trajectory is OFF
   */
  float compute(float manual_target);

  /**
   * Print current trajectory settings.
   */
  void printState();

private:
  unsigned long _start_us = 0;
  float _elapsed_s = 0.0f;
};

#endif // TRAJECTORY_GEN_H
