// =============================================================================
//  mcu/clock.h — System Clock Configuration
//  HSE/HSI clock setup for STM32G431
// =============================================================================
#pragma once

// Clock configuration is handled via extern "C" SystemClock_Config() override.
// The implementation is selected at compile time:
//   - USE_HSI_16MHZ_ONLY: Pure HSI 16MHz, no PLL (bench/debug)
//   - USE_HSI_CLOCK:      HSI + PLL → 170MHz (production)
//   - (default):          HSE crystal → PLL (if available)
//
// No function prototypes needed — SystemClock_Config is called by HAL_Init()
// before main(). Including this header ensures the override is linked.

namespace MCU {
  /// Print current clock configuration to Serial
  void printClockInfo();
}
