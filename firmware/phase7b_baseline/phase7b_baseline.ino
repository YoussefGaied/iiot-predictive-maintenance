// phase7b_baseline v0.5 -- motor run log: speed (completed pulses) + vibration RMS per 500 ms window
// ESP32 DevKit V1 | IMU on SDA 21 / SCL 22 (fixed to the motor body) | LM393 D0 on GPIO 18
// Motor: ENA 25, IN1 26, IN2 27 (same as Phase 4-6). No temperature sensor (the DS18B20 broke).
//
// Commands (Serial Monitor, 115200): f forward, r reverse, s stop, + / - duty +/-10,
//   p<n> = pulses per revolution used for rpm (default 2 = ASSUMED, not verified on the taped two-sector disc),
//   w<us> = shortest HIGH pulse that is counted (default 100; w0 = count everything),
//   i = switch IMU sampling off/on.
// Motor auto-stops after 3 s. One report line every 500 ms, also while the motor is off, so rest and
// motor-on data are in the same log.
//
// SPEED: a pulse is counted when D0 goes HIGH and then LOW again after at least w microseconds
// (a "completed pulse"). Counting rising edges instead overcounts: the edges carry tiny glitches
// (Phase 6 finding). pulses = completed pulses in the window, rise = rising edges (diagnostic only),
// pps = pulses per second, rpm = pps * 60 / ppr (only as good as the ppr setting).
//
// VIBRATION: vib_rms_g = RMS of the changing part of the acceleration vector in the window
// (per-axis mean removed, per-axis scale from the Phase 7A calibration), independent of IMU orientation.
// n = IMU samples in the window (target about 500 at 1 kHz), err = failed reads.
// KNOWN LIMIT: while the motor runs the I2C reads fail (Phase 7 finding). A window is only trustworthy
// if n is about 500, err is 0 and amean_g is about 1.00 (the IMU is fixed to the motor).

#include <Wire.h>

const int SDA_PIN = 21, SCL_PIN = 22;
const uint8_t MPU_ADDR = 0x68;
const int PIN_D0 = 18;
const int PIN_ENA = 25, PIN_IN1 = 26, PIN_IN2 = 27;

const int PWM_FREQ = 5000, PWM_BITS = 8, DUTY_MAX = 255, DUTY_STEP = 10;
const unsigned long RUN_MS = 3000;          // motor auto-stop
const unsigned long WINDOW_MS = 500;        // report interval
const unsigned long SAMPLE_US = 1000;       // IMU target 1 kHz

const float ACCEL_LSB_PER_G = 16384.0;      // +/-2 g power-up default
const double ACC_OFFSET[3] = {-0.0448, 0.0018, 0.2771};   // Phase 7A result
const double ACC_SCALE[3]  = {0.9876, 0.9947, 0.9905};

volatile uint32_t pulseCount = 0;           // completed pulses at least minWidthUs wide
volatile uint32_t riseCount = 0;            // rising edges seen (diagnostic)
volatile uint32_t tRise = 0;
volatile bool isHigh = false;
volatile uint32_t minWidthUs = 100;
int ppr = 2;                                // ASSUMED pulses per revolution, set with p<n>

int duty = 195, dir = 0, runId = 0;
bool imuOn = true;
unsigned long runStart = 0;

double sumA[3] = {0, 0, 0}, sumSq[3] = {0, 0, 0};
unsigned long nSamples = 0, nErr = 0;

void IRAM_ATTR onEdge() {
  uint32_t now = micros();
  if (digitalRead(PIN_D0)) {
    tRise = now;
    isHigh = true;
    riseCount++;
  } else if (isHigh) {
    isHigh = false;
    if (now - tRise >= minWidthUs) pulseCount++;
  }
}

void pwmWrite(int v) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(PIN_ENA, v);
#else
  ledcWrite(0, v);
#endif
}

