// =============================================================================
//  driver/drv8316_helper.h — DRV8316 Driver Wrapper
//  SPI initialization, register configuration, fault decoding, full SPI dump
// =============================================================================
#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <SimpleFOC.h>
#include "drivers/drv8316/drv8316.h"
#include "../config.h"

namespace DRV {

  // --- Initialization ---
  /// Initialize SPI bus and DRV8316 communication
  void init();

  /// Configure all DRV8316 registers (PWM mode, OCP, CSA, slew, Buck)
  void configure();

  /// Clear faults and verify nFAULT is HIGH
  bool clearAndCheckFault();

  // --- Fault Diagnostics ---
  /// Print full fault status with decoded bit fields
  void printFaultStatus();

  /// Check if nFAULT pin is LOW (fault active)
  bool isFaulted();

  // --- SPI Register Dump (bit-field decoded) ---
  /// Dump ALL DRV8316 registers with human-readable field decoding
  void dumpAllRegisters();

  /// Dump single register at given address with field decoding
  void dumpRegister(uint8_t addr);

  // --- Hardware Diagnostics ---
  /// Run comprehensive DRV8316 diagnostic (SPI read/write test + fault check)
  void runDiagnostics();

  /// Dump SPI1 + SPI3 GPIO/AF/CR registers for bus diagnostics
  void dumpSPIRegisters();

  // --- Accessors ---
  extern DRV8316Driver drv8316;
  extern SPIClass spiDRV;

} // namespace DRV
