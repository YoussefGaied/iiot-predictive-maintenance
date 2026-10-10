# Phase 2: IMU over I2C

**Status:** Done (2026-10-10)

## Objective

Confirm the ESP32 can talk to the IMU module over I2C and read raw
acceleration and gyro values, with the motor and all other sensors
disconnected.

## Hardware

- ESP32 DevKit V1 (USB-C), USB power only
- GY-521 module sold as MPU6050. `WHO_AM_I` reads `0x70`, which is the ID
  commonly associated with MPU-6500-class parts (a genuine MPU6050 reads
  `0x68`). The chip marking has not been read, so the exact part is not
  confirmed. I2C address is `0x68`.

## Wiring

| IMU pin | ESP32 pin | Purpose |
|---|---|---|
| VCC | 3V3 | 3.3 V supply (keeps SDA/SCL pull-ups at 3.3 V) |
| GND | GND | Common ground |
| SDA | GPIO 21 (D21) | I2C data |
| SCL | GPIO 22 (D22) | I2C clock |

AD0, INT, XDA, XCL are not connected.

## Firmware

- `firmware/phase2_i2c_scan` v0.1: scans addresses 1-126, prints responders.
- `firmware/phase2_mpu6050_read` v0.2: reads `WHO_AM_I`, wakes the chip
  (writes 0 to `PWR_MGMT_1`), then prints accel (g) and gyro (deg/s) at
  10 Hz using the default +/-2 g and +/-250 deg/s ranges. The on-chip
  temperature is ignored because its conversion differs between chips.

## Results

| Test | Observation |
|---|---|
| I2C scan | Device found at 0x68 |
| WHO_AM_I | 0x70 |
| Flat, component side up | az ~ +1.258 g, ax ~ -0.128 g, ay ~ +0.023 g |
| Flat, component side down | az ~ -0.763 g, ax ~ -0.140 g, ay ~ +0.003 g |
| Tilt ~45 deg about Y | ax ~ -0.787 g, az ~ +0.920 g, ay ~ +0.021 g |
| Rotation by hand | gyro spikes up to ~32 deg/s, returns to baseline |
| Gyro bias at rest | about +1.1 / +1.7 / -0.4 deg/s (X/Y/Z) |
| Accel noise at rest | about +/-0.005 g sample to sample |

Raw captures: `docs/logs/phase2_*.txt`.

## Analysis (estimates)

From the flat and flipped captures, assuming a level table and exactly 1 g:

- Z offset ~ (1.258 + (-0.763)) / 2 ~ +0.25 g
- Z scale ~ (1.258 - (-0.763)) / 2 ~ 1.01
- X offset ~ -0.13 g (ax keeps its sign when the board is flipped, which
  points to an offset rather than a tilted table)

Applying these to two earlier resting captures brings their magnitude from
1.055 g and 1.133 g to about 1.00 g. The tilt capture corrects to about
0.94 g, which suggests the X axis has a different sensitivity; a single
tilt cannot separate scale from offset.

## Limitations

- The accelerometer is uncalibrated: raw magnitude ranges from about
  0.78 g to 1.27 g depending on orientation. Raw values must not be
  reported as true acceleration.
- A proper per-axis calibration (all six faces) is planned before baselines
  in Phase 7. Vibration features based on the changing part of the signal
  (for example RMS after removing the mean) are much less sensitive to
  constant offsets.
- The part may not be a genuine MPU6050; register details such as filter
  and sample-rate settings must be checked against the actual chip's
  register map in Phase 7.

## Next

Phase 3: DS18B20 temperature sensor (marking and pinout check first).
