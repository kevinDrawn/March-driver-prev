#ifndef MOTOR_ID_H
#define MOTOR_ID_H

#include <Arduino.h>
#include <SimpleFOC.h>

/**
 * Motor Parameter Identification
 *
 * Measures motor electrical parameters by applying test signals:
 *   - Phase Resistance (R): DC voltage step → steady-state current
 *   - Phase Inductance (L): Voltage step → current rise time constant
 *   - Back-EMF Constant (Ke): Open-loop spin → measured back-EMF
 *
 * Usage:
 *   MotorID motorID;
 *   motorID.measureResistance(motor, currentSense, driver);
 *   motorID.measureInductance(motor, currentSense, driver);
 *   motorID.measureKe(motor, sensor);
 *   motorID.printResults();
 */
class MotorID {
public:
  // Measured parameters
  float phase_resistance = 0.0f; // [Ohm] Phase-to-neutral resistance
  float phase_inductance = 0.0f; // [Henry] Phase inductance
  float Ke = 0.0f;               // [V/(rad/s)] Back-EMF constant
  float Kt = 0.0f;               // [Nm/A] Torque constant (= Ke for BLDC)
  bool valid = false;            // True if identification completed

  // Test parameters (adjustable)
  float test_voltage = 1.0f;     // [V] Voltage used for R/L measurement
  float test_speed = 10.0f;      // [rad/s] Speed for Ke measurement
  float test_duration_ms = 2000; // [ms] Duration for each test phase

  /**
   * Measure phase resistance.
   * Applies a DC voltage between two phases and measures steady-state current.
   * Motor must be stationary. Driver must be initialized.
   */
  void measureResistance(BLDCMotor &motor, CurrentSense &cs,
                         BLDCDriver &driver);

  /**
   * Measure phase inductance.
   * Applies a voltage step and measures the current rise time constant.
   * R must be measured first (calls measureResistance if R == 0).
   */
  void measureInductance(BLDCMotor &motor, CurrentSense &cs,
                         BLDCDriver &driver);

  /**
   * Measure back-EMF constant (Ke).
   * Spins motor in open-loop at known speed, then coasts and measures back-EMF.
   * Requires sensor for speed measurement.
   */
  void measureKe(BLDCMotor &motor, Sensor &sensor, BLDCDriver &driver);

  /**
   * Run full identification sequence (R → L → Ke).
   */
  void runFullID(BLDCMotor &motor, Sensor &sensor, CurrentSense &cs,
                 BLDCDriver &driver);

  /**
   * Print all measured parameters to Serial.
   */
  void printResults();

  /**
   * Suggest PID gains based on measured parameters.
   * Uses pole-placement: bandwidth = R/L, provides starting-point gains.
   */
  void suggestPIDGains();

  /**
   * Apply measured parameters to SimpleFOC motor object.
   * Sets motor.phase_resistance, motor.phase_inductance, motor.axis_inductance,
   * and motor.KV_rating — enabling SimpleFOC's built-in DQ decoupling
   * feedforward compensation.
   */
  void applyToMotor(BLDCMotor &motor);
};

#endif // MOTOR_ID_H
