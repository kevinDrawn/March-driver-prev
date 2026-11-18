// =============================================================================
//  safety/safety.cpp — Motor Safety Protection Module
//  Reads ADC every loop, shuts down motor on overcurrent or stall
// =============================================================================
#include "safety.h"
#include "../mcu/current_sense.h"
#include <SimpleFOC.h>

// Motor reference (extern from main)
extern BLDCMotor motor;
extern bool motorRunning;

namespace Safety {

static Config  _cfg;
static bool    _shutdown = false;
static const char* _reason = "";
static float   _lastIrms = 0;
static float   _peakIrms = 0;
static uint32_t _stallStart = 0;
static bool    _stallActive = false;
static uint32_t _ocCount = 0;
static uint32_t _checkCount = 0;

// ---------------------------------------------------------------------------
void init(const Config& cfg) {
  _cfg = cfg;
  _shutdown = false;
  _reason = "";
  _lastIrms = 0;
  _peakIrms = 0;
  _stallStart = 0;
  _stallActive = false;
  _ocCount = 0;
  _checkCount = 0;
  Serial.print("[SAFETY] Initialized: Imax="); Serial.print(_cfg.max_current_A, 1);
  Serial.print("A  Istall="); Serial.print(_cfg.stall_current_A, 1);
  Serial.print("A  t_stall="); Serial.print(_cfg.stall_time_ms);
  Serial.println("ms");
}

// ---------------------------------------------------------------------------
bool check() {
  if (_shutdown) return false;
  _checkCount++;

  // Read current (every call — this is fast, just ADC register reads)
  _lastIrms = CS::readIrms();
  if (_lastIrms > _peakIrms) _peakIrms = _lastIrms;

  // --- Overcurrent check ---
  if (_lastIrms > _cfg.max_current_A) {
    _ocCount++;
    if (_ocCount >= 3) {  // 3 consecutive over-limit readings
      _shutdown = true;
      _reason = "OVERCURRENT";
      motor.disable();
      motorRunning = false;
      Serial.print("\n[SAFETY] *** OVERCURRENT SHUTDOWN! *** Irms=");
      Serial.print(_lastIrms, 2);
      Serial.print("A (limit="); Serial.print(_cfg.max_current_A, 1);
      Serial.println("A)");
      return false;
    }
  } else {
    _ocCount = 0;
  }

  // --- Stall detection ---
  // If current > stall threshold but motor velocity is very low, it's stalling
  if (_lastIrms > _cfg.stall_current_A) {
    if (!_stallActive) {
      _stallActive = true;
      _stallStart = millis();
    } else if (millis() - _stallStart > _cfg.stall_time_ms) {
      _shutdown = true;
      _reason = "STALL";
      motor.disable();
      motorRunning = false;
      Serial.print("\n[SAFETY] *** STALL SHUTDOWN! *** Irms=");
      Serial.print(_lastIrms, 2);
      Serial.print("A sustained >"); Serial.print(_cfg.stall_time_ms);
      Serial.println("ms");
      return false;
    }
  } else {
    _stallActive = false;
  }

  // --- Warning (periodic, not shutdown) ---
  static uint32_t lastWarn = 0;
  if (_lastIrms > _cfg.warn_current_A && millis() - lastWarn > 2000) {
    Serial.print("[SAFETY] WARN: Irms="); Serial.print(_lastIrms, 2);
    Serial.println("A (high)");
    lastWarn = millis();
  }

  return true;
}

// ---------------------------------------------------------------------------
bool isShutdown() { return _shutdown; }

// ---------------------------------------------------------------------------
void reset() {
  _shutdown = false;
  _reason = "";
  _ocCount = 0;
  _stallActive = false;
  _peakIrms = 0;
  Serial.println("[SAFETY] Shutdown state cleared.");
}

// ---------------------------------------------------------------------------
float getLastIrms() { return _lastIrms; }
const char* getShutdownReason() { return _reason; }

// ---------------------------------------------------------------------------
void printStatus() {
  Serial.println("\n[SAFETY] Status:");
  Serial.print("  Shutdown: "); Serial.println(_shutdown ? "YES" : "No");
  if (_shutdown) {
    Serial.print("  Reason: "); Serial.println(_reason);
  }
  Serial.print("  Last Irms: "); Serial.print(_lastIrms, 3); Serial.println("A");
  Serial.print("  Peak Irms: "); Serial.print(_peakIrms, 3); Serial.println("A");
  Serial.print("  Limits: max="); Serial.print(_cfg.max_current_A, 1);
  Serial.print("A  stall="); Serial.print(_cfg.stall_current_A, 1);
  Serial.print("A  warn="); Serial.print(_cfg.warn_current_A, 1); Serial.println("A");
  Serial.print("  Check count: "); Serial.println(_checkCount);
}

} // namespace Safety
