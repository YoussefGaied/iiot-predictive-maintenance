# Industrial IoT Predictive Maintenance System

A physical demonstration rig that monitors a small DC motor with an ESP32 and
sensors, will send measurements to a backend, flag unusual operating
conditions, and show them in a dashboard.

## Scope and honesty

This project is **motor condition monitoring and anomaly detection on a small
6 V DC-motor demo rig**. It is **not** validated for predicting real
industrial failures. Claiming failure prediction would require labeled failure
data and proper evaluation, which this rig does not have. Anomaly detection
starts with simple threshold rules based on measured baselines; machine
learning is added only if enough representative data exists.

## Status

| Phase | Goal | Status |
|---|---|---|
| 0 | Identify hardware | In progress (see `docs/hardware-inventory.md`) |
| 1 | ESP32 setup and upload test | Done (2026-10-10) |
| 2 | MPU6050 accelerometer over I²C | Done (2026-10-10) |
| 3 | DS18B20 temperature sensor | Not started |
| 4 | Drive the 6 V motor with the L298N | Not started |
| 5 | ACS712 current sensing + calibration | Not started |
| 6 | LM393 speed sensor + RPM | Not started |
| 7 | Vibration and operating-condition baselines | Not started |
| 8 | MQTT telemetry + database | Not started |
| 9 | React/TypeScript dashboard | Not started |
| 10 | Validated anomaly detection + portfolio docs | Not started |

## Hardware

ESP32 DevKit V1 (USB-C), L298N motor driver, 6 V DC motor ("310DC"),
ACS712 current sensor, MPU6050 (GY-521), DS18B20 (TO-92), LM393 speed
sensor module, 6×AA battery holder. Details and verification status:
[`docs/hardware-inventory.md`](docs/hardware-inventory.md).

## Planned architecture (after hardware is reliable)

ESP32 sensors → MQTT (Mosquitto) → Python consumer/API (FastAPI) →
PostgreSQL → React + TypeScript dashboard.

## Repository layout

```
docs/                 engineering log, hardware inventory, safety notes, phase write-ups, raw logs, images
firmware/             one folder per phase, each with its own sketch
```

## Safety

Read [`docs/safety-notes.md`](docs/safety-notes.md) before wiring anything.
Test one subsystem at a time and disconnect power before changing wiring.

## Engineering log

Every phase is documented with wiring, firmware version, measurements,
results, and issues in [`docs/engineering-log.md`](docs/engineering-log.md).

## Limitations

- The 6 V motor is a demonstration device, not industrial-grade equipment.
- The ACS712 20 A module may have poor resolution for a small motor.
- Absolute RPM is reported only after pulses per revolution is determined.

## License

To be decided.
