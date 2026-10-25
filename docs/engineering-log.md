# Engineering Log

Entry format: date, phase, wiring, firmware version, readings, result, issues, next step. Only what was actually
observed is recorded. Corrections to earlier claims are listed at the end. Details of each phase: `docs/phases/`;
raw captures: `docs/logs/`.

---

## 2026-10-10 - Phase 0: hardware identification (status updated 2026-10-25)
![Hardware overview](images/phase0_hardware_overview.png)

- Photographed the ESP32 (front/back) and the parts layout.
- ESP32 back silkscreen: "ESP32 DEVKIT V1 TYPEC"; 30-pin header. Front labels read: left EN, VP, VN, D34, D35, D32,
  D33, D25, D26, D27, D14, D12, D13, GND, VIN; right D23, D22, TX0, RX0, D21, D19, D18, D5, TX2, RX2, D4, D2, D15,
  GND, 3V3.
- A 2-channel relay module is in the parts pile; not part of this project and stays unconnected.
- MPU6050 close-up photos skipped; the sensor was verified electrically in Phase 2.
- Verified later (see phases): ACS712 variant (chip marking, Phase 5), LM393 type (slot sensor, Phase 6), DS18B20
  marking and pinout (Phase 3), L298N labels and jumpers (Phase 4).
- **Still not verified:** battery pack voltage (no multimeter), barrel adapter polarity (power LED lit, not measured),
  label of the cap beside the L298N power terminal (assumed 5V-enable).

## 2026-10-10 - Phase 1: ESP32 setup (DONE)
![Phase 1 serial output](images/phase1_esp32_serial_test.png)
- Board: ESP32 DevKit V1 (USB-C). Chip: ESP32-D0WD-V3, revision 301, 2 cores, 240 MHz, 4 MB flash.
- Toolchain: Arduino IDE on Ubuntu, board "ESP32 Dev Module", 115200 baud.
- Firmware: `firmware/phase1_esp32_test` v0.1. Wiring: USB only.
- Result: chip info printed; counter and LED state toggle every 1000 ms (observed up to t = 165056 ms).
- Open: `lsusb` output (USB-serial chip), visual confirmation of the LED blink.

## 2026-10-10 - Phase 2: IMU over I2C (DONE)
- Wiring: VCC -> 3V3, GND -> GND, SDA -> GPIO 21, SCL -> GPIO 22 (USB power only).
- Firmware: phase2_i2c_scan v0.1, phase2_mpu6050_read v0.2.
- WHO_AM_I = 0x70 (MPU-6500 class, sold as MPU6050), address 0x68.
- Flat az about +1.258 g; upside down az about -0.763 g; tilt (about 45 deg about Y): ax about -0.787 g, az about +0.920 g.
- Raw |a| ranged 0.78-1.27 g across orientations, so per-axis calibration was needed. Gyro bias about +1.1 / +1.7 /
  -0.4 deg/s; rotation test passed.
- The offset/scale estimates made here (Z offset about +0.25 g, X offset about -0.13 g, X scale about 10 % low) were
  wrong; see Phase 7A and the corrections list.
- Details: `docs/phases/phase2_imu.md`. Next: Phase 3.

## 2026-10-10 - Phase 3: DS18B20 (DONE)
- Wiring: DQ on GPIO 4 with a 10 kOhm pull-up to 3V3, VDD on 3V3, GND to GND (not parasite powered).
- Firmware: phase3_ds18b20_test v0.1.
- Result: 1 device, family 0x28, no -127 / 85.00 errors. Rest 28.06-28.19 C; finger pinch peak 31.81 C; 29.44 C about
  16 s after release. 12-bit steps of 0.0625 C. Cooling time constant roughly 12-15 s (one run, still air).
- Limits: no reference thermometer, so absolute accuracy unverified.
- Later: the sensor broke during Phase 7 (cause not given) and is not in the telemetry.
- Next: Phase 4.

## 2026-10-11 - Phase 4: motor + L298N (DONE)
- Wiring: motor on OUT1/OUT2; 6 AA to +12V/GND; ESP32 VIN to the +5V terminal; common GND; GPIO 25/26/27 to
  ENA/IN1/IN2. ENA cap and 5V-enable cap removed.
- Firmware: phase4_motor_test v0.1 (f/r/s, +/-, 3 s auto-stop).
- Result: forward, reverse, stop and auto-stop work. Lowest starting duty 145 (10-step grid, six fresh AA, voltage not
  measured). Motor spins directly from 4 AA and through the L298N.
