#include "MAQ473Sensor.h"
#include <common/foc_utils.h> // _2PI

// =============================================================================
//  MAQ473 SPI Angle Sensor — Implementation
// =============================================================================

MAQ473Sensor::MAQ473Sensor(int cs_pin, SPIClass *spi, uint32_t spi_clk)
    : _cs(cs_pin), _spi(spi),
      _settings(spi_clk, MSBFIRST, SPI_MODE3) // CPOL=1, CPHA=1
{}

void MAQ473Sensor::init() {
  // Configure chip-select as output, idle high (active-low)
  pinMode(_cs, OUTPUT);
  digitalWrite(_cs, HIGH);

  // Begin SPI (does nothing if already started, safe to call multiple times)
  _spi->begin();

  // Small delay to let sensor stabilize after power-on
  delay(10);

  // Perform a dummy read to flush any residual data
  _spiTransfer16(0x0000);
  delayMicroseconds(1); // >150 ns inter-frame gap

  // Read angle once to verify communication
  uint16_t testAngle = readRawAngle();
  Serial.print("[MAQ473] Init OK — raw angle: ");
  Serial.println(testAngle);

  // Call base Sensor::init() to seed internal angle tracking
  Sensor::init();
}

// =============================================================================
//  SimpleFOC Sensor Interface
// =============================================================================

float MAQ473Sensor::getSensorAngle() {
  uint16_t raw = readRawAngle();
  // Convert 16-bit unsigned (0–65535) to radians (0–2π)
  float angle = (float)raw / 65536.0f * _2PI;
  // Apply non-linearity correction if calibrated
  if (use_lut) angle = applyLUT(angle);
  return angle;
}

// =============================================================================
//  LUT-Based Angle Non-Linearity Compensation
// =============================================================================

float MAQ473Sensor::applyLUT(float raw_rad) {
  // Map angle [0, 2π) → fractional LUT index [0, LUT_SIZE)
  float idx_f = raw_rad * (float)LUT_SIZE / _2PI;
  if (idx_f < 0) idx_f += (float)LUT_SIZE;
  if (idx_f >= (float)LUT_SIZE) idx_f -= (float)LUT_SIZE;

  int idx0 = (int)idx_f;
  int idx1 = (idx0 + 1) % LUT_SIZE;
  float frac = idx_f - (float)idx0;

  // Linear interpolation between adjacent LUT entries
  float error = lut_error[idx0] * (1.0f - frac) + lut_error[idx1] * frac;

  // Subtract error to get corrected angle
  float corrected = raw_rad - error;

  // Wrap to [0, 2π)
  if (corrected < 0) corrected += _2PI;
  if (corrected >= _2PI) corrected -= _2PI;
  return corrected;
}

// =============================================================================
//  Raw Angle Read
// =============================================================================

uint16_t MAQ473Sensor::readRawAngle() {
  if (enableParity) {
    // 17-bit read: 16-bit angle + 1 parity bit
    bool parityBit;
    uint16_t raw = _spiTransfer17(0x0000, parityBit);

    if (checkParity(raw, parityBit)) {
      _lastValidAngle = raw;
      return raw;
    } else {
      parityErrorCount++;
      return _lastValidAngle; // Use last valid on parity error
    }
  } else {
    // Standard 16-bit read (no parity check)
    uint16_t raw = _spiTransfer16(0x0000);
    _lastValidAngle = raw;
    return raw;
  }
}

// =============================================================================
//  Register Read / Write
// =============================================================================

uint8_t MAQ473Sensor::readRegister(uint8_t addr) {
  // MAQ473 register read command:
  //   Bit[15]    = 0 (read)
  //   Bit[14]    = 1 (register access)
  //   Bit[13:8]  = register address (6 bits, addr[5:0])
  //   Bit[7:0]   = 0x00 (don't care for read)
  uint16_t cmd = (0b01 << 14) | ((addr & 0x3F) << 8);

  // First transfer: send the read command
  _spiTransfer16(cmd);
  delayMicroseconds(1); // Inter-frame gap

  // Second transfer: the response contains the register value in bits[7:0]
  uint16_t response = _spiTransfer16(0x0000);
  return (uint8_t)(response & 0xFF);
}

