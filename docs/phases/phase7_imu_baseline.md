# Phase 7: IMU calibration and operating baselines (closed 2026-10-18, partly complete)

## Objective
Calibrate the MPU6050-class IMU, then record vibration baselines with the motor off and running, together with speed.

## Parts and wiring
- GY-521 sold as MPU6050 (I2C 0x68, WHO_AM_I 0x70, MPU-6500-class): VCC 3V3, GND, SDA GPIO 21, SCL GPIO 22.
- For 7B the IMU was taped on the motor body. LM393 D0 on GPIO 18, motor on 25 / 26 / 27 as before.
- The DS18B20 temperature sensor broke during this phase (cause not given); temperature is omitted until it is
  replaced (a new sensor needs its pinout checked before it is powered).

## 7A: six-position calibration (`phase7a_imu_calibration` v0.1, log `phase7a_calibration.txt`)
Six still captures (1 s each, scatter about 0.003 g), pose detected automatically. corrected = (raw - offset) / scale.
| Axis | Offset (g) | Scale |
|---|---|---|
| X | -0.0448 | 0.9876 |
| Y | 0.0018 | 0.9947 |
| Z | 0.2771 | 0.9905 |
Verify in two tilted poses: |a| corrected 1.0277 g (raw 0.917) and 1.0098 g (raw 1.155). The edge poses leaned a few
degrees, which probably explains the 0.03 g miss (not tested). Gyro bias about +1.10 / +1.80 / -0.85 deg/s with a
spread between poses of 0.35 / 0.77 / 2.05, so the gyro bias is not constant; subtract a rest value at the start of
each use. The Phase 2 estimates (X offset -0.13 g, X scale about 10 % low) were wrong and are replaced.

## 7B: baselines (`phase7b_baseline` v0.1 to v0.5)
- Motor off, IMU on the motor body: n = 500 samples per 500 ms window, err = 0, vib_rms_g about 0.0052 g (typical
  spread 0.005-0.006 with occasional touches), amean_g 1.002-1.003 g.
- The IMU mount shifted between runs at times: rest amean_g moved to 1.05-1.11 and rest vib_rms_g to about 0.011 g
  until the mount was fixed again. A baseline needs a rigid, unmoving mount.
- **Motor running: no valid vibration data.** While the motor ran, I2C reads failed (n dropped to 9-260 of 500, err up
  to 205 per window) and amean_g fell to 0.15-0.9 g, which is impossible for a fixed IMU, so the vibration values of
  0.4-1.7 g in those windows are not measurements (`phase7b_rest_and_motor_on_errors.txt`).
- Tests that did not cure it: I2C 400 kHz to 100 kHz with a 5 ms timeout (`err` unchanged); IMU sampling off versus
  on (the pulse count did not change, so I2C traffic is not what disturbs the speed sensor).
- Not yet tried: 100 nF ceramic capacitor across the motor terminals (ownership never confirmed); moving the IMU away
  from the motor; shorter or twisted SDA/SCL wires; a separate supply for the sensor.
- Cause not proven. The motor also puts microsecond-scale glitches on the speed-sensor output, so electrical noise
  from the motor is the working hypothesis.

## Speed in this phase
The RPM columns of firmware v0.1-v0.4 count rising edges and are invalid. v0.5 counts completed pulses
(see Phase 6) and takes `p<n>` for pulses per revolution (default 2, assumed).

## Done criteria
- Calibration done and checked: met (with the 0.03 g note above).
- Motor-off vibration baseline: met (about 0.005 g RMS with a rigid mount).
- Motor-running vibration baseline: **NOT met**. Needs the noise problem solved first.
- Temperature in the baseline: not done (sensor broke).

## Consequence for later phases
The pipeline can publish pulses per second (and RPM with its stated assumption), uncalibrated current (Phase 5) and
the motor-off vibration baseline. Vibration under load must not be published or used for anomaly detection until a
re-test with the noise fixes shows n about 500, err 0 and amean_g about 1.00 while the motor runs. Do not claim
fault detection or prediction from this data.

## Check with firmware v0.5 (2026-10-18, `phase7b_v05_check.txt`)
Same result as before: motor off gives n about 499, err 0, vib_rms_g about 0.005 g, amean_g 1.000 (later 1.025 and 1.087,
so the mount shifted again between runs). While the motor runs, n is 88-248 of 500, err is 77-149 per window and amean_g is
0.27-0.96, so the vibration values of 0.4-0.9 g in those windows are not measurements. v0.5 changes the speed counting
only; it does not change the I2C problem, which stays open.
