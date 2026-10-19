# Phase 8: MQTT telemetry, SQLite storage and a read-only API (closed 2026-10-19)

## Objective
Move the rig's measurements off the USB cable: ESP32 -> Wi-Fi -> MQTT broker -> Python consumer -> SQLite -> FastAPI.
Only fields with a stated validity are published; weak fields carry flags instead of being hidden.

## Architecture
ESP32 (`firmware/phase8d_telemetry`) publishes JSON to `iiot/rig1/telemetry` every 500 ms (not retained) and a retained
`iiot/rig1/status` (online/offline, offline set by the MQTT Last Will). Mosquitto runs on the laptop (LAN, user `rig1`
with password, anonymous access refused). `backend/consumer.py` stores every message raw in `messages` and telemetry
typed in `telemetry`. `backend/api.py` serves the data read-only on 127.0.0.1:8000. SQLite is used first; PostgreSQL is
still the planned database if the project needs it.

## 8A / 8B-1: broker
Mosquitto installed from apt. Local pub/sub test passed (the wildcard `iiot/test/#` did not match `iiot/rig1/telemetry`).
`/etc/mosquitto/conf.d/lan.conf`: `listener 1883`, `allow_anonymous false`, `password_file /etc/mosquitto/passwd`.
Test through the LAN IP: authenticated publish arrived, anonymous publish was refused ("not authorised").
Limit: plain MQTT, the password travels unencrypted; acceptable only on a trusted home LAN. Port 1883 must not be forwarded.

## 8B-2: ESP32 publishes (`phase8b_mqtt_hello` v0.1, logs `phase8b_serial.txt`, `phase8b_subscriber.txt`)
ESP32 IP 192.168.1.8, broker 192.168.1.10. `seq` continuous 28-40, Last Will `offline` appeared after unplugging, `online`
and `seq` 0 after replugging. RSSI -63 to -74 dBm (usable, weak side). The paste of the subscriber output started at seq 28.

## 8C: consumer v0.1 (`phase8c_db_check.txt`)
10 `hello` rows, seq 0-9, none lost.

## 8D: telemetry firmware (`phase8d_telemetry` v0.1 and v0.2; logs `phase8d_v01_run1_serial.txt`, `phase8d_v02_run2_serial.txt`)
Built on `phase7b_baseline` v0.5 (completed-pulse speed, calibrated IMU) plus Wi-Fi, MQTT and the ACS712 on GPIO 34.
Payload fields: device_id, fw, seq, uptime_ms, run, motor_dir, duty, settling, pulses_per_s, rpm, ppr_assumed,
current_dmv, vib_rms_g, vib_n, vib_err, amean_g, vib_valid, rssi. No temperature (DS18B20 broken).
Safety design: reconnects are never attempted while the motor runs, so a slow network cannot delay the 3 s auto-stop.
`mqtt.setBufferSize(512)` is required (PubSubClient default of 256 bytes is too small).

Validity rules: `vib_valid` = IMU on, not `settling`, vib_n >= 400, vib_err = 0, 0.97 <= amean_g <= 1.03.
`rpm` = pulses_per_s * 60 / ppr_assumed; ppr_assumed = 2 was never verified. `current_dmv` is a mV change from a rest
level set with `z` (null until set), uncalibrated, not amps.

v0.1 bugs found from the first run and fixed in v0.2:
1. The vib_n threshold of 450 came from the old loop. Adding the ADC read to the 1 kHz tick lowered the motor-off
   n to a steady 420-424 per window (cause is an estimate, not measured), so `vib_valid` was never true. Threshold now 400.
2. `z` used the partly filled current window and failed ("not enough samples"). v0.2 uses the last full window.

## Results (observed)
- Delivery: `pub-ok` on every window, seq continuous, no resets with Wi-Fi on, auto-stop worked forward and reverse.
- Speed at duty 195: about 200-212 completed pulses per second forward and reverse (consistent with Phase 6/7).
- Vibration while the motor runs: still invalid (n 233-376 of about 420, err 31-67 per window), as in Phase 7. The flag reports it.
- Motor off: vib_rms_g about 0.005 g, n = 420, err = 0. In the v0.2 run amean_g was 1.060, then 1.126 after the first
  run, so `vib_valid` stayed 0 (correct: the quality gate rejected it). After a fresh power-up amean_g was 1.00 and
  `vib_valid` was 1 (API check). Note: with a valid calibration the magnitude of gravity should read 1.00 in any
  orientation, so a stable 1.126 points to a changed calibration (offset drift, board stress, temperature) and not only a
  shifted mount. Not investigated.
- **Current is not usable with Wi-Fi on.** After `z` (rest 2443.6 mV; Phase 5 rest was about 2582 mV) the motor-off value
  scattered by about +/-8 mV and later drifted to -25 mV; running values ranged from -0.2 to -63 mV with spikes of -50
  and -63 mV. Run and stop overlap; Phase 5 (Wi-Fi off) separated them. Cause unproven (ADC noise with Wi-Fi active is one
  candidate). An earlier run (seq 62-76) showed motor-off values between -43 and -166 mV against -160 to -225 mV running.
  No conclusion about amps may be drawn from this data.

## 8E: typed table and API (`backend/consumer.py` v0.2, `backend/api.py` v0.1, `backfill.sql`; log `phase8e_api_check.txt`)
Backfill copied 129 stored telemetry rows (seq 0-76 across several boots, so seq is not unique) into `telemetry`.
Endpoints: `/api/health`, `/api/status`, `/api/latest`, `/api/telemetry?limit&after_id` (oldest first). Responses
carry notes about assumed RPM, uncalibrated current, vibration validity and the missing temperature.
Verified: status offline/online follows the device; `seq` and `received_at` advance in `/api/latest` while the ESP32 runs.
Observation for Phase 9: `/api/latest` returns the last stored row even if it is minutes old, so the dashboard must
compare `received_at` with the current time and the device status and show "stale".
The `received_at` timestamp is the laptop's clock (the ESP32 has no clock).

## Limits
Plain-text MQTT on a LAN; API on localhost only, no authentication; SQLite single file; sampling slower than in Phase 7
(about 420 instead of 500 IMU samples per window); current, RPM and running vibration are not trustworthy.

## Open items
Motor-noise fixes (rigid IMU mount, twisted/short I2C wires, 100 nF across the motor), IMU calibration check, ADC noise
with Wi-Fi, pulses per revolution, battery voltage and ACS712 calibration (need a multimeter), DS18B20 replacement.
