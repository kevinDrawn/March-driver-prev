#include "TrajectoryGen.h"

// =============================================================================
//  Trajectory Generator — Implementation
// =============================================================================

void TrajectoryGen::start() {
  _start_us = micros();
  Serial.print("[Traj] Started: type=");
  Serial.print((int)type);
  Serial.print(" A=");
  Serial.print(amplitude, 2);
  Serial.print(" f=");
  Serial.print(frequency, 2);
  Serial.print("Hz off=");
  Serial.println(offset, 2);
}

void TrajectoryGen::stop() {
  type = WAVE_OFF;
  Serial.println("[Traj] Stopped → manual target");
}

float TrajectoryGen::compute(float manual_target) {
  if (type == WAVE_OFF)
    return manual_target;

  // Elapsed time in seconds
  _elapsed_s = (float)(micros() - _start_us) / 1e6f;
  float t = _elapsed_s;
  float phase = frequency * t; // cycles elapsed (not radians)

  switch (type) {

  case WAVE_SINE: {
    // A × sin(2π × f × t) + offset
    return amplitude * sinf(2.0f * PI * phase) + offset;
  }

  case WAVE_RAMP: {
    // Triangle wave: ramps up then down, period = 1/f
    // fmod gives fractional cycle position [0, 1)
    float frac = phase - floorf(phase); // [0, 1)
    float tri;
    if (frac < 0.5f)
      tri = 4.0f * frac - 1.0f; // -1 → +1 during first half
    else
      tri = 3.0f - 4.0f * frac; // +1 → -1 during second half
    return amplitude * tri + offset;
  }

  case WAVE_STEP: {
    // Square wave: toggles between offset+A and offset-A
    float frac = phase - floorf(phase);
    return (frac < 0.5f) ? (offset + amplitude) : (offset - amplitude);
  }

  case WAVE_CHIRP: {
    // Linear chirp: frequency sweeps from f_start to f_end over time
    // Instantaneous frequency: f(t) = f_start + (f_end - f_start) × t / T_sweep
    // For continuous chirp, use accumulating phase
    float f_inst =
        frequency + (chirp_f_end - frequency) * fmodf(t * 0.1f, 1.0f);
    return amplitude * sinf(2.0f * PI * f_inst * t) + offset;
  }

  default:
    return manual_target;
  }
}

void TrajectoryGen::printState() {
  const char *names[] = {"OFF", "SINE", "RAMP", "STEP", "CHIRP"};
  Serial.println("\n--- Trajectory Generator ---");
  Serial.print("  Type: ");
  Serial.println(names[type]);
  if (type != WAVE_OFF) {
    Serial.print("  Amplitude: ");
    Serial.println(amplitude, 3);
    Serial.print("  Frequency: ");
    Serial.print(frequency, 3);
    Serial.println(" Hz");
    Serial.print("  Offset:    ");
    Serial.println(offset, 3);
    if (type == WAVE_CHIRP) {
      Serial.print("  Chirp end: ");
      Serial.print(chirp_f_end, 1);
      Serial.println(" Hz");
    }
    Serial.print("  Elapsed:   ");
    Serial.print(_elapsed_s, 2);
    Serial.println(" s");
  }
  Serial.println("---");
}
