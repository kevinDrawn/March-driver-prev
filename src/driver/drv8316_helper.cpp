// =============================================================================
//  driver/drv8316_helper.cpp — DRV8316 Driver Module
//  SPI init, register config, fault decode, bit-field register dump
// =============================================================================
#include "drv8316_helper.h"

namespace DRV {

// ---------------------------------------------------------------------------
//  Hardware instances
// ---------------------------------------------------------------------------
SPIClass spiDRV(PIN_SPI_DRV_MOSI, PIN_SPI_DRV_MISO, PIN_SPI_DRV_SCLK, -1);
DRV8316Driver drv8316(PIN_DRV_CS);

// ---------------------------------------------------------------------------
//  init() — SPI bus + DRV8316 communication startup
// ---------------------------------------------------------------------------
void init() {
  Serial.println("[DRV] Initializing DRV8316...");
  drv8316.init(&spiDRV);

  // Re-enable internal pull-up on DRV MISO after spiDRV.begin()
  // STM32's SPI init resets all SPI pins to NOPULL; DRV8316 SDO is open-drain
#if defined(USE_H723_BOARD)
  GPIOB->PUPDR = (GPIOB->PUPDR & ~(3U << 28)) | (1U << 28); // PB14
#elif defined(USE_CUSTOM_BOARD)
  GPIOB->PUPDR = (GPIOB->PUPDR & ~(3U << (4 * 2))) | (1U << (4 * 2)); // PB4
#else
  GPIOA->PUPDR = (GPIOA->PUPDR & ~(3U << (6 * 2))) | (1U << (6 * 2)); // PA6
#endif
  delay(1);

  // Unlock registers (DRV8316 boots locked!)
  drv8316.setRegistersLocked(false);
  delay(1);

  // Switch SDO to Push-Pull (no longer needs pull-up after this)
  drv8316.setSDOMode(SDOMode_PushPull);
  delay(1);
}

// ---------------------------------------------------------------------------
//  configure() — Full register setup
// ---------------------------------------------------------------------------
void configure() {
#ifdef USE_CUSTOM_BOARD
  drv8316.setPWMMode(PWM6_Mode);
#else
  drv8316.setPWMMode(PWM3_Mode);
#endif
  delay(1);
  drv8316.setSlew(Slew_200Vus);
  delay(1);
  drv8316.setCurrentSenseGain(Gain_0V15);
  delay(1);
  drv8316.setOCPMode(AutoRetry_Fault);  // ★ AutoRetry for safety
  delay(1);
  drv8316.setOCPLevel(Curr_24A);
  delay(1);
  drv8316.setOCPDeglitchTime(Deglitch_1us6);
  delay(1);
  drv8316.setOCPClearInPWMCycleChange(true);
  delay(1);
  drv8316.setBuckVoltage(VB_5V);
  delay(1);

  // Clear boot-time faults
  clearAndCheckFault();

  Serial.println("[DRV] Configuration complete.");
}

// ---------------------------------------------------------------------------
//  clearAndCheckFault()
// ---------------------------------------------------------------------------
bool clearAndCheckFault() {
  drv8316.clearFault();
  delay(10);
  drv8316.clearFault();
  delay(10);
  bool ok = !DRV::isFaulted();
  Serial.print("[DRV] nFAULT: ");
  Serial.println(ok ? "HIGH (OK)" : "LOW (FAULT!)");
  return ok;
}

// ---------------------------------------------------------------------------
//  isFaulted()
// ---------------------------------------------------------------------------
bool isFaulted() {
#ifdef USE_H723_BOARD
  return drv8316.getStatus().isFault();
#else
  return digitalRead(PIN_nFAULT) == LOW;
#endif
}

// ---------------------------------------------------------------------------
//  printFaultStatus() — Decoded fault status
// ---------------------------------------------------------------------------
void printFaultStatus() {
  DRV8316Status st = drv8316.getStatus();
  Serial.println("\n--- DRV8316 Fault Status ---");
  Serial.print("  IC_Status raw: 0x"); Serial.println(st.status.reg, HEX);

  // IC_Status register
  Serial.print("  FAULT: ");   Serial.print(st.isFault());
  Serial.print("  OT: ");      Serial.print(st.isOverTemperature());
  Serial.print("  OCP: ");     Serial.print(st.isOverCurrent());
  Serial.print("  OVP: ");     Serial.print(st.isOverVoltage());
  Serial.print("  SPI: ");     Serial.print(st.isSPIError());
  Serial.print("  BUCK: ");    Serial.print(st.isBuckError());
  Serial.print("  POR: ");     Serial.println(st.isPowerOnReset());

  // Status_1 — per-phase OCP detail
  Serial.print("  OCP_HA: ");  Serial.print(st.isOverCurrent_Ah());
  Serial.print("  OCP_LA: ");  Serial.print(st.isOverCurrent_Al());
  Serial.print("  OCP_HB: ");  Serial.print(st.isOverCurrent_Bh());
  Serial.print("  OCP_LB: ");  Serial.print(st.isOverCurrent_Bl());
  Serial.print("  OCP_HC: ");  Serial.print(st.isOverCurrent_Ch());
  Serial.print("  OCP_LC: ");  Serial.println(st.isOverCurrent_Cl());
  Serial.print("  OTS: ");     Serial.print(st.isOverTemperatureShutdown());
  Serial.print("  OTW: ");     Serial.println(st.isOverTemperatureWarning());

  // Status_2 — SPI/Buck faults
  Serial.print("  SPI_PAR: "); Serial.print(st.isSPIParityError());
  Serial.print("  SPI_CLK: "); Serial.print(st.isSPIClockFramingError());
  Serial.print("  SPI_ADDR: ");Serial.print(st.isSPIAddressError());
  Serial.print("  VCP_UV: ");  Serial.print(st.isChargePumpUnderVoltage());
  Serial.print("  BUCK_UV: "); Serial.print(st.isBuckUnderVoltage());
  Serial.print("  BUCK_OCP: ");Serial.println(st.isBuckOverCurrent());

  Serial.print("  nFAULT pin: ");
  Serial.println((!DRV::isFaulted()) ? "HIGH (OK)" : "LOW (FAULT!)");

  // Diagnosis hints
  if (st.isOverCurrent()) {
    Serial.println("  --> HINT: OCP detected. Check Status_1 for phase detail.");
    if (st.isOverCurrent_Ah() || st.isOverCurrent_Al()) Serial.println("       Phase A overcurrent!");
    if (st.isOverCurrent_Bh() || st.isOverCurrent_Bl()) Serial.println("       Phase B overcurrent!");
    if (st.isOverCurrent_Ch() || st.isOverCurrent_Cl()) Serial.println("       Phase C overcurrent!");
  }
  if (st.isOverTemperature()) {
    Serial.println("  --> HINT: Over-temperature! Reduce current/voltage.");
  }
  if (st.isBuckError()) {
    Serial.println("  --> HINT: Buck error. Check VM supply and Buck settings.");
  }
  Serial.println("---");
}

// ---------------------------------------------------------------------------
//  Register names and bit-field definitions for decoded dump
// ---------------------------------------------------------------------------
struct BitField {
  uint8_t msb;
  uint8_t lsb;
  const char* name;
  const char* values[8]; // up to 8 values for 3-bit fields
};

struct RegDef {
  uint8_t addr;
  const char* name;
  uint8_t fieldCount;
  BitField fields[8];
};

// DRV8316 register map — bit positions per datasheet (LSB-first)
static const RegDef REG_MAP[] = {
  // 0x00: IC_Status (read-only)
  { 0x00, "IC_Status", 7, {
    {6,6, "FAULT",    {"No fault", "FAULT!"}},
    {5,5, "OT",       {"OK", "Over-Temp!"}},
    {4,4, "OCP",      {"OK", "Over-Current!"}},
    {3,3, "OVP",      {"OK", "Over-Voltage!"}},
    {2,2, "SPI_FLT",  {"OK", "SPI Error!"}},
    {1,1, "BK_FLT",   {"OK", "Buck Fault!"}},
    {0,0, "POR",      {"No", "Power-On Reset"}},
  }},

  // 0x01: Status_1 (read-only)
  { 0x01, "Status_1", 8, {
    {7,7, "OTS",      {"OK", "OT Shutdown!"}},
    {6,6, "OTW",      {"OK", "OT Warning!"}},
    {5,5, "OCP_HA",   {"OK", "Phase A High OCP!"}},
    {4,4, "OCP_LA",   {"OK", "Phase A Low OCP!"}},
    {3,3, "OCP_HB",   {"OK", "Phase B High OCP!"}},
    {2,2, "OCP_LB",   {"OK", "Phase B Low OCP!"}},
    {1,1, "OCP_HC",   {"OK", "Phase C High OCP!"}},
    {0,0, "OCP_LC",   {"OK", "Phase C Low OCP!"}},
  }},

  // 0x02: Status_2 (read-only)
  { 0x02, "Status_2", 6, {
    {5,5, "SPI_PAR",  {"OK", "Parity Error!"}},
    {4,4, "SPI_CLK",  {"OK", "Clock Frame Err!"}},
    {3,3, "SPI_ADDR", {"OK", "Addr Error!"}},
    {2,2, "VCP_UV",   {"OK", "ChargePump UV!"}},
    {1,1, "BUCK_UV",  {"OK", "Buck UV!"}},
    {0,0, "BUCK_OCP", {"OK", "Buck OCP!"}},
  }},

  // 0x03: Control_1 — REG_LOCK
  { 0x03, "Control_1", 1, {
    {2,0, "REG_LOCK",  {"Unlocked(000)", "","","Unlocked(011)", "","","Locked(110)","Locked(111)"}},
  }},

  // 0x04: Control_2 — PWM, slew, SDO
  { 0x04, "Control_2", 4, {
    {2,1, "PWM_MODE",   {"6x-PWM", "6xPWM+CurrLim", "3x-PWM", "3xPWM+CurrLim"}},
    {4,3, "SLEW",       {"25V/us", "50V/us", "150V/us", "200V/us"}},
    {5,5, "SDO_MODE",   {"Open-drain", "Push-pull"}},
    {0,0, "CLR_FLT",    {"Normal", "Clear Fault"}},
  }},

  // 0x05: Control_3 — OTW, SPI_FLT, OVP
  { 0x05, "Control_3", 5, {
    {0,0, "OTW_REP",    {"Disabled", "Enabled"}},
    {1,1, "SPI_FLT_REP",{"Report(0)", "NoReport(1)"}},
    {2,2, "OVP_EN",     {"Disabled", "Enabled"}},
    {3,3, "OVP_SEL",    {"32V", "20V"}},
    {4,4, "PWM100_DUTY",{"20kHz", "40kHz"}},
  }},

  // 0x06: Control_4 — OCP, DRV_OFF
  { 0x06, "Control_4", 6, {
    {1,0, "OCP_MODE",   {"Latched", "AutoRetry", "ReportOnly", "NoAction"}},
    {2,2, "OCP_LVL",    {"16A", "24A"}},
    {3,3, "OCP_RETRY",  {"5ms", "500ms"}},
    {5,4, "OCP_DEG",    {"0.2us", "0.6us", "1.1us", "1.6us"}},
    {6,6, "OCP_CBC",    {"Disabled", "Enabled"}},
    {7,7, "DRV_OFF",    {"Active", "HiZ"}},
  }},

  // 0x07: Control_5 — CSA, ASR, AAR
  { 0x07, "Control_5", 5, {
    {1,0, "CSA_GAIN",   {"0.15V/A", "0.1875V/A", "0.25V/A", "0.375V/A"}},
    {2,2, "EN_ASR",     {"Disabled", "Enabled"}},
    {3,3, "EN_AAR",     {"Disabled", "Enabled"}},
    {6,6, "ILIM_RECIR", {"Brake(FET)", "Coast(Diode)"}},
  }},

  // 0x08: Control_6 — Buck regulator
  { 0x08, "Control_6", 5, {
    {0,0, "BUCK_DIS",   {"Enabled", "Disabled"}},
    {2,1, "BUCK_SEL",   {"3.3V", "5.0V", "4.0V", "5.7V"}},
    {3,3, "BUCK_CL",    {"600mA", "150mA"}},
    {4,4, "BUCK_PS_DIS",{"SeqEnabled", "SeqDisabled"}},
  }},

  // 0x09: Control_7 (was Control_10 in some docs)
  { 0x09, "Control_7", 1, {
    {2,0, "REG_LOCK",   {"Unlocked(000)", "","","Unlocked(011)", "","","Locked(110)","Locked(111)"}},
  }},
};
static const int NUM_REGS = sizeof(REG_MAP) / sizeof(REG_MAP[0]);

// ---------------------------------------------------------------------------
//  dumpRegister() — Single register with bit-field decode
// ---------------------------------------------------------------------------
void dumpRegister(uint8_t addr) {
  // Raw SPI read: DRV8316 SPI frame = [R/W][addr4:0][data8][parity]
  spiDRV.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE1));
  digitalWrite(PIN_DRV_CS, LOW);
  delayMicroseconds(1);
  uint16_t cmd = (1 << 15) | ((addr & 0x1F) << 9); // Read command
  uint16_t raw = spiDRV.transfer16(cmd);
  digitalWrite(PIN_DRV_CS, HIGH);
  spiDRV.endTransaction();

  uint8_t status_byte = (raw >> 8) & 0xFF;
  uint8_t data_byte   = raw & 0xFF;

  // Find register definition
  const RegDef* rd = nullptr;
  for (int i = 0; i < NUM_REGS; i++) {
    if (REG_MAP[i].addr == addr) { rd = &REG_MAP[i]; break; }
  }

  Serial.print("[DRV] REG[0x0"); Serial.print(addr, HEX);
  Serial.print("] ");
  if (rd) Serial.print(rd->name);
  Serial.print(" = 0x"); Serial.print(raw, HEX);
  Serial.print(" (status=0x"); Serial.print(status_byte, HEX);
  Serial.print(" data=0x"); Serial.print(data_byte, HEX);
  Serial.println(")");

  // Check for SPI failure (all 1s)
  if (raw == 0xFFFF) {
    Serial.println("  *** SPI FAIL: All bits high. Check wiring! ***");
    return;
  }

  // Decode bit fields
  if (rd) {
    for (int f = 0; f < rd->fieldCount; f++) {
      const BitField& bf = rd->fields[f];
      uint8_t width = bf.msb - bf.lsb + 1;
      uint8_t mask = (1 << width) - 1;
      uint8_t val = (data_byte >> bf.lsb) & mask;

      Serial.print("  ["); Serial.print(bf.msb);
      if (bf.msb != bf.lsb) { Serial.print(":"); Serial.print(bf.lsb); }
      Serial.print("] ");
      Serial.print(bf.name);

      // Pad to alignment
      int nameLen = strlen(bf.name);
      for (int p = nameLen; p < 12; p++) Serial.print(' ');

      Serial.print("= "); Serial.print(val);
      Serial.print(" -> ");

      // Print decoded value
      if (val < 8 && bf.values[val] && bf.values[val][0]) {
        Serial.print(bf.values[val]);
        // Mark errors for status registers
        if (addr <= 0x02 && val != 0 && bf.lsb != 0) {
          // Non-zero in status reg (except POR) = problem
          Serial.print(" !");
        }
      } else {
        Serial.print("(raw="); Serial.print(val); Serial.print(")");
      }
      Serial.println();
    }
  }
  Serial.flush();
}

