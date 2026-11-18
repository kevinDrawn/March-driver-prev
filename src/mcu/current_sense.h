// =============================================================================
//  mcu/current_sense.h — ADC Current Sensing Module
//  JSQR patch, offset calibration, ADC register dump
// =============================================================================
#pragma once

#include <Arduino.h>
#include <SimpleFOC.h>
#include "../config.h"

// Forward declaration — actual instance lives in main.cpp or motor_ctrl
extern BLDCDriver6PWM driver6;
extern BLDCDriver3PWM driver3;
extern BLDCMotor motor;

#ifdef USE_H723_BOARD
class H723CurrentSense : public LowsideCurrentSense {
public:
  H723CurrentSense() : LowsideCurrentSense(CSA_GAIN_MV_PER_AMP, PIN_ISENA, PIN_ISENB, PIN_ISENC) {}
  int init() override;
  PhaseCurrent_s getPhaseCurrents() override;
};
#endif
namespace CS {

  /// Initialize current sense and link to driver
  void init();

  /// Patch JSQR register (fix SimpleFOC's ADC channel corruption on G431)
  void patchJSQR();

  /// Recalibrate ADC offsets at zero current (motor must be disabled)
  void calibrateOffsets();

  /// Read phase currents as DQ (requires electrical angle)
  void readDQ(float& id, float& iq, float e_angle);

  /// Read RMS current from phase measurements
  float readIrms();

  /// Dump ADC registers with human-readable decode
  void dumpADCRegisters();

  /// Dump JSQR register with decoded fields
  void dumpJSQR();

  /// Get the current sense object reference
#if defined(USE_H723_BOARD)
  extern H723CurrentSense currentSense;
#elif defined(USE_CUSTOM_BOARD)
  extern LowsideCurrentSense currentSense;
#else
  extern LowsideCurrentSense currentSense;
#endif

} // namespace CS
