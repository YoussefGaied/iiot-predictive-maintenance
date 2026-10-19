// phase8d_telemetry v0.2 -- phase7b_baseline v0.5 + Wi-Fi + MQTT JSON telemetry every 500 ms
// ESP32 DevKit V1 | IMU SDA 21 / SCL 22 | LM393 D0 GPIO 18 | L298N ENA 25 / IN1 26 / IN2 27
// ACS712 OUT -> 10 kOhm -> GPIO 34 (ADC1). No temperature sensor (DS18B20 broken -> not published).
//
// Serial commands (115200): f forward, r reverse, s stop, + / - duty +/-10,
//   z = zero the current reading using the LAST FULL 500 ms window (motor STOPPED for at least 2 s, battery connected),
//   p<n> = pulses per revolution used for rpm (default 2 = ASSUMED, never verified),
//   w<us> = shortest counted HIGH pulse (default 100), i = IMU sampling on/off.
// Motor auto-stops after 3 s. Needs secrets.h next to this file (see secrets.h.example).
//
// Topics: iiot/rig1/telemetry (every 500 ms, not retained), iiot/rig1/status (retained online/offline, Last Will).
//
// VALIDITY (what the dashboard must respect):
//   vib_valid  = true only if vib_n >= 400 AND vib_err == 0 AND 0.97 <= amean_g <= 1.03 AND IMU on AND not settling.
//                (v0.2: was 450; with the ADC read added to the 1 kHz tick the motor-off baseline is n = 423-424 per window.)
//                While the motor runs this is expected to be false most of the time (Phase 7 I2C errors).
//   settling   = true in the window where a motor run started (acceleration); do not use that window.
//   rpm        = pulses_per_s * 60 / ppr_assumed. ppr_assumed is NOT verified: rpm is only an assumption-based estimate.
//   current_dmv = mean ADC mV minus the rest level stored by 'z' (mV, UNCALIBRATED, negative while running);
//                null until 'z' was used. Not amps.
// Mosquitto must be on the LAN only. Reconnects are never attempted while the motor runs, so a slow
// network can never delay the 3 s auto-stop.

#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "secrets.h"

const char* FW_VERSION = "0.2";
const char* DEVICE_ID  = "rig1";
const char* TOPIC_TELEMETRY = "iiot/rig1/telemetry";
const char* TOPIC_STATUS    = "iiot/rig1/status";

const int SDA_PIN = 21, SCL_PIN = 22;
const uint8_t MPU_ADDR = 0x68;
const int PIN_D0 = 18, PIN_ADC = 34;
const int PIN_ENA = 25, PIN_IN1 = 26, PIN_IN2 = 27;

const int PWM_FREQ = 5000, PWM_BITS = 8, DUTY_MAX = 255, DUTY_STEP = 10;
const unsigned long RUN_MS = 3000;
const unsigned long WINDOW_MS = 500;
const unsigned long SAMPLE_US = 1000;
const unsigned long RECONNECT_MS = 5000;

const float ACCEL_LSB_PER_G = 16384.0;
const double ACC_OFFSET[3] = {-0.0448, 0.0018, 0.2771};   // Phase 7A
const double ACC_SCALE[3]  = {0.9876, 0.9947, 0.9905};

volatile uint32_t pulseCount = 0, riseCount = 0, tRise = 0;
volatile bool isHigh = false;
volatile uint32_t minWidthUs = 100;
int ppr = 2;                                  // ASSUMED

int duty = 195, dir = 0, runId = 0;
bool imuOn = true, settling = false;
unsigned long runStart = 0;

double sumA[3] = {0, 0, 0}, sumSq[3] = {0, 0, 0};
unsigned long nSamples = 0, nErr = 0;
double sumMv = 0; unsigned long nAdc = 0;
double restMv = 0, lastMeanMv = 0; bool zeroed = false, haveLast = false;

uint32_t seq = 0;
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

