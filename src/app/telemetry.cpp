// =============================================================================
//  app/telemetry.cpp — Heartbeat and Telemetry
// =============================================================================
#include "telemetry.h"
#include "../config.h"
#include "../driver/drv8316_helper.h"
#include "../mcu/current_sense.h"
#include "../safety/safety.h"
#include <SimpleFOC.h>

extern BLDCMotor motor;
extern bool motorRunning;
extern bool logMode;
extern bool showIdIq;
extern int opMode;
extern float manual_target;
extern float target;

namespace Telem {

// ---------------------------------------------------------------------------
void heartbeat(unsigned long now_ms) {
  static unsigned long lastHB = 0;
  static bool faultSeen = false;

  if (!motorRunning) {
    // 2-second heartbeat when idle
    if (now_ms - lastHB >= 2000) {
      lastHB = now_ms;
      bool nf = (!DRV::isFaulted());
      Serial.print("[HB] t="); Serial.print(now_ms / 1000);
      Serial.print("s nF="); Serial.print(nf ? "OK" : "FAULT");
      Serial.print(" ang="); Serial.print(motor.shaft_angle, 2);
      if (Safety::isShutdown()) {
        Serial.print(" SAFETY:"); Serial.print(Safety::getShutdownReason());
      }
      Serial.println();
      faultSeen = !nf;
    }
  } else {
    // Running: minimal output — only on fault
    if (!faultSeen && DRV::isFaulted()) {
      Serial.println("[!] nFAULT LOW during operation!");
      faultSeen = true;
    }
    if (faultSeen && !DRV::isFaulted()) {
      faultSeen = false;
    }
  }
}

// ---------------------------------------------------------------------------
void csvLog(unsigned long now_us) {
  static unsigned long lastLog = 0;
  if (!logMode || !motorRunning) return;
  if (now_us - lastLog < 5000) return; // 200Hz for step response capture

  lastLog = now_us;

  float angle_deg = motor.shaft_angle * (180.0f / PI);
  float target_deg = target * (180.0f / PI);
  float error_deg = target_deg - angle_deg;

  Serial.print(now_us / 1000);       Serial.print(",");   // ts_ms
  Serial.print(target_deg, 2);       Serial.print(",");   // target (deg)
  Serial.print(angle_deg, 2);        Serial.print(",");   // angle (deg)
  Serial.print(error_deg, 3);        Serial.print(",");   // error (deg)
  Serial.print(motor.shaft_velocity, 1); Serial.print(","); // velocity (rad/s)
  Serial.println(motor.voltage.q, 2);                      // Vq (V)
}

// ---------------------------------------------------------------------------
void printFullStatus() {
  Serial.println("\n--- Motor Status ---");
  Serial.print("vel="); Serial.print(motor.shaft_velocity, 2);
  Serial.print(" ang="); Serial.print(motor.shaft_angle, 3);
  Serial.print(" Vq="); Serial.print(motor.voltage.q, 3);
  Serial.print(" Vlim="); Serial.print(motor.voltage_limit, 1);
  Serial.print(" en="); Serial.println(motorRunning ? "Y" : "N");

  Serial.print("Mode="); Serial.print(opMode);
  Serial.print(" TGT="); Serial.println(manual_target, 2);

  Serial.print("PID: P="); Serial.print(motor.PID_velocity.P, 3);
  Serial.print(" I="); Serial.print(motor.PID_velocity.I, 3);
  Serial.print(" ramp="); Serial.println(motor.PID_velocity.output_ramp, 1);

  // Safety
  Safety::printStatus();

  // DRV8316 fault
  DRV::printFaultStatus();
}

// ---------------------------------------------------------------------------
void printMotorState(float e_angle) {
  if (!showIdIq || !motorRunning) return;

  static unsigned long lastIdIq = 0;
  if (micros() - lastIdIq < 100000) return; // 10Hz
  lastIdIq = micros();

  float id, iq;
  CS::readDQ(id, iq, e_angle);
  Serial.print("[IQ] Id="); Serial.print(id, 3);
  Serial.print(" Iq="); Serial.print(iq, 3);
  Serial.print(" Irms="); Serial.println(Safety::getLastIrms(), 3);
}

} // namespace Telem