// ---------------------------------------------------------------------------
//  dumpAllRegisters() — Full register dump
// ---------------------------------------------------------------------------
void dumpAllRegisters() {
  Serial.println("\n========================================");
  Serial.println("  DRV8316 Full Register Dump");
  Serial.println("========================================");

  for (int i = 0; i < NUM_REGS; i++) {
    dumpRegister(REG_MAP[i].addr);
    Serial.println();
  }

  // nFAULT pin state
  Serial.print("[DRV] nFAULT pin: ");
  Serial.println((!DRV::isFaulted()) ? "HIGH (OK)" : "LOW (FAULT!)");
  Serial.println("========================================\n");
}

// ---------------------------------------------------------------------------
//  runDiagnostics() — SPI read/write test + config verification
// ---------------------------------------------------------------------------
void runDiagnostics() {
  Serial.println("\n[DRV-DIAG] DRV8316 SPI Diagnostics");

  // Read test
  DRV8316_PWMMode pwmMode = drv8316.getPWMMode();
  Serial.print("  PWM Mode: ");
#ifdef USE_CUSTOM_BOARD
  Serial.print(pwmMode == PWM6_Mode ? "6xPWM" : "UNEXPECTED!");
  bool pwm_ok = (pwmMode == PWM6_Mode);
#else
  Serial.print(pwmMode == PWM3_Mode ? "3xPWM" : "UNEXPECTED!");
  bool pwm_ok = (pwmMode == PWM3_Mode);
#endif
  Serial.print(" (raw="); Serial.print(pwmMode); Serial.println(")");

  DRV8316_OCPMode ocpMode = drv8316.getOCPMode();
  Serial.print("  OCP Mode: ");
  Serial.print(ocpMode == AutoRetry_Fault ? "AutoRetry OK" : "NOT AutoRetry!");
  Serial.print(" (raw="); Serial.print(ocpMode); Serial.println(")");

  DRV8316_CSAGain csaGain = drv8316.getCurrentSenseGain();
  Serial.print("  CSA Gain: ");
  Serial.print(csaGain == Gain_0V15 ? "0.15V/A OK" : "UNEXPECTED!");
  Serial.print(" (raw="); Serial.print(csaGain); Serial.println(")");

  DRV8316_Slew slew = drv8316.getSlew();
  Serial.print("  Slew Rate: ");
  Serial.print(slew == Slew_200Vus ? "200V/us OK" : "UNEXPECTED!");
  Serial.print(" (raw="); Serial.print(slew); Serial.println(")");

  Serial.print("  nFAULT: ");
  Serial.println((!DRV::isFaulted()) ? "HIGH (OK)" : "LOW (FAULT!)");

  // Overall
  bool all_ok = pwm_ok && (ocpMode == AutoRetry_Fault)
                && (csaGain == Gain_0V15) && !isFaulted();
  Serial.print("[DRV-DIAG] Result: ");
  Serial.println(all_ok ? "ALL OK" : "ISSUES FOUND - run Dd for full dump");
}

