// Phase 7A: six-position accelerometer calibration + gyro bias (no motor, no battery)
// Firmware v0.1 | ESP32 DevKit V1 | SDA = GPIO 21, SCL = GPIO 22 | same I2C pattern as Phase 2
//
// Commands (Serial Monitor, 115200, line ending "Newline" or "No line ending"):
//   c  capture the current pose (hold the board STILL for about 1 s)
//   p  print the calibration result (needs all six poses)
//   v  verify: capture the current pose and print the corrected |a|
//   r  reset all captured poses
// The pose is detected automatically: the axis with the largest reading decides
// which of +X -X +Y -Y +Z -Z it is, so you do not need to know the arrow directions.

#include <Wire.h>

const int SDA_PIN = 21;
const int SCL_PIN = 22;
const uint8_t MPU_ADDR = 0x68;

const uint8_t REG_PWR_MGMT_1   = 0x6B;
const uint8_t REG_WHO_AM_I     = 0x75;
const uint8_t REG_ACCEL_XOUT_H = 0x3B;

const float ACCEL_LSB_PER_G  = 16384.0;   // +/-2 g power-up default
const float GYRO_LSB_PER_DPS = 131.0;     // +/-250 deg/s power-up default

const int N_SAMPLES = 200;                // 200 samples x 5 ms = about 1 s
const float MIN_DOMINANT_G = 0.7;         // a clean pose has one axis near 1 g
const float MAX_SD_G = 0.02;              // reject captures where the board moved

const char* SLOT_NAME[6] = {"+X", "-X", "+Y", "-Y", "+Z", "-Z"};
const char* AXIS_NAME[3] = {"X", "Y", "Z"};

double slotAcc[6][3];
double slotGyro[6][3];
bool slotHave[6] = {false, false, false, false, false, false};
double accOffset[3] = {0, 0, 0};
double accScale[3] = {1, 1, 1};
bool calValid = false;

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

// Mean of N_SAMPLES readings: acc in g, gyro in deg/s, sd = accel standard deviation in g.
bool grab(double acc[3], double gyro[3], double sd[3]) {
  double sumA[3] = {0, 0, 0}, sumSq[3] = {0, 0, 0}, sumG[3] = {0, 0, 0};
  for (int i = 0; i < N_SAMPLES; i++) {
    uint8_t b[14];
    if (!readRegs(REG_ACCEL_XOUT_H, b, 14)) return false;
    int16_t raw[6];
    raw[0] = (b[0] << 8) | b[1];
    raw[1] = (b[2] << 8) | b[3];
    raw[2] = (b[4] << 8) | b[5];
    raw[3] = (b[8] << 8) | b[9];     // b[6], b[7] = die temperature: ignored
    raw[4] = (b[10] << 8) | b[11];
    raw[5] = (b[12] << 8) | b[13];
    for (int k = 0; k < 3; k++) {
      double a = raw[k] / ACCEL_LSB_PER_G;
      sumA[k] += a;
      sumSq[k] += a * a;
      sumG[k] += raw[3 + k] / GYRO_LSB_PER_DPS;
    }
    delay(5);
  }
  for (int k = 0; k < 3; k++) {
    acc[k] = sumA[k] / N_SAMPLES;
    gyro[k] = sumG[k] / N_SAMPLES;
    double var = sumSq[k] / N_SAMPLES - acc[k] * acc[k];
    sd[k] = sqrt(var > 0 ? var : 0);
  }
  return true;
}

int dominantAxis(const double acc[3]) {
  int best = 0;
  for (int k = 1; k < 3; k++) if (fabs(acc[k]) > fabs(acc[best])) best = k;
  return best;
}

void capturePose() {
  double acc[3], gyro[3], sd[3];
  Serial.println("capturing, hold still...");
  if (!grab(acc, gyro, sd)) { Serial.println("ERROR: IMU read failed"); return; }
  int ax = dominantAxis(acc);
  if (fabs(acc[ax]) < MIN_DOMINANT_G) {
    Serial.printf("REJECTED: largest axis is only %.3f g. Rest the board on a flat face or edge.\n", fabs(acc[ax]));
    return;
  }
  if (sd[0] > MAX_SD_G || sd[1] > MAX_SD_G || sd[2] > MAX_SD_G) {
    Serial.printf("REJECTED: board moved during capture (sd %.3f %.3f %.3f g). Try again.\n", sd[0], sd[1], sd[2]);
    return;
  }
  int slot = 2 * ax + (acc[ax] < 0 ? 1 : 0);
  for (int k = 0; k < 3; k++) { slotAcc[slot][k] = acc[k]; slotGyro[slot][k] = gyro[k]; }
  slotHave[slot] = true;
  int have = 0;
  for (int s = 0; s < 6; s++) if (slotHave[s]) have++;
  Serial.printf("pose=%s  a_g=(%.4f %.4f %.4f)  sd_g=(%.4f %.4f %.4f)  gyro_dps=(%.2f %.2f %.2f)\n",
                SLOT_NAME[slot], acc[0], acc[1], acc[2], sd[0], sd[1], sd[2], gyro[0], gyro[1], gyro[2]);
  Serial.printf("captured %d of 6 poses:", have);
  for (int s = 0; s < 6; s++) Serial.printf(" %s%s", SLOT_NAME[s], slotHave[s] ? "(ok)" : "(-)");
  Serial.println();
}

