#ifndef USE_H723_BOARD
// =============================================================================
//  mcu/current_sense.cpp — ADC Current Sensing Module
//  JSQR patch, offset calibration, ADC register dump with bit-field decode
// =============================================================================
#include "current_sense.h"
#include <stm32g4xx_ll_adc.h>

namespace CS {

// ---------------------------------------------------------------------------
//  Hardware instance
// ---------------------------------------------------------------------------
#ifdef USE_CUSTOM_BOARD
LowsideCurrentSense currentSense(0.01f, 15.0f, PIN_ISENA, PIN_ISENB, PIN_ISENC);
#else
LowsideCurrentSense currentSense(0.01f, 15.0f, PIN_ISENA, PIN_ISENB);
#endif

// ---------------------------------------------------------------------------
//  patchJSQR() — Fix injected ADC channel assignment
// ---------------------------------------------------------------------------
void patchJSQR() {
  LL_ADC_INJ_StopConversion(ADC1);
  while(LL_ADC_INJ_IsStopConversionOngoing(ADC1)) {}

  uint32_t jsqr = ADC1->JSQR;

#ifdef USE_CUSTOM_BOARD
  // PA0=CH1, PA1=CH2, PA2=CH3 — 3 conversions (JL=2)
  jsqr &= ~(0x3 << 0);         jsqr |= (2 << 0);        // JL=2
  jsqr &= ~(0x1F << 9);        jsqr |= (1 << 9);        // JSQ1=CH1
  jsqr &= ~(0x1F << 15);       jsqr |= (2 << 15);       // JSQ2=CH2
  jsqr &= ~(0x1F << 21);       jsqr |= (3 << 21);       // JSQ3=CH3
#else
  // PB0=CH15, PA1=CH2 — 2 conversions (JL=1)
  jsqr &= ~(0x3 << 0);         jsqr |= (1 << 0);        // JL=1
  jsqr &= ~(0x1F << 9);        jsqr |= (15 << 9);       // JSQ1=CH15
  jsqr &= ~(0x1F << 15);       jsqr |= (2 << 15);       // JSQ2=CH2
#endif

  ADC1->JSQR = jsqr;
  LL_ADC_INJ_StartConversion(ADC1);
}

// ---------------------------------------------------------------------------
//  init()
// ---------------------------------------------------------------------------
void init() {
  // Link will be done externally; just init the CS hardware
  currentSense.skip_align = true;
  // Note: linkDriver and init must be called from main after driver.init()
}

// ---------------------------------------------------------------------------
//  calibrateOffsets() — Average raw ADC at zero current
// ---------------------------------------------------------------------------
void calibrateOffsets() {
  const int N = 1000;
  float sa = 0, sb = 0;
#ifdef USE_CUSTOM_BOARD
  float sc = 0;
#endif

  for (int i = 0; i < N; i++) {
    sa += ADC1->JDR1;
    sb += ADC1->JDR2;
#ifdef USE_CUSTOM_BOARD
    sc += ADC1->JDR3;
#endif
    delayMicroseconds(100);
  }

  float conv = 3.3f / 4096.0f;
  currentSense.offset_ia = (sa / N) * conv;
  currentSense.offset_ib = (sb / N) * conv;
#ifdef USE_CUSTOM_BOARD
  currentSense.offset_ic = (sc / N) * conv;
#endif

  Serial.print("[CS] Offsets: ia="); Serial.print(currentSense.offset_ia, 4);
  Serial.print("V  ib="); Serial.print(currentSense.offset_ib, 4);
#ifdef USE_CUSTOM_BOARD
  Serial.print("V  ic="); Serial.print(currentSense.offset_ic, 4);
#endif
  Serial.println("V");

  // Verify
  delay(10);
  PhaseCurrent_s c = currentSense.getPhaseCurrents();
  Serial.print("[CS] Zero check: Ia="); Serial.print(c.a, 3);
  Serial.print("A  Ib="); Serial.print(c.b, 3);
  Serial.println("A (expect ~0)");
}

// ---------------------------------------------------------------------------
//  readDQ() — Get Id/Iq from phase currents
// ---------------------------------------------------------------------------
void readDQ(float& id, float& iq, float e_angle) {
  PhaseCurrent_s c = currentSense.getPhaseCurrents();
  // Clarke transform
  float i_alpha = c.a;
  float i_beta = (c.a + 2.0f * c.b) / sqrtf(3.0f);
  // Park transform
  float cos_a = cosf(e_angle);
  float sin_a = sinf(e_angle);
  id =  i_alpha * cos_a + i_beta * sin_a;
  iq = -i_alpha * sin_a + i_beta * cos_a;
}

// ---------------------------------------------------------------------------
//  readIrms() — RMS from phase currents
// ---------------------------------------------------------------------------
float readIrms() {
  PhaseCurrent_s c = currentSense.getPhaseCurrents();
  return sqrtf(c.a * c.a + c.b * c.b) * 0.816f; // sqrt(2/3) for 3-phase
}

// ---------------------------------------------------------------------------
//  dumpJSQR() — Decode JSQR register
// ---------------------------------------------------------------------------
void dumpJSQR() {
  uint32_t jsqr = ADC1->JSQR;
  uint32_t jl      = (jsqr >> 0) & 0x3;
  uint32_t jextsel = (jsqr >> 2) & 0x1F;
  uint32_t jexten  = (jsqr >> 7) & 0x3;
  uint32_t jsq1    = (jsqr >> 9) & 0x1F;
  uint32_t jsq2    = (jsqr >> 15) & 0x1F;
  uint32_t jsq3    = (jsqr >> 21) & 0x1F;
  uint32_t jsq4    = (jsqr >> 27) & 0x1F;

  Serial.println("\n[ADC] JSQR Register Decode:");
  Serial.print("  Raw: 0x"); Serial.println(jsqr, HEX);
  Serial.print("  [1:0]   JL       = "); Serial.print(jl);
  Serial.print(" -> "); Serial.print(jl + 1); Serial.println(" conversions");

  Serial.print("  [6:2]   JEXTSEL  = "); Serial.print(jextsel);
  Serial.print(" -> ");
  // STM32G4 injected trigger sources
  const char* trig_names[] = {
    "TIM1_TRGO", "TIM1_CC4", "TIM2_TRGO", "TIM2_CC1",
    "TIM3_CC4", "TIM4_TRGO", "EXTI15", "TIM8_CC4",
    "TIM1_TRGO2", "TIM8_TRGO", "TIM8_TRGO2", "TIM3_CC3",
    "TIM3_TRGO", "TIM3_CC1", "TIM6_TRGO", "TIM15_TRGO"
  };
  if (jextsel < 16) Serial.println(trig_names[jextsel]);
  else { Serial.print("Unknown("); Serial.print(jextsel); Serial.println(")"); }

  Serial.print("  [8:7]   JEXTEN   = "); Serial.print(jexten);
  Serial.print(" -> ");
  const char* edge_names[] = {"Disabled", "Rising", "Falling", "Both"};
  Serial.println(edge_names[jexten]);

  Serial.print("  [13:9]  JSQ1     = CH"); Serial.print(jsq1);
  Serial.print(jsq1 == 0 ? " (NONE!)" : ""); Serial.println();
  Serial.print("  [18:14] JSQ2     = CH"); Serial.print(jsq2);
  Serial.print(jsq2 == 0 ? " (NONE!)" : ""); Serial.println();
  Serial.print("  [23:19] JSQ3     = CH"); Serial.println(jsq3);
  Serial.print("  [28:24] JSQ4     = CH"); Serial.println(jsq4);

  // Expected values check
#ifdef USE_CUSTOM_BOARD
  bool ok = (jl == 2 && jsq1 == 1 && jsq2 == 2 && jsq3 == 3 && jexten != 0);
  Serial.print("  Expected: JL=2 JSQ1=CH1 JSQ2=CH2 JSQ3=CH3 JEXTEN!=0 -> ");
#else
  bool ok = (jl == 1 && jsq1 == 15 && jsq2 == 2 && jexten != 0);
  Serial.print("  Expected: JL=1 JSQ1=CH15 JSQ2=CH2 JEXTEN!=0 -> ");
#endif
  Serial.println(ok ? "OK" : "MISMATCH!");
}

// ---------------------------------------------------------------------------
//  dumpADCRegisters() — Full ADC diagnostic dump
// ---------------------------------------------------------------------------
void dumpADCRegisters() {
  Serial.println("\n========================================");
  Serial.println("  ADC1 Register Dump");
  Serial.println("========================================");

  // ISR
  uint32_t isr = ADC1->ISR;
  Serial.print("[ADC] ISR = 0x"); Serial.println(isr, HEX);
  Serial.print("  [0] ADRDY  = "); Serial.print((isr >> 0) & 1);
  Serial.println((isr & 1) ? " (ADC Ready)" : " (Not Ready!)");
  Serial.print("  [2] EOC    = "); Serial.println((isr >> 2) & 1);
  Serial.print("  [3] EOS    = "); Serial.println((isr >> 3) & 1);
  Serial.print("  [5] JEOC   = "); Serial.println((isr >> 5) & 1);
  Serial.print("  [6] JEOS   = "); Serial.println((isr >> 6) & 1);
  Serial.print("  [7] AWD1   = "); Serial.println((isr >> 7) & 1);

  // CR
  uint32_t cr = ADC1->CR;
  Serial.print("\n[ADC] CR = 0x"); Serial.println(cr, HEX);
  Serial.print("  [0]  ADEN   = "); Serial.println((cr >> 0) & 1);
  Serial.print("  [2]  ADSTART= "); Serial.println((cr >> 2) & 1);
  Serial.print("  [3]  JADSTART="); Serial.println((cr >> 3) & 1);

  // SMPR1/SMPR2
  uint32_t smpr1 = ADC1->SMPR1;
  uint32_t smpr2 = ADC1->SMPR2;
  Serial.print("\n[ADC] SMPR1 = 0x"); Serial.println(smpr1, HEX);
  Serial.print("[ADC] SMPR2 = 0x"); Serial.println(smpr2, HEX);

  const char* smp_cycles[] = {"2.5", "6.5", "12.5", "24.5", "47.5", "92.5", "247.5", "640.5"};
#ifdef USE_CUSTOM_BOARD
  uint8_t smp_ch1 = (smpr1 >> 3) & 0x7;
  uint8_t smp_ch2 = (smpr1 >> 6) & 0x7;
  uint8_t smp_ch3 = (smpr1 >> 9) & 0x7;
  Serial.print("  CH1(PA0) = "); Serial.print(smp_ch1);
  Serial.print(" -> "); Serial.print(smp_cycles[smp_ch1]); Serial.println(" cycles");
  Serial.print("  CH2(PA1) = "); Serial.print(smp_ch2);
  Serial.print(" -> "); Serial.print(smp_cycles[smp_ch2]); Serial.println(" cycles");
  Serial.print("  CH3(PA2) = "); Serial.print(smp_ch3);
  Serial.print(" -> "); Serial.print(smp_cycles[smp_ch3]); Serial.println(" cycles");
#else
  uint8_t smp_ch2 = (smpr1 >> 6) & 0x7;
  uint8_t smp_ch15 = (smpr2 >> 15) & 0x7;
  Serial.print("  CH2(PA1)  = "); Serial.print(smp_ch2);
  Serial.print(" -> "); Serial.print(smp_cycles[smp_ch2]); Serial.println(" cycles");
  Serial.print("  CH15(PB0) = "); Serial.print(smp_ch15);
  Serial.print(" -> "); Serial.print(smp_cycles[smp_ch15]); Serial.println(" cycles");
#endif

  // GPIO MODER check
  Serial.println("\n[ADC] GPIO Analog Mode Check:");
#ifdef USE_CUSTOM_BOARD
  uint32_t pa = GPIOA->MODER;
  Serial.print("  PA0 MODER="); Serial.print((pa >> 0) & 3);
  Serial.print(((pa >> 0) & 3) == 3 ? " (Analog OK)" : " (NOT Analog!)"); Serial.println();
  Serial.print("  PA1 MODER="); Serial.print((pa >> 2) & 3);
  Serial.print(((pa >> 2) & 3) == 3 ? " (Analog OK)" : " (NOT Analog!)"); Serial.println();
  Serial.print("  PA2 MODER="); Serial.print((pa >> 4) & 3);
  Serial.print(((pa >> 4) & 3) == 3 ? " (Analog OK)" : " (NOT Analog!)"); Serial.println();
#else
  Serial.print("  PA1 MODER="); Serial.print((GPIOA->MODER >> 2) & 3);
  Serial.println(((GPIOA->MODER >> 2) & 3) == 3 ? " (Analog OK)" : " (NOT Analog!)");
  Serial.print("  PB0 MODER="); Serial.print((GPIOB->MODER >> 0) & 3);
  Serial.println(((GPIOB->MODER >> 0) & 3) == 3 ? " (Analog OK)" : " (NOT Analog!)");
#endif

  // JSQR
  dumpJSQR();

  // JDR values (live)
  Serial.println("\n[ADC] JDR Live Values (10 samples):");
  for (int i = 0; i < 10; i++) {
    Serial.print("  ["); Serial.print(i); Serial.print("] ");
    Serial.print("JDR1="); Serial.print(ADC1->JDR1);
    Serial.print("  JDR2="); Serial.print(ADC1->JDR2);
    Serial.print("  JDR3="); Serial.print(ADC1->JDR3);
    // Convert to voltage
    float v1 = ADC1->JDR1 * 3.3f / 4096.0f;
    float v2 = ADC1->JDR2 * 3.3f / 4096.0f;
    float v3 = ADC1->JDR3 * 3.3f / 4096.0f;
    Serial.print("  ("); Serial.print(v1, 3);
    Serial.print("V "); Serial.print(v2, 3);
    Serial.print("V "); Serial.print(v3, 3);
    Serial.println("V)");
    delayMicroseconds(200);
  }

  // Current sense offsets
  Serial.println("\n[ADC] Current Sense Offsets:");
  Serial.print("  offset_ia = "); Serial.print(currentSense.offset_ia, 4); Serial.println("V");
  Serial.print("  offset_ib = "); Serial.print(currentSense.offset_ib, 4); Serial.println("V");
#ifdef USE_CUSTOM_BOARD
  Serial.print("  offset_ic = "); Serial.print(currentSense.offset_ic, 4); Serial.println("V");
#endif

  Serial.println("========================================\n");
}

} // namespace CS

#endif