// ---------------------------------------------------------------------------
//  dumpSPIRegisters() — SPI1 + SPI3 hardware register diagnostic
// ---------------------------------------------------------------------------
void dumpSPIRegisters() {
#ifdef USE_H723_BOARD
  Serial.print("SPI1 CFG1=0x"); Serial.println(SPI1->CFG1, HEX);
  Serial.print("SPI2 CFG1=0x"); Serial.println(SPI2->CFG1, HEX);
  Serial.print("SPI2 SR=0x"); Serial.println(SPI2->SR, HEX);
#else
  Serial.println("\n========================================");
  Serial.println("  SPI Bus Hardware Diagnostic");
  Serial.println("========================================");

  // --- SPI3 (DRV8316) ---
  Serial.println("[SPI3] DRV8316 Bus (PB3/PB4/PB5)");
  uint32_t moderB = GPIOB->MODER;
  uint32_t afrlB  = GPIOB->AFR[0];
  Serial.print("  MODER: PB2="); Serial.print((moderB >> 4) & 3);
  Serial.print(" PB3="); Serial.print((moderB >> 6) & 3);
  Serial.print(" PB4="); Serial.print((moderB >> 8) & 3);
  Serial.print(" PB5="); Serial.println((moderB >> 10) & 3);
  Serial.print("  AFRL:  PB3_AF="); Serial.print((afrlB >> 12) & 0xF);
  Serial.print(" PB4_AF="); Serial.print((afrlB >> 16) & 0xF);
  Serial.print(" PB5_AF="); Serial.println((afrlB >> 20) & 0xF);
  Serial.print("  CR1=0x"); Serial.print(SPI3->CR1, HEX);
  Serial.print("  CR2=0x"); Serial.print(SPI3->CR2, HEX);
  Serial.print("  SR=0x"); Serial.println(SPI3->SR, HEX);
  uint32_t ds3 = (SPI3->CR2 >> 8) & 0xF;
  Serial.print("  DS="); Serial.print(ds3); Serial.print(" (7=8bit 15=16bit)");
  Serial.print("  SPE="); Serial.println((SPI3->CR1 >> 6) & 1);
  Serial.print("  EXPECT: AF=6(SPI3) DS=7 SPE=1");
  bool spi3_ok = (((afrlB >> 12) & 0xF) == 6) && (((SPI3->CR1 >> 6) & 1) == 1);
  Serial.println(spi3_ok ? " -> OK" : " -> FAIL!");

  // --- SPI1 (MAQ473) ---
  Serial.println("\n[SPI1] MAQ473 Encoder Bus (PA5/PA6/PA7)");
  uint32_t moderA = GPIOA->MODER;
  uint32_t afrlA  = GPIOA->AFR[0];
  Serial.print("  MODER: PA4="); Serial.print((moderA >> 8) & 3);
  Serial.print(" PA5="); Serial.print((moderA >> 10) & 3);
  Serial.print(" PA6="); Serial.print((moderA >> 12) & 3);
  Serial.print(" PA7="); Serial.println((moderA >> 14) & 3);
  Serial.print("  AFRL:  PA5_AF="); Serial.print((afrlA >> 20) & 0xF);
  Serial.print(" PA6_AF="); Serial.print((afrlA >> 24) & 0xF);
  Serial.print(" PA7_AF="); Serial.println((afrlA >> 28) & 0xF);
  Serial.print("  CR1=0x"); Serial.print(SPI1->CR1, HEX);
  Serial.print("  CR2=0x"); Serial.print(SPI1->CR2, HEX);
  Serial.print("  SR=0x"); Serial.println(SPI1->SR, HEX);
  uint32_t ds1 = (SPI1->CR2 >> 8) & 0xF;
  Serial.print("  DS="); Serial.print(ds1);
  Serial.print("  SPE="); Serial.println((SPI1->CR1 >> 6) & 1);
  Serial.print("  EXPECT: AF=5(SPI1) DS=7 SPE=1");
  bool spi1_ok = (((afrlA >> 20) & 0xF) == 5) && (((SPI1->CR1 >> 6) & 1) == 1);
  Serial.println(spi1_ok ? " -> OK" : " -> FAIL!");

  Serial.println("========================================");
#endif
}

} // namespace DRV
