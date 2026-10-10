# Phase 3: DS18B20 temperature sensor

**Status:** Done (2026-10-10)

## Objective

Read a plausible temperature from the DS18B20 over the 1-Wire bus and
confirm the part identifies itself as a DS18B20 through its ROM family
code, with the motor and all other sensors disconnected.

## Hardware

- ESP32 DevKit V1 (USB-C), USB power only
- DS18B20, bare TO-92 package. Marking read: "18B20 2501C2+ 007AG".
  Pinout per datasheet: pin 1 GND, pin 2 DQ, pin 3 VDD.
- 10 kOhm pull-up resistor (the standard value is 4.7 kOhm; see Limitations)

## Wiring

| DS18B20 | ESP32 | Purpose |
|---|---|---|
| Pin 1 (GND) | GND | Common ground |
| Pin 2 (DQ) | GPIO 4 (D4) | 1-Wire data (open-drain) |
| Pin 3 (VDD) | 3V3 | 3.3 V supply (not parasite powered) |
| 10 kOhm from DQ | 3V3 | Pull-up: the bus is only driven low |

## Firmware

`firmware/phase3_ds18b20_test` v0.1 (OneWire + DallasTemperature
libraries). It reads the ROM address and family code, sets 12-bit
resolution, and requests a conversion every 2 s without blocking
(`millis()` timing). It reports `-127` (disconnected) and `85.00`
(power-on default) as errors rather than readings.

## Results

| Check | Result |
|---|---|
| Devices on bus | 1 |
| ROM family code | 0x28 (DS18B20) |
| Parasite power | No |
| Error values | None (`-127` / `85.00` never seen) |
| Resting reading | about 28.06 - 28.19 C, steady |
| Finger pinch | rose to 31.81 C (run 2 peak) within about 8 s |
| After release | fell to 29.44 C in about 16 s, still above rest at end of capture |

Raw captures: `docs/logs/phase3_run*.txt`.

## Analysis

- All readings are multiples of 0.0625 C, matching the 12-bit resolution
  step.
- The cooling after release is roughly exponential. Using the excess over
  the resting value (about 28.15 C), the decay over each 4 s interval is a
  factor of about 0.72 - 0.77, which gives a time constant of roughly
  12 - 15 s. This is a rough estimate from a single run in still air.

## Limitations

- No reference thermometer was used, so absolute accuracy is unverified.
  Relative changes and trends are what this phase demonstrates.
- Thermal lag is significant: a bare sensor in air takes on the order of
  seconds to tens of seconds to follow a change. Temperature will be a slow
  signal and cannot detect fast events. Mounting on the motor body will
  change this and must be re-characterised in Phase 7.
- The 10 kOhm pull-up worked on short breadboard wires. A 4.7 kOhm resistor
  is recommended before running longer wires near the motor.
- The marking and ROM family code indicate a DS18B20 but do not prove it is
  genuine.

## Next

Phase 4: drive the 6 V motor with the L298N (voltage measurement first).
