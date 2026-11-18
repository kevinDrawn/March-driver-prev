// =============================================================================
//  safety/safety.h — Motor Safety Protection Module
//  SW current limiting, stall detection, emergency shutdown
// =============================================================================
#pragma once

#include <Arduino.h>

namespace Safety {

  struct Config {
    float max_current_A    = 4.0f;     // SW overcurrent shutdown threshold
    float stall_current_A  = 3.0f;     // Stall detection current threshold
    uint32_t stall_time_ms = 500;      // Stall duration before shutdown
    float warn_current_A   = 2.5f;     // Warning threshold
  };

  /// Initialize safety module with config
  void init(const Config& cfg);

  /// Call every loop() — reads ADC and checks limits
  /// Returns true if motor should continue, false if shutdown triggered
  bool check();

  /// Is the motor in shutdown state?
  bool isShutdown();

  /// Reset shutdown state (after user acknowledges)
  void reset();

  /// Print safety module status
  void printStatus();

  /// Get last measured Irms
  float getLastIrms();

  /// Get shutdown reason string
  const char* getShutdownReason();

} // namespace Safety
