# Limitations and what would improve them

Written 2026-10-25 from the phase write-ups. "Observed" items come from recorded runs; nothing here is a claim of
accuracy.

| Area | State | What would be needed |
|---|---|---|
| Failure prediction | Not done. No labelled failure data exists, so none can be claimed | Many repeated runs with known, safely induced conditions and a proper train/test evaluation |
| Anomaly rules | 4 threshold rules tuned on 834 rows from two sessions (2026-10-19, 2026-10-24) of this rig; zero false alarms on the quiet session is partly by construction | More sessions, held-out data, and a defined set of conditions to detect |
| Speed | Pulses per second is repeatable (about 200-212 at duty 195, both directions). Run-to-run agreement varied from about 2 % to about 25 % in different checks | Count real turns by hand to verify pulses per revolution; compare with a reference tachometer |
| RPM | Assumes 2 pulses/rev; never verified. With 1 per rev it doubles | Same hand-turn check |
| Vibration, motor off | About 0.005 g RMS on a rigid mount; mount shifts changed it (0.011 g) and short spikes up to 0.5 g occurred (cause unknown) | Rigid mount, repeated sessions |
| Vibration, motor running | **Invalid**: I2C reads fail (n as low as 9 and up to 376 of about 420-500 expected, up to 205 errors per window, mean magnitude 0.15-1.05 g across the runs) | Noise fixes not yet tried: 100 nF capacitor across the motor terminals, IMU a few cm from the motor, shorter/twisted SDA-SCL wires, separate supply |
| IMU calibration | Six-position calibration done (offsets and scales in `docs/phases/phase7_imu_baseline.md`). Rest mean magnitude later drifted to 1.05-1.126 g, and on 2026-10-25 serial lines showed 1.044 then 2.299 g at rest within about two minutes (cause not investigated) | Rigid mount, re-calibrate on the final mount, check after handling |
| Current | Phase 5 (Wi-Fi off, 20 A module): about 0.1-0.2 A, datasheet scaling, uncalibrated; run vs stop detectable, duty levels not separable. With Wi-Fi on: scatter about +/-8 mV, drift to -25 mV, run and stop overlap | Multimeter or reference ammeter for calibration; a 5 A module or a shunt with an amplifier; investigate ADC noise with Wi-Fi |
| Temperature | DS18B20 worked in Phase 3, then broke (cause not given). Never measured against a reference | Replace; check pinout before powering; 4.7 kOhm pull-up |
| Supply voltage | Pack voltage never measured (no multimeter). Six fresh AA cells (about 9 V nominal) run a 6 V motor, a risk accepted knowingly | Multimeter; a regulated supply or a lower-voltage pack |
| Electrical noise | The motor puts microsecond glitches on the speed-sensor output (handled with the completed-pulse method) | See the noise fixes above |
| Security | Plain MQTT with a password on the LAN; API without authentication on localhost | TLS and authentication before any use beyond a home bench |
| Scale | One device, SQLite, one laptop, dev server | Not designed for more |
| Hardware | Small 6 V demo motor, cardboard disc, taped IMU: not industrial equipment | n/a |

## Candidate next steps (not commitments)
1. Buy a multimeter: measure the pack voltage, then calibrate the current sensor.
2. Hand-turn check of pulses per revolution; then RPM can be reported with a verified factor.
3. Try the motor-noise fixes one at a time and re-test until n about 420-500, err 0, mean magnitude about 1.00 while running.
4. Replace the DS18B20 (check the pinout first) and add temperature rules once there is baseline data.
5. Collect more repeated sessions under defined, safe conditions before considering any machine learning.