void applyOutputs() {
  if (dir == 0) {
    pwmWrite(0);
    digitalWrite(PIN_IN1, LOW);
    digitalWrite(PIN_IN2, LOW);
  } else {
    digitalWrite(PIN_IN1, dir > 0 ? HIGH : LOW);
    digitalWrite(PIN_IN2, dir < 0 ? HIGH : LOW);
    pwmWrite(duty);
    runStart = millis();
  }
  Serial.printf("dir=%d duty=%d\n", dir, duty);
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

void sampleImu() {
  uint8_t b[6];
  if (!readRegs(0x3B, b, 6)) { nErr++; return; }
  int16_t raw[3];
  raw[0] = (b[0] << 8) | b[1];
  raw[1] = (b[2] << 8) | b[3];
  raw[2] = (b[4] << 8) | b[5];
  for (int k = 0; k < 3; k++) {
    double a = raw[k] / ACCEL_LSB_PER_G;
    sumA[k] += a;
    sumSq[k] += a * a;
  }
  nSamples++;
}

void report(unsigned long elapsedMs) {
  static uint32_t lastCount = 0, lastRise = 0;
  uint32_t count = pulseCount, rises = riseCount;
  uint32_t d = count - lastCount;
  uint32_t dr = rises - lastRise;
  lastCount = count;
  lastRise = rises;
  float pps = d * 1000.0f / elapsedMs;

  double vib = 0, amean = 0;
  if (nSamples > 0) {
    double varSum = 0, magSq = 0;
    for (int k = 0; k < 3; k++) {
      double mean = sumA[k] / nSamples;
      double var = sumSq[k] / nSamples - mean * mean;
      if (var < 0) var = 0;
      varSum += var / (ACC_SCALE[k] * ACC_SCALE[k]);
      double c = (mean - ACC_OFFSET[k]) / ACC_SCALE[k];
      magSq += c * c;
    }
    vib = sqrt(varSum);
    amean = sqrt(magSq);
  }
  Serial.printf("run=%d dir=%d duty=%d imu=%d w=%lu ppr=%d n=%lu err=%lu pulses=%lu rise=%lu pps=%.1f rpm=%.0f vib_rms_g=%.4f amean_g=%.3f\n",
                runId, dir, duty, imuOn ? 1 : 0, (unsigned long)minWidthUs, ppr, nSamples, nErr,
                (unsigned long)d, (unsigned long)dr, pps, ppr > 0 ? pps * 60.0f / ppr : 0.0f, vib, amean);
  for (int k = 0; k < 3; k++) { sumA[k] = 0; sumSq[k] = 0; }
  nSamples = 0;
  nErr = 0;
}

void setup() {
  pinMode(PIN_IN1, OUTPUT);
  pinMode(PIN_IN2, OUTPUT);
  digitalWrite(PIN_IN1, LOW);
  digitalWrite(PIN_IN2, LOW);
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(PIN_ENA, PWM_FREQ, PWM_BITS);
#else
  ledcSetup(0, PWM_FREQ, PWM_BITS);
  ledcAttachPin(PIN_ENA, 0);
#endif
  pwmWrite(0);

  pinMode(PIN_D0, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_D0), onEdge, CHANGE);

  Serial.begin(115200);
  delay(1000);
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);                    // v0.2: 100 kHz (v0.1 used 400 kHz and failed while the motor ran)
  Wire.setTimeOut(5);                       // v0.2: a failed read gives up after 5 ms instead of stalling the loop
  Serial.println();
  Serial.println("=== phase7b_baseline v0.5  (f/r/s, +/-, i, w<us>, p<n>)  ppr=2 ASSUMED ===");

  uint8_t who = 0;
  if (!readRegs(0x75, &who, 1)) {
    Serial.println("ERROR: no response from IMU (check wiring)");
    while (true) delay(1000);
  }
  Serial.printf("WHO_AM_I = 0x%02X\n", who);
  if (!writeReg(0x6B, 0x00)) {
    Serial.println("ERROR: could not wake IMU");
    while (true) delay(1000);
  }
  delay(100);
  applyOutputs();
}

void loop() {
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'f') { dir = 1; runId++; applyOutputs(); }
    else if (c == 'r') { dir = -1; runId++; applyOutputs(); }
    else if (c == 's') { dir = 0; applyOutputs(); }
    else if (c == '+') { duty = min(duty + DUTY_STEP, DUTY_MAX); applyOutputs(); }
    else if (c == '-') { duty = max(duty - DUTY_STEP, 0); applyOutputs(); }
    else if (c == 'w') {
      int v = Serial.parseInt();
      minWidthUs = (v >= 0 && v <= 5000) ? v : 100;
      Serial.printf("min pulse width w=%lu us\n", (unsigned long)minWidthUs);
    }
    else if (c == 'p') {
      int v = Serial.parseInt();
      ppr = (v > 0 && v <= 100) ? v : 2;
      Serial.printf("ppr=%d\n", ppr);
    }
    else if (c == 'i') { imuOn = !imuOn; Serial.printf("imu sampling %s\n", imuOn ? "ON" : "OFF"); }
  }

  if (dir != 0 && millis() - runStart > RUN_MS) {
    dir = 0;
    Serial.println("auto-stop");
    applyOutputs();
  }

  static unsigned long lastSampleUs = 0;
  unsigned long nowUs = micros();
  if (nowUs - lastSampleUs >= SAMPLE_US) {
    lastSampleUs = nowUs;
    if (imuOn) sampleImu();
  }

  static unsigned long lastReport = 0;
  unsigned long nowMs = millis();
  if (nowMs - lastReport >= WINDOW_MS) {
    report(nowMs - lastReport);
    lastReport = nowMs;
  }
}