- Issues: the first hum was the old Phase 3 sketch still uploaded (GPIO 25/26/27 floating). A by-hand test using the
  board's own 5 V did not turn the motor; it worked once the logic 5 V came from ESP32 VIN with the 5V-enable cap removed.
- Six fresh AA were used instead of four because the motor felt weak on four (about 9 V nominal into a 6 V motor, voltage
  unmeasured: a risk accepted knowingly). The motor was reported not to heat up (not measured).
- Details: `docs/phases/phase4_motor_l298n.md`; raw capture `docs/logs/phase4_run1_serial.txt`.

## 2026-10-14 to 2026-10-15 - Phase 5: ACS712 current sensing (DONE, uncalibrated)
- Wiring: ACS712 VCC -> ESP32 VIN (5 V), GND -> GND, OUT -> 10 kOhm -> GPIO 34. 5B: sensor in series in the motor
  supply (battery + -> ACS712 -> L298N +12V).
- Firmware: phase5_acs712_idle v0.1, phase5_acs712_motor v0.1. Python logger/analyzer built in `tools/`.
- 5A (motor off): offset about 2578-2587 mV, std 15-18 mV per line, drift about -7 mV over a minute.
- 5B run 1: the sketch's boot offset (about 2500 mV) was wrong, so its delta and amps columns were invalid.
  Recomputed against the rest level (about 2582 mV).
- 5B run 2 (zeroed with `z` at rest, battery on): duty 145 about -11 mV (about 0.11 A), duty 255 about -16.6 mV (about
  0.17 A), first line of a run about -41 mV (about 0.41 A, 200 ms average). Amps are datasheet scaling only.
- Result: run vs stop clearly detectable; duty 145 vs 255 not reliably separable (5-6 mV apart, run-to-run 3-5 mV).
- Limits: 20 A range on a motor of about 0.1-0.2 A gives poor resolution; no reference meter.
- Details: `docs/phases/phase5_acs712_current.md`.

## 2026-10-17 to 2026-10-18 - Phase 6: LM393 speed sensor (CLOSED WITH LIMITS)
- Wiring: LM393 VCC -> 3V3, GND -> GND, D0 -> GPIO 18 (the colour-to-pin mapping was never typed out).
- Firmware: phase6_lm393_rpm v0.1/v0.2 (rising edges, superseded), phase6b_pulse_width v0.1, phase7b_baseline v0.5
  (completed pulses).
- Issues found: at motor speed the rising-edge counter reported about 800-1000 per second even with the blade removed;
  a filter of 50 us or more removed them, so they were glitches. The first bar-shaped target triggered the sensor only
  rarely. A cardboard disc with two opposite cut sectors produced real pulses; black tape gave a small gain only.
- Result (completed pulses, taped disc, 6 AA): about 134 / 186 / 273 pulses/s at duty 145 / 195 / 255.
  Later check (v0.5): about 200-210 pulses/s at duty 195 and 216-290 at duty 255 (25 % apart between two runs).
- Not met: pulses per revolution never verified (2 assumed); no reference tachometer.
- Details: `docs/phases/phase6_lm393_rpm.md`; raw logs `docs/logs/phase6*.txt`.

## 2026-10-17 to 2026-10-18 - Phase 7: IMU calibration and baselines (CLOSED PARTLY)
- Firmware: phase7a_imu_calibration v0.1, phase7b_baseline v0.1-v0.5. IMU taped on the motor body.
- 7A six-position calibration: ACC_OFFSET = {-0.0448, 0.0018, 0.2771} g, ACC_SCALE = {0.9876, 0.9947, 0.9905}.
  Verify poses: corrected |a| 1.0277 and 1.0098 g (raw 0.917 and 1.155). Gyro bias about +1.10 / +1.80 / -0.85 deg/s,
  not constant between poses.
- 7B motor off: n about 500 samples per window, err 0, vib_rms_g about 0.005 g, amean_g about 1.00. The IMU mount
  shifted at times (amean_g 1.05-1.11, vib_rms_g about 0.011 g).
- 7B motor running: I2C reads failed (up to 205 errors per window, amean_g 0.15-0.9 g), so those vibration values are
  not measurements. Lowering the I2C clock to 100 kHz did not help; IMU sampling did not change the pulse count.
- The DS18B20 broke in this phase; temperature omitted.
- Not met: motor-running vibration baseline. Untried: 100 nF capacitor across the motor, moving the IMU, twisted/short
  I2C wires, separate sensor supply.
- Details: `docs/phases/phase7_imu_baseline.md`.

