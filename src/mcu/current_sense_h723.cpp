#include "current_sense.h"
#ifdef USE_H723_BOARD
#include <stm32h7xx_ll_adc.h>

// The PCB routes each phase to a different ADC. SimpleFOC's stock backend
// requires one common ADC, so configure one injected rank on each instance.
namespace {
ADC_HandleTypeDef handles[3] = {};
constexpr float voltsPerCount = 3.3f / 4096.0f;
bool configureADC(unsigned i, ADC_TypeDef* instance, uint32_t channel) {
  auto& h = handles[i];
  h.Instance = instance;
  h.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV4;
  h.Init.Resolution = ADC_RESOLUTION_12B;
  h.Init.ScanConvMode = ADC_SCAN_DISABLE;
  h.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  h.Init.NbrOfConversion = 1;
  h.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  h.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DR;
  h.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  if (i == 2) h.Init.DataAlign = ADC3_DATAALIGN_RIGHT;
  if (HAL_ADC_Init(&h) != HAL_OK) return false;
  ADC_InjectionConfTypeDef c = {};
  c.InjectedChannel = channel;
  c.InjectedRank = ADC_INJECTED_RANK_1;
  c.InjectedSamplingTime = i == 2 ? ADC3_SAMPLETIME_12CYCLES_5 : ADC_SAMPLETIME_8CYCLES_5;
  c.InjectedSingleDiff = ADC_SINGLE_ENDED;
  c.InjectedOffsetNumber = ADC_OFFSET_NONE;
  c.InjectedNbrOfConversion = 1;
  c.ExternalTrigInjecConv = ADC_EXTERNALTRIGINJEC_T1_TRGO;
  c.ExternalTrigInjecConvEdge = ADC_EXTERNALTRIGINJECCONV_EDGE_RISING;
  if (HAL_ADCEx_InjectedConfigChannel(&h, &c) != HAL_OK) return false;
  if (HAL_ADCEx_Calibration_Start(&h, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED) != HAL_OK) return false;
  return HAL_ADCEx_InjectedStart(&h) == HAL_OK;
}
}

int H723CurrentSense::init() {
  if (!driver) return 0;
  __HAL_RCC_ADC12_CLK_ENABLE();
  __HAL_RCC_ADC3_CLK_ENABLE();
  pinMode(PIN_ISENA, INPUT_ANALOG);
  pinMode(PIN_ISENB, INPUT_ANALOG);
  pinMode(PIN_ISENC, INPUT_ANALOG);
  // Open PC2_C analog switch (dedicated ADC3 input, not PC2).
  HAL_SYSCFG_AnalogSwitchConfig(SYSCFG_SWITCH_PC2, SYSCFG_SWITCH_PC2_OPEN);
  // TIM1 update triggers all three ADCs from the same PWM timebase.
  MODIFY_REG(TIM1->CR2, TIM_CR2_MMS, TIM_TRGO_UPDATE);
  if (!configureADC(0, ADC1, ADC_CHANNEL_6) ||
      !configureADC(1, ADC2, ADC_CHANNEL_6) ||
      !configureADC(2, ADC3, ADC_CHANNEL_0)) return 0;
  uint32_t start = millis();
  while (!(ADC1->ISR & ADC_ISR_JEOS) || !(ADC2->ISR & ADC_ISR_JEOS) || !(ADC3->ISR & ADC_ISR_JEOS)) {
    if (millis() - start > 100) return 0;
  }
  initialized = true;
  CS::calibrateOffsets();
  return 1;
}

PhaseCurrent_s H723CurrentSense::getPhaseCurrents() {
  return {(ADC1->JDR1 * voltsPerCount - offset_ia) * gain_a,
          (ADC2->JDR1 * voltsPerCount - offset_ib) * gain_b,
          (ADC3->JDR1 * voltsPerCount - offset_ic) * gain_c};
}

namespace CS {
H723CurrentSense currentSense;
void init() { currentSense.skip_align = true; }
void patchJSQR() {} // G431 workaround must never touch the H723 sequencers.
void calibrateOffsets() {
  float a=0, b=0, c=0;
  for (unsigned i=0; i<1000; ++i) {
    a += ADC1->JDR1; b += ADC2->JDR1; c += ADC3->JDR1;
    delayMicroseconds(100);
  }
  currentSense.offset_ia = a * voltsPerCount / 1000;
  currentSense.offset_ib = b * voltsPerCount / 1000;
  currentSense.offset_ic = c * voltsPerCount / 1000;
}
void readDQ(float& id, float& iq, float angle) {
  auto c = currentSense.getPhaseCurrents();
  float beta = (c.a + 2*c.b) * 0.577350269f;
  id = c.a*cosf(angle) + beta*sinf(angle);
  iq = -c.a*sinf(angle) + beta*cosf(angle);
}
float readIrms() {
  auto c = currentSense.getPhaseCurrents();
  return sqrtf((c.a*c.a+c.b*c.b+c.c*c.c)/3.0f);
}
void dumpJSQR() {
  Serial.print("ADC1 JSQR=0x"); Serial.println(ADC1->JSQR, HEX);
  Serial.print("ADC2 JSQR=0x"); Serial.println(ADC2->JSQR, HEX);
  Serial.print("ADC3 JSQR=0x"); Serial.println(ADC3->JSQR, HEX);
}
void dumpADCRegisters() {
  dumpJSQR();
  Serial.print("A/B/C raw: "); Serial.print(ADC1->JDR1);
  Serial.print('/'); Serial.print(ADC2->JDR1);
  Serial.print('/'); Serial.println(ADC3->JDR1);
}
}
#endif
