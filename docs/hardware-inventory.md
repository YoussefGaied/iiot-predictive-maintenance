# Hardware Inventory

"Verified" means checked on the actual board or by a recorded test. Do not wire based on appearance alone.
Status as of 2026-10-25.

| Component | Owned | Verified | Notes |
|---|---|---|---|
| ESP32 DevKit V1 (USB-C, 30-pin) | Yes | Yes | Back printed "ESP32 DEVKIT V1 TYPEC". ESP32-D0WD-V3 rev 301, 2 cores, 240 MHz, 4 MB flash, LED on GPIO 2. USB-serial chip not identified. Wi-Fi RSSI about -63 to -78 dBm (weak side) |
| L298N driver (red board) | Yes | Yes (Phase 4) | Terminals OUT1, OUT2, +12V, GND, +5V, OUT3, OUT4; edge header ENA, IN1, IN2, IN3, IN4, ENB. ENA cap removed, ENB cap left in place, cap beside the power terminal removed (label unreadable, assumed 5V-enable). Logic 5 V comes from ESP32 VIN into the +5V terminal. Significant internal voltage drop (datasheet: about 2.5 V total at 1 A; not measured here) |
| Mini 6 V DC motor "310DC" | Yes | Yes (Phase 4) | Spins directly from 4 AA and through the L298N. Avoid stalls and long runs. Lowest duty that starts it from rest: 145 (10-step grid, one pack) |
| ACS712 current module, green, screw terminal | Yes | Yes (Phase 5) | Chip marking "ACS712TELC-20A25331": 20 A variant, 100 mV/A (datasheet). Pins VCC, OUT, GND. Reads as a drop in both directions (terminals opposite to IP+ -> IP-). **Uncalibrated**: no reference meter. Too noisy with Wi-Fi on |
| MPU6050 / GY-521 | Yes | Partly | I2C 0x68, WHO_AM_I 0x70 (MPU-6500 class; a genuine MPU6050 reads 0x68). Headers soldered, works. Chip marking and pin labels never read. Six-position calibration done in Phase 7A |
| DS18B20, bare TO-92 | Yes | **Broken** | Marked "18B20 2501C2+ 007AG", family 0x28; worked in Phase 3 on GPIO 4 with a 10 kOhm pull-up. Broke during Phase 7 (cause not given). Not in telemetry. A replacement needs its pinout checked before power |
| LM393 speed module, blue, slot (fork) type, 4-pin header | Yes | Yes (Phase 6) | Header A0, D0, GND, VCC; A0 unused; slot gap about 5-6 mm (builder's estimate). VCC on 3V3, D0 on GPIO 18 |
| Target disc on the motor shaft | Made | Final version | Cardboard disc about 25-30 mm, two opposite sectors cut away, black tape on the sectors. Pulses per revolution **not verified** (2 assumed) |
| 6xAA battery holder with barrel lead | Yes | Partly | Slot contacts continuous (series). Six fresh AA cells, no bridge wires. **Voltage never measured** |
| Barrel jack to screw-terminal adapter | Yes | Partly | L298N power LED lights when connected; polarity not measured |
| Breadboard, jumper wires | Yes | n/a | |
| 10 kOhm resistor | Yes (one) | n/a | Used as the DS18B20 pull-up (4.7 kOhm is standard) and as the ACS712 series resistor |
| 4.7 kOhm resistor | No | n/a | Needed for a replacement DS18B20 |
| Multimeter | No | n/a | Would measure the battery voltage and allow current calibration |
| 100 nF ceramic capacitor | Unknown | n/a | Never confirmed; one of the untried motor-noise fixes |
| LED | No | n/a | |
| 2-channel relay module | Yes | n/a | In the parts pile, **not part of this project**; keep unconnected |

## Pins in use

| Function | GPIO / pin |
|---|---|
| I2C SDA / SCL (IMU) | 21 / 22 |
| 1-Wire DQ (DS18B20, broken) | 4 |
| L298N ENA (PWM) / IN1 / IN2 | 25 / 26 / 27 |
| L298N logic 5 V / ground | VIN / GND |
| ACS712 OUT through 10 kOhm | 34 |
| LM393 D0 | 18 |
| Onboard LED | 2 |

## Evidence still missing
Photos for `docs/images/`: wiring for Phases 4 and 5, serial screenshots for Phases 4 and 5, the target disc next to
the sensor (Phase 6), the IMU mount (Phase 7), Phase 8 broker/consumer screenshots.