void MAQ473Sensor::writeRegister(uint8_t addr, uint8_t value) {
  // MAQ473 register write command:
  //   Bit[15]    = 1 (write)
  //   Bit[14]    = 1 (register access)
  //   Bit[13:8]  = register address (6 bits)
  //   Bit[7:0]   = data to write
  uint16_t cmd = (0b11 << 14) | ((addr & 0x3F) << 8) | value;

  _spiTransfer16(cmd);
  // Note: If writing to NVM (non-volatile), the caller must wait ~20 ms
  // before subsequent SPI transactions.
}

// =============================================================================
//  Diagnostics
// =============================================================================

bool MAQ473Sensor::isMagnetInRange() {
  // Register 27 (0x1B) contains:
  //   Bit[7] = MGH, Bit[6] = MGL
  //   Bit[3:0] = MIR[3:0] — 0b0011 when field is in range
  uint8_t reg27 = readRegister(0x1B);
  uint8_t mir = reg27 & 0x0F;
  return (mir == 0b0011);
}

bool MAQ473Sensor::checkParity(uint16_t data, bool parityBit) {
  // Even parity: total number of 1-bits in (data + parityBit) should be even
  uint16_t bits = data;
  bits ^= bits >> 8;
  bits ^= bits >> 4;
  bits ^= bits >> 2;
  bits ^= bits >> 1;
  bool dataParity = bits & 1;
  return (dataParity ^ parityBit) == 0;
}

// =============================================================================
//  Low-Level SPI Transfer
// =============================================================================

uint16_t MAQ473Sensor::_spiTransfer16(uint16_t txData) {
  _spi->beginTransaction(_settings);
  digitalWrite(_cs, LOW);

  // Transfer 16 bits as two bytes (MSB first)
  uint8_t highByte = _spi->transfer((uint8_t)(txData >> 8));
  uint8_t lowByte = _spi->transfer((uint8_t)(txData & 0xFF));

  digitalWrite(_cs, HIGH);
  _spi->endTransaction();

  return ((uint16_t)highByte << 8) | lowByte;
}

uint16_t MAQ473Sensor::_spiTransfer17(uint16_t txData, bool &parityBit) {
  _spi->beginTransaction(_settings);
  digitalWrite(_cs, LOW);

  // Transfer 16 data bits + 1 parity bit = 17 bits
  // Send as 3 bytes (24 clocks), only first 17 bits are meaningful
  uint8_t b0 = _spi->transfer((uint8_t)(txData >> 8));   // Bits 15-8
  uint8_t b1 = _spi->transfer((uint8_t)(txData & 0xFF)); // Bits 7-0
  uint8_t b2 = _spi->transfer(0x00); // Bit 17 = parity (MSB of b2)

  digitalWrite(_cs, HIGH);
  _spi->endTransaction();

  // Parity bit is the MSB of the 3rd byte (bit 7 of b2)
  parityBit = (b2 >> 7) & 1;

  return ((uint16_t)b0 << 8) | b1;
}