void printResult() {
  for (int s = 0; s < 6; s++) {
    if (!slotHave[s]) {
      Serial.printf("Not ready: missing pose %s. Capture all six first.\n", SLOT_NAME[s]);
      return;
    }
  }
  Serial.println("--- accelerometer calibration: corrected = (raw - offset) / scale ---");
  for (int k = 0; k < 3; k++) {
    double plus = slotAcc[2 * k][k];
    double minus = slotAcc[2 * k + 1][k];
    accOffset[k] = (plus + minus) / 2.0;
    accScale[k] = (plus - minus) / 2.0;
    Serial.printf("axis=%s offset_g=%.4f scale=%.4f  (reading at +1 g: %.4f, at -1 g: %.4f)\n",
                  AXIS_NAME[k], accOffset[k], accScale[k], plus, minus);
  }
  calValid = true;
  Serial.println("--- gyro bias (mean over the six still captures, deg/s) and spread max-min ---");
  for (int k = 0; k < 3; k++) {
    double sum = 0, lo = 1e9, hi = -1e9;
    for (int s = 0; s < 6; s++) {
      double g = slotGyro[s][k];
      sum += g;
      if (g < lo) lo = g;
      if (g > hi) hi = g;
    }
    Serial.printf("gyro %s bias_dps=%.3f spread=%.3f\n", AXIS_NAME[k], sum / 6.0, hi - lo);
  }
  Serial.printf("const float ACC_OFFSET[3] = {%.4f, %.4f, %.4f};\n", accOffset[0], accOffset[1], accOffset[2]);
  Serial.printf("const float ACC_SCALE[3]  = {%.4f, %.4f, %.4f};\n", accScale[0], accScale[1], accScale[2]);
}

void verifyPose() {
  if (!calValid) { Serial.println("Run p first (needs all six poses)."); return; }
  double acc[3], gyro[3], sd[3];
  Serial.println("capturing, hold still...");
  if (!grab(acc, gyro, sd)) { Serial.println("ERROR: IMU read failed"); return; }
  double c[3];
  for (int k = 0; k < 3; k++) c[k] = (acc[k] - accOffset[k]) / accScale[k];
  double rawMag = sqrt(acc[0] * acc[0] + acc[1] * acc[1] + acc[2] * acc[2]);
  double corMag = sqrt(c[0] * c[0] + c[1] * c[1] + c[2] * c[2]);
  Serial.printf("raw_g=(%.4f %.4f %.4f) |a|raw=%.4f   corrected_g=(%.4f %.4f %.4f) |a|corrected=%.4f\n",
                acc[0], acc[1], acc[2], rawMag, c[0], c[1], c[2], corMag);
}

void setup() {
  // Motor outputs LOW (no motor power is connected in this phase, but keep the pins defined).
  pinMode(25, OUTPUT); pinMode(26, OUTPUT); pinMode(27, OUTPUT);
  digitalWrite(25, LOW); digitalWrite(26, LOW); digitalWrite(27, LOW);

  Serial.begin(115200);
  delay(1000);
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);

  Serial.println();
  Serial.println("=== phase7a_imu_calibration v0.1 ===");
  uint8_t who = 0;
  if (!readRegs(REG_WHO_AM_I, &who, 1)) {
    Serial.println("ERROR: no response from IMU (check wiring/address)");
    while (true) delay(1000);
  }
  Serial.printf("WHO_AM_I = 0x%02X\n", who);
  if (!writeReg(REG_PWR_MGMT_1, 0x00)) {
    Serial.println("ERROR: could not wake IMU");
    while (true) delay(1000);
  }
  delay(100);
  Serial.println("Commands: c capture, p result, v verify, r reset");
}

void loop() {
  if (!Serial.available()) return;
  char ch = Serial.read();
  if (ch == 'c') capturePose();
  else if (ch == 'p') printResult();
  else if (ch == 'v') verifyPose();
  else if (ch == 'r') {
    for (int s = 0; s < 6; s++) slotHave[s] = false;
    calValid = false;
    Serial.println("reset");
  }
}
