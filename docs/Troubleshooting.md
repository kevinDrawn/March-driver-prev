# Troubleshooting Guide

## Quick Reference: nFAULT LED Status

| LED State | Meaning | Action |
|-----------|---------|--------|
| **OFF** (nFAULT=HIGH) | Normal operation | None |
| **Solid ON** | Latched fault (OTS, OVP, or SPI error) | Run `S` command, check fault register |
| **Flickering** | Repeated OCP triggers (ringing noise) | Expected in jumper wire setup; see §1 |
| **Blinks once on enable** | Alignment current triggers OCP | Normal during `initFOC()`; clears automatically |

---

## 1. nFAULT Triggers During Normal Operation

### Symptom
Motor runs briefly, then nFAULT LED lights up and motor stops or stutters.

### Diagnosis Steps

```
1. Stop the motor:              d
2. Read fault register:         S
3. Look at the fault flags in the output:
   - OTS=1 → Over Temperature (see §2)
   - OCP_HA/LA/HB/LB/HC/LC → Over Current (see §3)
   - OVP=1 → Over Voltage (check power supply)
   - SPI=1 → SPI communication error (check wiring)
   - BUCK=1 → Buck regulator fault (check 12V supply)
4. Clear the fault:             C
5. Check if nFAULT clears:      (output shows "nF=OK" or "nF=FAULT")
```

### If Fault Persists After Clear
- **OTS**: DRV8316 die is still hot. Wait 30+ seconds for cooling, then retry.
- **SPI**: Check all 4 SPI wires (SCK, MISO, MOSI, CS). Loose jumper wires are the #1 cause.
- **POR (Power On Reset)**: DRV8316 lost power momentarily. Check 12V supply stability.

---

## 2. Over Temperature Shutdown (OTS)

### Symptom
Motor runs for 10~30 seconds at moderate speed, then OTS=1 triggers.

### Root Cause (Jumper Wire Environment)
In the current setup, the ~15cm jumper wires have a parasitic inductance of ~183nH. At every PWM switching edge (600,000 times/second), this inductance causes voltage ringing of **28.7V amplitude**. The ringing current flows through the DRV8316's FETs, adding heat beyond what the DC current model predicts.

**Key evidence**: The same EVM board reached **100 rad/s** when directly connected to a C2000 LaunchPad (no jumper wires). With jumper wires, OTS occurs at **40 rad/s**.

### Mitigation (Software)

1. **Use voltage mode** (already default): `motor.torque_controller = TorqueControlType::voltage`. This bypasses the noisy ADC.

2. **Dynamic voltage clamp** (already active in m2): Limits current to I_MAX_SAFE (1.5A).

3. **Reduce target speed**: Stay below 30 rad/s for sustained operation.

4. **Use acceleration ramp**: The firmware uses `cl_accel = 2 rad/s²`. Don't increase this — faster acceleration means sustained high current.

### Permanent Fix
Migrate to the custom PCB. Short traces (≤5mm) reduce parasitic inductance to ~5nH, eliminating ringing-induced heating.

---

## 3. Over Current Protection (OCP) False Trips

### Symptom
nFAULT LED flickers rapidly. Motor may produce a scratching/grinding sound.

### Root Cause
Jumper wire ringing causes instantaneous current spikes that the DRV8316's internal comparator interprets as overcurrent (>16A), even though actual motor current is only 1~2A.

### Current Configuration
OCP is set to `NoAction` — the DRV8316 ignores OCP events entirely. This prevents false shutdowns but means **hardware overcurrent protection is disabled**. The dynamic voltage clamp provides software-level protection instead.

### How to Verify It's Noise (Not Real Overcurrent)

```
1. Set OCP to ReportOnly:    (requires code change)
2. Run motor at 30 rad/s
3. If nFAULT LED flickers but motor runs fine → noise, not real OCP
4. Set OCP to NoAction:      (already set)
5. Run same test → LED should stay OFF
```

### AutoRetry Sound Test
If you temporarily set OCP to `AutoRetry_Fault` mode, you'll hear a distinctive scratching sound — this is the DRV8316 chopping the FETs on/off at 50kHz as it detects and recovers from OCP on every PWM cycle.

### On Custom PCB
Revert OCP to `AutoRetry_Fault` — with short traces, ringing will be small enough that OCP only triggers on genuine overcurrent events.

---

## 4. Motor Doesn't Move After `e` (Enable)

### Check 1: Is `nFAULT` OK?

```
S       ← Check status
```

If nFAULT is LOW, a fault is preventing operation. Clear it with `C` and re-enable.

### Check 2: Is a target set?

```
m2      ← Switch to velocity closed-loop
t10     ← Set target to 10 rad/s
```

After mode switch, target resets to 0. You must set a target.

### Check 3: Is the mode correct?

In position mode (`m3`), target is relative to the entry position. `t0` means "stay here". You need `t3.14` or similar to see movement.

### Check 4: Voltage limit too low?

```
S       ← Check Vlim
V6      ← Set to 6V if it's too low
```

In open-loop modes, voltage limit is automatically set to 1.5V. If you switch from `m0` to `m2` without explicit `V` command, the limit stays at 1.5V — not enough for 30+ rad/s operation.

### Check 5: DRV8316 sleeping?

The firmware sets `nSLEEP=HIGH` at boot, but if the wire is loose, DRV8316 enters sleep mode. Check the nSLEEP jumper wire connection.

---

## 5. Motor Vibrates But Doesn't Spin

