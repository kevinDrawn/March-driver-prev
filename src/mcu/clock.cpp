// =============================================================================
//  mcu/clock.cpp — System Clock Configuration
// =============================================================================
#include <Arduino.h>
#include "clock.h"

// ---------------------------------------------------------------------------
//  SystemClock_Config overrides (weak symbol from HAL)
// ---------------------------------------------------------------------------
#ifdef USE_HSI_CLOCK

#ifdef USE_HSI_16MHZ_ONLY
// Pure HSI 16MHz — no PLL (for bench test / debugging)
extern "C" void SystemClock_Config(void) {
  RCC_OscInitTypeDef osc = {0};
  RCC_ClkInitTypeDef clk = {0};
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);
  osc.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  osc.HSIState = RCC_HSI_ON;
  osc.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  osc.PLL.PLLState = RCC_PLL_NONE;
  HAL_RCC_OscConfig(&osc);
  clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                 | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  clk.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
  clk.APB1CLKDivider = RCC_HCLK_DIV1;
  clk.APB2CLKDivider = RCC_HCLK_DIV1;
  HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_0);
}
#else
// HSI + PLL → 170MHz
// HSI(16MHz) → PLL(M=4,N=85,R=2) → SYSCLK 170MHz
extern "C" void SystemClock_Config(void) {
  RCC_OscInitTypeDef osc = {0};
  RCC_ClkInitTypeDef clk = {0};
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);
  osc.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  osc.HSIState = RCC_HSI_ON;
  osc.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  osc.PLL.PLLState = RCC_PLL_ON;
  osc.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  osc.PLL.PLLM = RCC_PLLM_DIV4;
  osc.PLL.PLLN = 85;
  osc.PLL.PLLP = RCC_PLLP_DIV2;
  osc.PLL.PLLQ = RCC_PLLQ_DIV2;
  osc.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&osc) != HAL_OK) {
    // PLL failed — fall back to bare HSI 16MHz
    osc.PLL.PLLState = RCC_PLL_NONE;
    HAL_RCC_OscConfig(&osc);
    clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                   | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV1;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_0);
    return;
  }
  clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                 | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
  clk.APB1CLKDivider = RCC_HCLK_DIV1;
  clk.APB2CLKDivider = RCC_HCLK_DIV1;
  HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_8);
}
#endif // USE_HSI_16MHZ_ONLY

#endif // USE_HSI_CLOCK

// ---------------------------------------------------------------------------
//  Clock diagnostic print
// ---------------------------------------------------------------------------
namespace MCU {

void printClockInfo() {
  Serial.println("\n[MCU] Clock Configuration:");
  uint32_t sysclk = HAL_RCC_GetSysClockFreq();
  uint32_t hclk   = HAL_RCC_GetHCLKFreq();
  uint32_t pclk1  = HAL_RCC_GetPCLK1Freq();
  uint32_t pclk2  = HAL_RCC_GetPCLK2Freq();

  Serial.print("  SYSCLK = "); Serial.print(sysclk / 1000000); Serial.println(" MHz");
  Serial.print("  HCLK   = "); Serial.print(hclk   / 1000000); Serial.println(" MHz");
  Serial.print("  PCLK1  = "); Serial.print(pclk1  / 1000000); Serial.println(" MHz");
  Serial.print("  PCLK2  = "); Serial.print(pclk2  / 1000000); Serial.println(" MHz");

  // Clock source
  uint32_t src = __HAL_RCC_GET_SYSCLK_SOURCE();
  Serial.print("  Source = ");
  switch (src) {
    case RCC_SYSCLKSOURCE_STATUS_HSI: Serial.println("HSI"); break;
    case RCC_SYSCLKSOURCE_STATUS_HSE: Serial.println("HSE (crystal)"); break;
    case RCC_SYSCLKSOURCE_STATUS_PLLCLK: Serial.println("PLL"); break;
    default: Serial.print("Unknown (0x"); Serial.print(src, HEX); Serial.println(")"); break;
  }

  // PLL config if active
  if (src == RCC_SYSCLKSOURCE_STATUS_PLLCLK) {
#ifndef USE_H723_BOARD
    Serial.print("  PLL: M="); Serial.print((RCC->PLLCFGR >> 4) & 0xF);
    Serial.print(" N="); Serial.print((RCC->PLLCFGR >> 8) & 0x7F);
    Serial.print(" R="); Serial.println(((RCC->PLLCFGR >> 25) & 0x3) * 2 + 2);

#endif
  }

  // Verify expectations
  bool ok = true;
#ifdef USE_HSI_16MHZ_ONLY
  if (sysclk != 16000000) { Serial.println("  *** WARN: Expected 16 MHz! ***"); ok = false; }
#elif defined(USE_HSI_CLOCK)
  if (sysclk != 170000000 && sysclk != 16000000) {
    Serial.println("  *** WARN: Expected 170 or 16 MHz! ***"); ok = false;
  }
#endif
  Serial.println(ok ? "  Status: OK" : "  Status: CHECK!");
}

} // namespace MCU
