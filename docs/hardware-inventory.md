# Hardware Inventory

"Verified" means checked on the actual board. Do not wire based on
appearance alone.

| Component | Owned | Verified | Notes |
|---|---|---|---|
| ESP32 DevKit V1 (USB-C) | Yes | Chip info read 2026-10-10 | ESP32-D0WD-V3 rev 301, 4 MB flash, LED on GPIO 2 |
| L298N motor driver | Yes | No | Check terminal labels, enable jumper, regulator jumper. Significant voltage drop. |
| Mini 6 V DC motor "310DC" | Yes | No | Avoid stalls and prolonged runs. |
| ACS712 current sensor | Yes | No | Variant (5/20/30 A) to be read from chip marking. 20 A range may give poor resolution for this motor. |
| MPU6050 / GY-521 | Yes | No | Headers soldered. Inspect for solder bridges; verify labels and voltage arrangement. |
| DS18B20 (TO-92) | Yes | No | Bare 3-pin sensor. Confirm marking and pinout before powering. |
| LM393 speed sensor module | Yes | No | Appears to be a slot (fork) type from the photo; unconfirmed. Needs a target on the shaft. |
| 6×AA battery holder with barrel lead | Yes | No | Fresh alkaline cells can reach ~9 V. Not safe to assume for the 6 V motor. |
| Breadboard, jumper wires | Yes | n/a | |
| 10 kOhm resistor | Yes | n/a | DS18B20 normally uses 4.7 kOhm pull-up. |
| Barrel jack to screw-terminal adapter | Yes | No | Verify size and polarity. |

Possible extras: 4.7 kOhm resistor, multimeter, motor mount, shaft target
for RPM sensing.
