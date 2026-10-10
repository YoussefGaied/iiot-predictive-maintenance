// Phase 2b: MPU-6050/6500-class IMU raw read over I2C, no external library
// Firmware v0.2 | ESP32 DevKit V1 | SDA = GPIO 21, SCL = GPIO 22

#include <Wire.h>

const int SDA_PIN = 21;
const int SCL_PIN = 22;
uint8_t MPU_ADDR = 0x68;

const uint8_t REG_PWR_MGMT_1   = 0x6B;
const uint8_t REG_WHO_AM_I     = 0x75;
const uint8_t REG_ACCEL_XOUT_H = 0x3B;

// Power-up default full-scale ranges (+/-2 g, +/-250 deg/s)
const float ACCEL_LSB_PER_G  = 16384.0;
const float GYRO_LSB_PER_DPS = 131.0;

const unsigned long SAMPLE_INTERVAL_MS = 100;  // 10 Hz for this test
unsigned long lastSample = 0;

const char* chipName(uint8_t id) {
  switch (id) {
    case 0x68: return "MPU-6050";
    case 0x70: return "MPU-6500 class";
    case 0x71: return "MPU-9250 class";
    default:   return "unknown";
  }
}

bool writeReg(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.write(val);
  return Wire.endTransmission() == 0;
}

bool readRegs(uint8_t reg, uint8_t *buf, size_t n) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  size_t got = Wire.requestFrom((int)MPU_ADDR, (int)n, (int)true);
  if (got != n) return false;
  for (size_t i = 0; i < n; i++) buf[i] = Wire.read();
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);

  Serial.println();
  Serial.println("=== IMU raw read v0.2 ===");

  uint8_t who = 0;
  if (!readRegs(REG_WHO_AM_I, &who, 1)) {
    Serial.println("ERROR: no response from IMU (check wiring/address)");
    while (true) delay(1000);
  }
  Serial.printf("WHO_AM_I = 0x%02X -> %s\n", who, chipName(who));
  if (who != 0x68) {
    Serial.println("NOTE: not a genuine-MPU6050 ID; reading main registers anyway");
  }

  // The chip starts in sleep mode; writing 0 to PWR_MGMT_1 wakes it.
  if (!writeReg(REG_PWR_MGMT_1, 0x00)) {
    Serial.println("ERROR: could not wake IMU");
    while (true) delay(1000);
  }
  delay(100);
  Serial.println("Awake. Columns: ax ay az |a| (g)   gx gy gz (deg/s)");
}

void loop() {
  unsigned long now = millis();
  if (now - lastSample < SAMPLE_INTERVAL_MS) return;
  lastSample = now;

  uint8_t b[14];
  if (!readRegs(REG_ACCEL_XOUT_H, b, 14)) {
    Serial.println("read error");
    return;
  }

  int16_t axr = (b[0]  << 8) | b[1];
  int16_t ayr = (b[2]  << 8) | b[3];
  int16_t azr = (b[4]  << 8) | b[5];
  // b[6], b[7] = on-chip temperature: ignored (conversion differs by chip)
  int16_t gxr = (b[8]  << 8) | b[9];
  int16_t gyr = (b[10] << 8) | b[11];
  int16_t gzr = (b[12] << 8) | b[13];

  float ax = axr / ACCEL_LSB_PER_G;
  float ay = ayr / ACCEL_LSB_PER_G;
  float az = azr / ACCEL_LSB_PER_G;
  float mag = sqrtf(ax * ax + ay * ay + az * az);

  Serial.printf("%8.3f %8.3f %8.3f |%6.3f|  %8.2f %8.2f %8.2f\n",
                ax, ay, az, mag,
                gxr / GYRO_LSB_PER_DPS, gyr / GYRO_LSB_PER_DPS, gzr / GYRO_LSB_PER_DPS);
}