#ifndef MAQ473_SENSOR_H
#define MAQ473_SENSOR_H

#include <Arduino.h>
#include <SPI.h>
#include <common/base_classes/Sensor.h>

/**
 * MAQ473 Magnetic Angle Sensor — SimpleFOC-compatible Sensor driver.
 *
 * Communication: SPI Mode 3 (CPOL=1, CPHA=1), MSB-first, up to 25 MHz.
 * Angle read:    Send 16 zero-bits → receive 16-bit absolute angle.
 * Register R/W:  16-bit command frame (see MAQ473 datasheet §SPI Interface).
 *
 * The class extends SimpleFOC's Sensor base class, implementing
 * getSensorAngle() which returns the shaft angle in radians [0, 2π).
 */
class MAQ473Sensor : public Sensor {
public:
  /**
   * @param cs_pin   GPIO chip-select pin (directly driven, active-low).
   * @param spi      Pointer to the SPIClass instance (default: &SPI).
   * @param spi_clk  SPI clock frequency in Hz (default: 10 MHz).
   */
  MAQ473Sensor(int cs_pin, SPIClass *spi = &SPI, uint32_t spi_clk = 10000000);

  /**
   * Initialize SPI bus and chip-select GPIO.
   * Also performs a test angle read to verify communication.
   */
  void init() override;

  // ---- SimpleFOC Sensor interface ------------------------------------------

  /**
   * Read the current angle from the sensor.
   * @return Shaft angle in radians [0, 2π).
   */
  float getSensorAngle() override;

  // ---- Angle Non-Linearity Compensation (LUT) ------------------------------

  /** Number of LUT entries (one full mechanical revolution). */
  static constexpr int LUT_SIZE = 128;

  /** Enable LUT-based angle correction. Set to true after calibration. */
  bool use_lut = false;

  /** Error table: lut_error[i] = (sensor_angle - true_angle) at slot i.
   *  Populated by the calibration routine in main.cpp. */
  float lut_error[LUT_SIZE] = {0};

  /**
   * Apply LUT correction to a raw angle.
   * Subtracts the interpolated error from the raw reading.
   * @param raw_rad  Raw sensor angle in radians [0, 2π).
   * @return         Corrected angle in radians [0, 2π).
   */
  float applyLUT(float raw_rad);

  // ---- Extended diagnostic API --------------------------------------------

  /** Enable/disable parity checking on angle reads (default: disabled). */
  bool enableParity = false;


  /** Count of parity errors detected (resets on init). */
  uint32_t parityErrorCount = 0;

  /** Read raw 16-bit angle value (0–65535 → 0°–360°). */
  uint16_t readRawAngle();

  /**
   * Read an 8-bit register from the MAQ473.
   * @param addr  Register address (0x00–0x1B).
   * @return      Register value (8-bit).
   */
  uint8_t readRegister(uint8_t addr);

  /**
   * Write an 8-bit value to a MAQ473 register.
   * @param addr   Register address.
   * @param value  Value to write.
   *
   * Note: After writing, the NVM programming cycle takes ~20 ms.
   *       This function does NOT add a delay — the caller must ensure
   *       sufficient time before the next SPI transaction if writing to NVM.
   */
  void writeRegister(uint8_t addr, uint8_t value);

  /**
   * Check if the magnet is in the valid field range.
   * Reads register 27 and checks MIR[3:0] == 0b0011.
   * @return True if field is in range and sensor array is operational.
   */
  bool isMagnetInRange();

  /**
   * Verify even parity on a received 16-bit + parity-bit angle transfer.
   * The MAQ473 can optionally provide a 17th parity bit.
   * @param data       The 16-bit data word.
   * @param parityBit  The received parity bit (bit 17).
   * @return True if parity is correct (even parity over data + parityBit).
   */
  static bool checkParity(uint16_t data, bool parityBit);

  // ---- SPI Register Dump (decoded) -----------------------------------------

  /**
   * Dump all MAQ473 registers (0x00–0x1B) with bit-field decoding.
   * Shows register name, raw value, and decoded meaning for each field.
   */
  void dumpAllRegisters();

  /**
   * Run encoder diagnostics: angle reads, timing, magnet status
   */
  void runDiagnostics();

private:
  int _cs;
  SPIClass *_spi;
  SPISettings _settings;

  /**
   * Perform a 16-bit SPI transfer.
   * Asserts CS, transfers 16 bits, de-asserts CS.
   * @param txData  16-bit data to send.
   * @return        16-bit data received.
   */
  uint16_t _spiTransfer16(uint16_t txData);

  /**
   * Perform a 17-bit SPI transfer for parity-enabled angle reads.
   * Reads 16-bit angle + 1 parity bit.
   * @param txData     16-bit data to send.
   * @param parityBit  Output: the received parity bit.
   * @return           16-bit angle data.
   */
  uint16_t _spiTransfer17(uint16_t txData, bool &parityBit);

  /** Last valid angle reading (fallback for parity errors). */
  uint16_t _lastValidAngle = 0;
};

#endif // MAQ473_SENSOR_H