void IRAM_ATTR onEdge() {
  uint32_t now = micros();
  if (digitalRead(PIN_D0)) { tRise = now; isHigh = true; riseCount++; }
  else if (isHigh) { isHigh = false; if (now - tRise >= minWidthUs) pulseCount++; }
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
    settling = true;
  }
  Serial.printf("dir=%d duty=%d\n", dir, duty);
}

bool writeReg(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg); Wire.write(val);
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
  int16_t raw[3] = { (int16_t)((b[0] << 8) | b[1]), (int16_t)((b[2] << 8) | b[3]), (int16_t)((b[4] << 8) | b[5]) };
  for (int k = 0; k < 3; k++) {
    double a = raw[k] / ACCEL_LSB_PER_G;
    sumA[k] += a; sumSq[k] += a * a;
  }
  nSamples++;
}

void sampleAdc() { sumMv += analogReadMilliVolts(PIN_ADC); nAdc++; }

void connectWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("WiFi connecting");
  unsigned long t = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t < 20000) { delay(500); Serial.print('.'); }
  if (WiFi.status() == WL_CONNECTED) { Serial.print("\nWiFi OK, IP "); Serial.println(WiFi.localIP()); }
  else Serial.println("\nWiFi FAILED (telemetry off until it connects; motor control still works)");
}

bool connectMqtt() {
  bool ok = mqtt.connect(DEVICE_ID, MQTT_USER, MQTT_PASS, TOPIC_STATUS, 0, true, "offline");
  if (ok) mqtt.publish(TOPIC_STATUS, "online", true);
  Serial.printf("MQTT %s (rc=%d)\n", ok ? "connected" : "connect FAILED", mqtt.state());
  return ok;
}

void report(unsigned long elapsedMs) {
  static uint32_t lastCount = 0;
  uint32_t count = pulseCount;
  uint32_t d = count - lastCount;
  lastCount = count;
  float pps = d * 1000.0f / elapsedMs;
  float rpm = ppr > 0 ? pps * 60.0f / ppr : 0.0f;

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
  bool vibValid = imuOn && !settling && nSamples >= 400 && nErr == 0 && amean >= 0.97 && amean <= 1.03;

  char cur[16];
  if (zeroed && nAdc > 0) snprintf(cur, sizeof(cur), "%.1f", sumMv / nAdc - restMv);
  else snprintf(cur, sizeof(cur), "null");

  const char* mdir = dir > 0 ? "fwd" : (dir < 0 ? "rev" : "stop");
  char payload[420];
  snprintf(payload, sizeof(payload),
    "{\"device_id\":\"%s\",\"fw\":\"%s\",\"seq\":%lu,\"uptime_ms\":%lu,\"run\":%d,"
    "\"motor_dir\":\"%s\",\"duty\":%d,\"settling\":%s,"
    "\"pulses_per_s\":%.1f,\"rpm\":%.0f,\"ppr_assumed\":%d,"
    "\"current_dmv\":%s,"
    "\"vib_rms_g\":%.4f,\"vib_n\":%lu,\"vib_err\":%lu,\"amean_g\":%.3f,\"vib_valid\":%s,"
    "\"rssi\":%d}",
    DEVICE_ID, FW_VERSION, (unsigned long)seq, (unsigned long)millis(), runId,
    mdir, duty, settling ? "true" : "false",
    pps, rpm, ppr,
    cur,
    vib, nSamples, nErr, amean, vibValid ? "true" : "false",
    WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0);

  bool sent = false;
  if (mqtt.connected()) sent = mqtt.publish(TOPIC_TELEMETRY, payload);
  Serial.printf("seq=%lu %s dir=%s duty=%d pps=%.1f n=%lu err=%lu vib=%.4f amean=%.3f valid=%d cur=%s\n",
                (unsigned long)seq, sent ? "pub-ok " : "pub-NO ", mdir, duty, pps, nSamples, nErr, vib, amean,
                vibValid ? 1 : 0, cur);
  seq++;

  if (nAdc > 0) { lastMeanMv = sumMv / nAdc; haveLast = true; }
  for (int k = 0; k < 3; k++) { sumA[k] = 0; sumSq[k] = 0; }
  nSamples = 0; nErr = 0; sumMv = 0; nAdc = 0;
  settling = false;
}

