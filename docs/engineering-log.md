# Engineering Log

Entry format: date, phase, wiring, firmware version, readings, result,
issues, next step. Only record what was actually observed.

---

## 2026-10-10 - Phase 0: hardware identification (in progress)
![Hardware overview](images/phase0_hardware_overview.png)

- Photographed the ESP32 (front/back) and the parts layout.
- ESP32 back silkscreen: "ESP32 DEVKIT V1 TYPEC"; 30-pin header layout
  appears from the photo. Front labels read: left EN, VP, VN, D34, D35,
  D32, D33, D25, D26, D27, D14, D12, D13, GND, VIN; right D23, D22, TX0,
  RX0, D21, D19, D18, D5, TX2, RX2, D4, D2, D15, GND, 3V3.
- Not yet verified: MPU6050 pin labels and solder joints, ACS712 variant,
  LM393 sensor type, DS18B20 marking/pinout, L298N jumper/regulator layout,
  battery pack voltage, barrel adapter polarity.
- A 2-channel relay module is also in the parts pile; not part of this
  project and stays unconnected.
- Next: close-up photos of the MPU6050 (front, back/solder joints).

## 2026-10-10 - Phase 1: ESP32 setup (DONE)
![Phase 1 serial output](images/phase1_esp32_serial_test.png)
- Board: ESP32 DevKit V1 (USB-C)
- Chip: ESP32-D0WD-V3, revision 301, 2 cores, 240 MHz, 4 MB flash
- Toolchain: Arduino IDE on Ubuntu, board "ESP32 Dev Module", 115200 baud
- Firmware: `firmware/phase1_esp32_test` v0.1
- Wiring: USB only
- Result: chip info printed; counter and LED state toggle every 1000 ms
  (observed up to t = 165056 ms, count = 164)
- Open: `lsusb` output (USB-serial chip), visual confirmation of LED blink
- Next: Phase 2, MPU6050 over I2C
