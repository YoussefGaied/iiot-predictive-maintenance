// phase5_acs712_motor  v0.1 -- current vs motor state (UNCALIBRATED)
// Commands: f r s  + -  z (re-zero offset, motor must be stopped)
const int PIN_ACS = 34, PIN_ENA = 25, PIN_IN1 = 26, PIN_IN2 = 27;
const int PWM_FREQ = 5000, PWM_BITS = 8, DUTY_MAX = 255, DUTY_STEP = 10;
const unsigned long RUN_MS = 3000;
const float MV_PER_A = 100.0;      // datasheet, 20 A variant

int duty = 145, dir = 0;
unsigned long runStart = 0;
float offset_mV = 0;

void pwmWrite(int v) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(PIN_ENA, v);
#else
  ledcWrite(0, v);
#endif
}

float measure(int n) {
  long s = 0;
  for (int i = 0; i < n; i++) { s += analogReadMilliVolts(PIN_ACS); delayMicroseconds(500); }
  return s / (float)n;
}

void applyOutputs() {
  if (dir == 0) { pwmWrite(0); digitalWrite(PIN_IN1, LOW); digitalWrite(PIN_IN2, LOW); }
  else {
    digitalWrite(PIN_IN1, dir > 0); digitalWrite(PIN_IN2, dir < 0);
    pwmWrite(duty); runStart = millis();
  }
  Serial.printf("dir=%d duty=%d\n", dir, duty);
}

void zero() { offset_mV = measure(2000); Serial.printf("offset_mV=%.1f\n", offset_mV); }

void setup() {
  pinMode(PIN_IN1, OUTPUT); pinMode(PIN_IN2, OUTPUT);
  digitalWrite(PIN_IN1, LOW); digitalWrite(PIN_IN2, LOW);
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(PIN_ENA, PWM_FREQ, PWM_BITS);
#else
  ledcSetup(0, PWM_FREQ, PWM_BITS); ledcAttachPin(PIN_ENA, 0);
#endif
  pwmWrite(0);
  analogReadResolution(12);
  analogSetPinAttenuation(PIN_ACS, ADC_11db);
  Serial.begin(115200);
  delay(500);
  Serial.println("phase5_acs712_motor v0.1  (f/r/s, +/-, z)");
  zero();
}

void loop() {
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'f') { dir = 1; applyOutputs(); }
    else if (c == 'r') { dir = -1; applyOutputs(); }
    else if (c == 's') { dir = 0; applyOutputs(); }
    else if (c == '+') { duty = min(duty + DUTY_STEP, DUTY_MAX); applyOutputs(); }
    else if (c == '-') { duty = max(duty - DUTY_STEP, 0); applyOutputs(); }
    else if (c == 'z' && dir == 0) zero();
  }
  if (dir != 0 && millis() - runStart > RUN_MS) { dir = 0; Serial.println("auto-stop"); applyOutputs(); }

  float m = measure(200);
  float d = m - offset_mV;
  Serial.printf("dir=%d duty=%d mean_mV=%.1f delta_mV=%.1f approx_A=%.2f\n", dir, duty, m, d, d / MV_PER_A);
  delay(100);
}