## 2026-10-19 - Phase 8: MQTT telemetry, SQLite, API (DONE, commit 525a312)
- Setup: Mosquitto on the laptop (LAN listener, password, anonymous refused); ESP32 firmware phase8d_telemetry v0.1 ->
  v0.2 publishing JSON every 500 ms; `backend/consumer.py` v0.2 -> SQLite; `backend/api.py` v0.1 read-only.
- Results: `pub-ok` every window, `seq` continuous, no resets with Wi-Fi on, auto-stop worked. Speed at duty 195 about
  200-212 pulses/s both directions. Motor-off vib_rms_g about 0.005 g, n = 420, err 0. Vibration while the motor runs
  still invalid (n 233-376, err 31-67). Current not usable with Wi-Fi on.
- Issues: v0.1 never set `vib_valid` because the vib_n threshold (450) came from the old loop; v0.2 uses 400. `z`
  failed on a partly filled window; v0.2 uses the last full window. Rest amean_g read 1.126 in one session (not
  investigated).
- Details: `docs/phases/phase8_telemetry.md`.

## 2026-10-20 and 2026-10-24 - Phase 9: React + TypeScript dashboard (DONE, commits 02b8473, c5660b3)
- Built `frontend/` (Vite, React, TypeScript, Recharts). 2026-10-20: tested on 359 stored rows only.
- 2026-10-24 live test (motor off): Device online, "last data 1 s ago", `pub-ok` every window, charts extended without a
  page refresh, Device card went offline about 20 s after unplugging (one test).
- Issues: the API returns the newest `limit` rows, not the oldest (first guess wrong; limit raised to 600); Vite hot
  reload kept old state until a hard refresh; the first consumer start used a placeholder password ("Not authorized").
- Valid motor-off windows showed short spikes up to about 0.5 g (cause unknown).
- Limit: only the newest 600 rows are kept, so older sessions scroll out of the charts.
- Details: `docs/phases/phase9_dashboard.md`, `docs/images/phase9_dashboard_live.png`.

## 2026-10-25 - Phase 10: threshold condition rules and final documentation
- Added `backend/rules.py` (4 threshold rules), `replay_rules.py`, 9 unit tests, `/api/anomalies` (api.py v0.2) and a
  "Condition rules" dashboard card. Thresholds were read from the stored data.
- Replay over 834 stored rows (2026-10-19 to 2026-10-24): NO_SPEED_WHILE_COMMANDED 5 episodes (all five commanded "fwd"
  runs of 2026-10-24, which had 0 pulses; none on 2026-10-19); VIBRATION_AT_REST 5 episodes (all 2026-10-24, none on the
  quiet 2026-10-19 session); SPEED_WHILE_STOPPED 0 (no real event in the data); DATA_STALE 7 gaps.
- Live check with the motor stopped: with the consumer off the card showed one DATA_STALE alert (device offline, last
  data 18.1 h ago); with the consumer running, device online and data 1 s old, the card showed "no active alerts".
  Screenshots:
  `docs/images/phase10_rules_card_stale.png`, `phase10_rules_card_live.png`.
- Observed in the same serial capture: amean_g read 1.044 and then 2.299 at rest within about two minutes, so
  `vib_valid` was 0 (cause not investigated; the quality gate worked).
- No live alert on the motor rules has been observed on real hardware: the rules were checked on stored data and
  synthetic tests.
- Documentation update: README, architecture, limitations, hardware inventory, safety notes, this log.
- Details: `docs/phases/phase10_anomaly_rules.md`.

---

## Corrections made along the way
- Phase 0 entry (2026-10-10) said the battery voltage, adapter polarity and LM393 type were "verified now". They were
  not; corrected on 2026-10-25.
- Phase 2: offset/scale estimates from three poses (Z offset +0.25 g, X offset -0.13 g, X scale 10 % low) were wrong;
  replaced by the six-position calibration (Phase 7A).
- Phase 5: the first 5B run used a wrong boot offset (about 2500 mV vs the real rest level about 2582 mV); its delta
  and amps columns were discarded and recomputed.
- Phase 6: the first version reported RPM from rising-edge counts at motor speed; those values were glitches and are
  invalid. "The black tape was the fix" was wrong (the disc was). "About 2 % repeatability" applied to one pair of runs
  only; a later check showed 25 % between two runs.
- Phase 7: the Phase 7B vibration values while the motor ran were not measurements (failed I2C reads).
- Phase 9: the first guess that the API returned the oldest rows was wrong; it returns the newest.