void setup() {
  pinMode(PIN_IN1, OUTPUT); pinMode(PIN_IN2, OUTPUT);
  digitalWrite(PIN_IN1, LOW); digitalWrite(PIN_IN2, LOW);
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(PIN_ENA, PWM_FREQ, PWM_BITS);
#else
  ledcSetup(0, PWM_FREQ, PWM_BITS);
  ledcAttachPin(PIN_ENA, 0);
#endif
  pwmWrite(0);

  pinMode(PIN_D0, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_D0), onEdge, CHANGE);
  analogSetPinAttenuation(PIN_ADC, ADC_11db);

  Serial.begin(115200);
  delay(1000);
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);
  Wire.setTimeOut(5);
  Serial.println();
  Serial.println("=== phase8d_telemetry v0.2  (f/r/s, +/-, z, i, w<us>, p<n>)  ppr=2 ASSUMED ===");

  uint8_t who = 0;
  if (!readRegs(0x75, &who, 1)) { Serial.println("ERROR: no response from IMU (check wiring)"); while (true) delay(1000); }
  Serial.printf("WHO_AM_I = 0x%02X\n", who);
  if (!writeReg(0x6B, 0x00)) { Serial.println("ERROR: could not wake IMU"); while (true) delay(1000); }
  delay(100);

  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setBufferSize(512);          // default 256 bytes is too small for this payload
  mqtt.setSocketTimeout(2);         // seconds; keeps a failed connect short
  connectWifi();
  if (WiFi.status() == WL_CONNECTED) connectMqtt();
  applyOutputs();
  settling = false;
}

void loop() {
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'f') { dir = 1; runId++; applyOutputs(); }
    else if (c == 'r') { dir = -1; runId++; applyOutputs(); }
    else if (c == 's') { dir = 0; applyOutputs(); }
    else if (c == '+') { duty = min(duty + DUTY_STEP, DUTY_MAX); applyOutputs(); }
    else if (c == '-') { duty = max(duty - DUTY_STEP, 0); applyOutputs(); }
    else if (c == 'z') {
      if (dir != 0) Serial.println("z ignored: stop the motor first");
      else if (!haveLast) Serial.println("z: wait for the first full window");
      else { restMv = lastMeanMv; zeroed = true; Serial.printf("current zero set: rest = %.1f mV (last full window)\n", restMv); }
    }
    else if (c == 'w') { int v = Serial.parseInt(); minWidthUs = (v >= 0 && v <= 5000) ? v : 100; Serial.printf("w=%lu us\n", (unsigned long)minWidthUs); }
    else if (c == 'p') { int v = Serial.parseInt(); ppr = (v > 0 && v <= 100) ? v : 2; Serial.printf("ppr=%d\n", ppr); }
    else if (c == 'i') { imuOn = !imuOn; Serial.printf("imu sampling %s\n", imuOn ? "ON" : "OFF"); }
  }

  if (dir != 0 && millis() - runStart > RUN_MS) { dir = 0; Serial.println("auto-stop"); applyOutputs(); }

  static unsigned long lastSampleUs = 0;
  unsigned long nowUs = micros();
  if (nowUs - lastSampleUs >= SAMPLE_US) {
    lastSampleUs = nowUs;
    if (imuOn) sampleImu();
    sampleAdc();
  }

  mqtt.loop();

  // Reconnect only while the motor is stopped: a blocking connect must never delay the auto-stop.
  static unsigned long lastTry = 0;
  if (dir == 0 && WiFi.status() == WL_CONNECTED && !mqtt.connected() && millis() - lastTry >= RECONNECT_MS) {
    lastTry = millis();
    connectMqtt();
  }

  static unsigned long lastReport = 0;
  unsigned long nowMs = millis();
  if (nowMs - lastReport >= WINDOW_MS) {
    report(nowMs - lastReport);
    lastReport = nowMs;
  }
}