// =============================================================================
//  SPI Register Dump — MAQ473 decoded fields
// =============================================================================
void MAQ473Sensor::dumpAllRegisters() {
  Serial.println("\n========================================");
  Serial.println("  MAQ473 Register Dump");
  Serial.println("========================================");

  // Key registers with decoded fields
  struct { uint8_t addr; const char* name; } regs[] = {
    {0x00, "Zero_Pos_Low"},  {0x01, "Zero_Pos_High"},
    {0x02, "Config_BCT"},    {0x03, "Config_ETY_ETX"},
    {0x04, "Config_PPT"},    {0x05, "Config_MG"},
    {0x06, "Config_FW"},     {0x07, "Config_HYS"},
    {0x08, "Config_ABRES"},  {0x09, "NVM_Status"},
    {0x1A, "Magnitude"},     {0x1B, "AGC_Status"},
  };

  for (auto& r : regs) {
    uint8_t val = readRegister(r.addr);
    delayMicroseconds(10);
    Serial.print("[ENC] REG[0x");
    if (r.addr < 0x10) Serial.print("0");
    Serial.print(r.addr, HEX);
    Serial.print("] "); Serial.print(r.name);
    // Pad
    int len = strlen(r.name);
    for (int i = len; i < 16; i++) Serial.print(' ');
    Serial.print("= 0x");
    if (val < 0x10) Serial.print("0");
    Serial.print(val, HEX);
    Serial.print(" ("); Serial.print(val); Serial.print(")");

    // Decode specific registers
    if (r.addr == 0x1B) {
      // AGC Status: MGH[7], MGL[6], unused[5:4], MIR[3:0]
      Serial.println();
      Serial.print("       [7]   MGH = "); Serial.print((val >> 7) & 1);
      Serial.println((val >> 7) & 1 ? " (Field too HIGH!)" : " (OK)");
      Serial.print("       [6]   MGL = "); Serial.print((val >> 6) & 1);
      Serial.println((val >> 6) & 1 ? " (Field too LOW!)" : " (OK)");
      Serial.print("       [3:0] MIR = 0b");
      for (int b = 3; b >= 0; b--) Serial.print((val >> b) & 1);
      bool inRange = (val & 0x0F) == 0b0011;
      Serial.println(inRange ? " -> In Range OK" : " -> OUT OF RANGE!");
    }
    else if (r.addr == 0x02) {
      // BCT config
      Serial.println();
      Serial.print("       [7:4] BCT  = "); Serial.print((val >> 4) & 0xF);
      Serial.println(" (bandwidth control)");
      Serial.print("       [3:0] RESERVED");
      Serial.println();
    }
    else if (r.addr == 0x08) {
      // AB Resolution
      Serial.println();
      uint8_t abres = val & 0x0F;
      const char* res_names[] = {"8","16","32","64","128","256","512","1024",
                                  "2048","4096","8192","16384","32768","65536","Rsv","Rsv"};
      Serial.print("       [3:0] ABRES = "); Serial.print(abres);
      Serial.print(" -> "); Serial.print(res_names[abres]); Serial.println(" CPR");
    }
    else {
      Serial.println();
    }
  }

  // Live angle readings
  Serial.println("\n[ENC] Live Angle Readings (5 samples):");
  for (int i = 0; i < 5; i++) {
    uint16_t raw = readRawAngle();
    float deg = raw * 360.0f / 65536.0f;
    float rad = raw * 6.28318530718f / 65536.0f;
    Serial.print("  ["); Serial.print(i); Serial.print("] raw=");
    Serial.print(raw); Serial.print("  deg="); Serial.print(deg, 2);
    Serial.print("  rad="); Serial.println(rad, 4);
    delay(20);
  }

  Serial.print("[ENC] Parity errors: "); Serial.println(parityErrorCount);
  Serial.println("========================================\n");
}

// =============================================================================
//  Encoder Diagnostics
// =============================================================================
void MAQ473Sensor::runDiagnostics() {
  Serial.println("\n[ENC-DIAG] MAQ473 Diagnostics");

  // Magnet status
  Serial.print("  Magnet: ");
  Serial.println(isMagnetInRange() ? "IN RANGE (OK)" : "OUT OF RANGE!");

  // Angle read + stability
  Serial.println("  Angle stability (10 reads):");
  uint16_t minVal = 65535, maxVal = 0;
  for (int i = 0; i < 10; i++) {
    uint16_t r = readRawAngle();
    if (r < minVal) minVal = r;
    if (r > maxVal) maxVal = r;
    delayMicroseconds(500);
  }
  Serial.print("    Range: "); Serial.print(maxVal - minVal);
  Serial.print(" LSBs ("); Serial.print((maxVal - minVal) * 360.0f / 65536.0f, 3);
  Serial.println(" deg)");
  Serial.println((maxVal - minVal) < 50 ? "    Stability: GOOD" : "    Stability: NOISY!");

  // SPI timing
  unsigned long t0 = micros();
  for (int i = 0; i < 100; i++) readRawAngle();
  unsigned long t1 = micros();
  Serial.print("  SPI timing: "); Serial.print((t1 - t0) / 100.0f, 1);
  Serial.println(" us/read");

  Serial.print("  Parity errors: "); Serial.println(parityErrorCount);
}

