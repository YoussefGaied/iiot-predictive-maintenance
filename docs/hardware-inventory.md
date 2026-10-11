# Hardware Inventory

"Verified" means checked on the actual board. Do not wire based on
appearance alone.

| Component | Owned | Verified | Notes |
|---|---|---|---|
| ESP32 DevKit V1 (USB-C) | Yes | Chip info read 2026-10-10 | ESP32-D0WD-V3 rev 301, 4 MB flash, LED on GPIO 2 |
| L298N motor driver | Yes | No | Check terminal labels, enable jumper, regulator jumper. Significant voltage drop. |
| Mini 6 V DC motor "310DC" | Yes | No | Avoid stalls and prolonged runs. |
| ACS712 current sensor | Yes | No | Variant (5/20/30 A) to be read from chip marking. 20 A range may give poor resolution for this motor. |
| MPU6050 / GY-521 | Yes | Partly (I2C, WHO_AM_I) | Sold as MPU6050. WHO_AM_I = 0x70 (MPU-6500 class; genuine MPU6050 = 0x68), address 0x68. Accel/gyro read works. Uncalibrated: estimated offsets about -0.13 g (X) and +0.25 g (Z). Chip marking not read. |
| DS18B20 (TO-92) | Yes | Yes (ROM family 0x28) | Marked "18B20 2501C2+ 007AG". Pinout per datasheet: 1 GND, 2 DQ, 3 VDD. Works with a 10 kOhm pull-up on a short bench setup; 4.7 kOhm recommended for longer wires. Absolute accuracy not verified against a reference. |
| LM393 speed sensor module | Yes | No | Appears to be a slot (fork) type from the photo; unconfirmed. Needs a target on the shaft. |
| 6×AA battery holder with barrel lead | Yes | No | Fresh alkaline cells can reach ~9 V. Not safe to assume for the 6 V motor. |
| Breadboard, jumper wires | Yes | n/a | |
| 10 kOhm resistor | Yes | n/a | DS18B20 normally uses 4.7 kOhm pull-up. |
| Barrel jack to screw-terminal adapter | Yes | No | Verify size and polarity. |

Possible extras: 4.7 kOhm resistor, multimeter, motor mount, shaft target
for RPM sensing.

## 2026-10-10 - Phase 3: DS18B20 temperature sensor (DONE)
- Wiring: GND->GND, DQ->GPIO4 with 10 kOhm pull-up to 3V3, VDD->3V3 (USB power only, MPU6050 disconnected)
- Marking: "18B20 2501C2+ 007AG"; pinout per datasheet (1 GND, 2 DQ, 3 VDD)
- Firmware: phase3_ds18b20_test v0.1
- Result: 1 device, family code 0x28, not parasite powered, no -127 / 85.00 errors
- Rest ~ 28.06-28.19 C; finger pinch peak 31.81 C; fell to 29.44 C within ~16 s after release
- Readings are multiples of 0.0625 C (12-bit step)
- Cooling time constant roughly 12-15 s (rough, single run, still air)
- Limitations: no reference thermometer; thermal lag; 4.7 kOhm pull-up recommended for longer wires
- Details: docs/phases/phase3_ds18b20.md; raw captures: docs/logs/phase3_run*.txt
-  Phase 4, motor + L298N :done : terminals OUT1/OUT2/+12V/GND/+5V/OUT3/OUT4, edge pins ENA/IN1-IN4/ENB; 5V jumper removed and ENA cap removed; driven from ESP32
Motor: it spins directly from 4 AA and from the L298N.
Battery holder: 6 AA slots, series, voltage not measured.
Barrel adapter: the L298N power LED lit, polarity not measured.
