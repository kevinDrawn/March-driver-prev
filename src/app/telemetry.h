// =============================================================================
//  app/telemetry.h — Heartbeat and Telemetry Module
// =============================================================================
#pragma once

#include <Arduino.h>

// Forward declare MCU clock
namespace MCU { void printClockInfo(); }

namespace Telem {

  /// Heartbeat — call from loop(), prints status every 2s when idle
  void heartbeat(unsigned long now_ms);

  /// CSV logging — call from loop() when motor is running
  void csvLog(unsigned long now_us);

  /// Print full motor + driver status (for 'S' command)
  void printFullStatus();

  /// Print motor running state (Id/Iq if enabled)
  void printMotorState(float e_angle);

} // namespace Telem
