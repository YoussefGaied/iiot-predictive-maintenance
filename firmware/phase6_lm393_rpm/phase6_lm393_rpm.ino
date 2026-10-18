// phase6b_pulse_width v0.1 -- measure how long the LM393 D0 output stays HIGH, with the motor running
// Purpose: find out whether real blade pulses exist at motor speed, and how wide they are,
// versus short noise spikes. No filter is applied: every HIGH pulse is measured and put in a bucket.
// ESP32 DevKit V1 | LM393 D0 on GPIO 18 | motor ENA 25, IN1 26, IN2 27 | no IMU in this sketch.
//
// Commands (Serial Monitor, 115200): f forward, r reverse, s stop, + / - duty +/-10. Auto-stop after 3 s.
// Every 500 ms one line: how many HIGH pulses fell in each width bucket (microseconds):
//   b0 <20 | b1 20-49 | b2 50-99 | b3 100-199 | b4 200-499 | b5 500-999 | b6 >=1000
// Also: rise = number of rising edges, d0 = current level of D0.

const int PIN_D0 = 18;
const int PIN_ENA = 25, PIN_IN1 = 26, PIN_IN2 = 27;
const int PWM_FREQ = 5000, PWM_BITS = 8, DUTY_MAX = 255, DUTY_STEP = 10;
const unsigned long RUN_MS = 3000, WINDOW_MS = 500;

volatile uint32_t bucket[7] = {0, 0, 0, 0, 0, 0, 0};
volatile uint32_t riseCount = 0;
volatile uint32_t tRise = 0;
volatile bool isHigh = false;

int duty = 145, dir = 0;
unsigned long runStart = 0;

void IRAM_ATTR onEdge() {
  uint32_t now = micros();
  if (digitalRead(PIN_D0)) {
    tRise = now;
    isHigh = true;
    riseCount++;
  } else if (isHigh) {
    uint32_t w = now - tRise;
    isHigh = false;
    int b = (w < 20) ? 0 : (w < 50) ? 1 : (w < 100) ? 2 : (w < 200) ? 3 : (w < 500) ? 4 : (w < 1000) ? 5 : 6;
    bucket[b]++;
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
  Serial.println();
  Serial.println("=== phase6b_pulse_width v0.1  (f/r/s, +/-) ===");
  Serial.println("buckets in us: b0 <20 | b1 20-49 | b2 50-99 | b3 100-199 | b4 200-499 | b5 500-999 | b6 >=1000");
  applyOutputs();
}

void loop() {
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'f') { dir = 1; applyOutputs(); }
    else if (c == 'r') { dir = -1; applyOutputs(); }
    else if (c == 's') { dir = 0; applyOutputs(); }
    else if (c == '+') { duty = min(duty + DUTY_STEP, DUTY_MAX); applyOutputs(); }
    else if (c == '-') { duty = max(duty - DUTY_STEP, 0); applyOutputs(); }
  }

  if (dir != 0 && millis() - runStart > RUN_MS) {
    dir = 0;
    Serial.println("auto-stop");
    applyOutputs();
  }

  static unsigned long lastReport = 0;
  if (millis() - lastReport >= WINDOW_MS) {
    lastReport = millis();
    uint32_t b[7];
    for (int i = 0; i < 7; i++) { b[i] = bucket[i]; bucket[i] = 0; }
    uint32_t rise = riseCount;
    riseCount = 0;
    Serial.printf("dir=%d duty=%d d0=%d rise=%lu b0=%lu b1=%lu b2=%lu b3=%lu b4=%lu b5=%lu b6=%lu\n",
                  dir, duty, digitalRead(PIN_D0), (unsigned long)rise,
                  (unsigned long)b[0], (unsigned long)b[1], (unsigned long)b[2], (unsigned long)b[3],
                  (unsigned long)b[4], (unsigned long)b[5], (unsigned long)b[6]);
  }
}