### Cause: FOC Alignment Failed

If `initFOC()` couldn't determine the correct zero electrical angle, the motor will vibrate instead of rotating smoothly.

### Diagnosis

```
D       ← Run diagnostics
```

Check:
- `zero_electric_angle`: Should be a consistent value between 0 and 2π. If it's wildly different each boot, the alignment is failing.
- `sensor_direction`: Should be consistently CW or CCW. If it changes randomly, the sensor signal is noisy.

### Fix
1. Ensure MAQ473 magnet is centered and within range: diagnostics will show `Magnet: IN RANGE`
2. Increase alignment voltage: in `main.cpp`, find `motor.voltage_sensor_align` and increase from default (usually 3V)
3. Run sensor calibration: `c` command after successful alignment

---

## 6. Position Control: Motor Doesn't Hold Position

### Symptom
In `m3`, the motor reaches the target but drifts or oscillates.

### PID Tuning

```
a8      ← Increase position P gain (default 4.0, try 8~12)
p0.3    ← Increase velocity P gain if there's steady-state velocity error
i2.0    ← Increase velocity I gain if there's position offset
f0.02   ← Increase velocity LPF if there's high-frequency oscillation
```

### Mechanical Issues
- **Shaft slipping**: If the load is attached without a D-cut or set screw, the shaft will slip under torque, producing a squealing sound and potentially triggering faults.
- **Loose coupling**: Check all mechanical connections.

---

## 7. ADC / Current Sensing Issues

### Symptom
`I` command shows wild Id/Iq values (>5A) when motor is running slowly.

### Root Cause
In the jumper wire environment, EMI from power wires couples into the analog CSA wires. The ADC reads noise, not real current.

### Why This Doesn't Affect Normal Operation
The firmware uses `TorqueControlType::voltage` — the ADC readings are displayed for diagnostics (`I` command) but are **not used in the control loop**. Motor control is purely voltage-based.

### When ADC Matters
- `foc_current` torque mode uses ADC for real-time current PID → **currently broken** in jumper wire environment
- `estimated_current` mode uses BEMF estimation → **currently broken** due to Ke parameter mismatch
- Motor parameter identification (`R` command) uses ADC → results may be inaccurate

### On Custom PCB
With proper analog routing and ground plane, ADC accuracy improves to ±0.1A, enabling `foc_current` mode for precise torque control.

---

## 8. MotorID Results Don't Match Reality

### Symptom
`R` command reports Ke=0.136, but motor behaves as if Ke≈0.088.

### Explanation
MotorID measures Ke by spinning the motor at a known voltage and measuring the resulting speed. In the jumper wire environment:

1. Voltage drops across jumper wire resistance reduce the effective voltage at the motor
2. ADC noise adds uncertainty to the current measurement
3. The short test duration (3 seconds) may not reach true steady state

### Workaround
Use `config.h` values (derived from multi-speed telemetry analysis):

```c
#define MOTOR_KE      0.088f   // [V/(rad/s)] — from telemetry at 10/20/30 rad/s
#define MOTOR_R_PHASE 0.32f    // [Ω] — from Vq/I at steady state
```

After running `R`, if the values seem off, the motor parameters can be manually reset by re-flashing or power-cycling (MotorID results are not saved to flash).

---

## 9. Calibration (`c`) Produces Bad LUT

### Symptom
After calibration, motor control is worse than before (more vibration, less smooth).

### Possible Causes

1. **Load was attached during calibration**: The motor must rotate freely. Any friction distorts the error measurement. Remove all loads before running `c`.

2. **nFAULT during calibration**: If OCP trips during the slow sweep, some positions will have corrupted readings. Watch the serial output for `n=0` during calibration.

3. **Magnet not centered**: If the MAQ473 magnet is tilted or offset, the nonlinearity exceeds the LUT's correction capability (±2° typical max).

### Recovery
Power cycle the board. The LUT is stored in RAM and will be cleared on reset. The motor will revert to uncalibrated sensor operation.

---

## 10. Serial Commands Not Responding

### Symptom
Typing commands in the serial monitor produces no response.

### Check 1: Baud rate
Must be **115200**. Other rates will show garbled text or nothing.

### Check 2: Line ending
PlatformIO's monitor uses `send_on_enter` filter. You must press **Enter** after each command. Commands are not processed character-by-character.

### Check 3: Motor running?
When the motor is running, serial output is intentionally limited to prevent blocking the FOC loop. The heartbeat prints only every 2 seconds. Commands are still processed, but responses may be delayed.

### Check 4: TX buffer overflow
If `L` (logging) mode is active while the motor is running fast, the 512-byte TX buffer can overflow, causing the FOC loop to stall. Disable logging (`L`) before high-speed operation.

---

## Diagnostic Command Quick Reference

| Situation | Command Sequence |
|-----------|-----------------|
| "Why did the motor stop?" | `S` → read fault flags |
| "Is my hardware connected right?" | `D` → check all subsystems |
| "Is the ADC working?" | `X` → energize phases, read current |
| "What are the motor parameters?" | `R` → auto-measure R, L, Ke |
| "What current is flowing?" | `I` → toggle Id/Iq in heartbeat |
| "I need raw telemetry data" | `L` → 50Hz CSV: timestamp,vel,Vq,nFAULT |
| "nFAULT won't clear" | `C` → double-clear, then `S` to verify |
| "Sensor seems noisy" | `c` → run full LUT calibration |
