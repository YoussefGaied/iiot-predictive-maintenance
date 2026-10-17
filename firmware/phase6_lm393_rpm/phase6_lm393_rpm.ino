// phase6_lm393_rpm  v0.2 -- slot-type LM393 pulse counting + RPM
// Commands: f r s  (motor)   + -  (duty +/- 10)   p<number> (set pulses per revolution, e.g. p1)
//           c (clear total pulse count)
//           m<microseconds> (set chatter filter, e.g. m50000 for slow hand turning, m500 for motor runs)
// RPM is printed ONLY after you set pulses per revolution (PPR) with p<number>.
// PPR is never assumed: measure it by hand-turning the shaft (see Phase 6 steps).
// Safety: motor outputs LOW at boot, motor auto-stops after RUN_MS.

const int PIN_D0  = 18;                  // LM393 D0 (module powered from 3V3)
const int PIN_ENA = 25, PIN_IN1 = 26, PIN_IN2 = 27;   // same as Phase 4/5

const int PWM_FREQ = 5000, PWM_BITS = 8, DUTY_MAX = 255, DUTY_STEP = 10;
const unsigned long RUN_MS = 3000;            // motor auto-stop
const unsigned long WINDOW_MS = 500;          // report interval

volatile unsigned long pulseCount = 0;
volatile unsigned long lastEdgeUs = 0;
volatile unsigned long minPulseUs = 500;      // ignore edges closer than this (chatter filter)

int duty = 145, dir = 0;                      // dir: 0 stop, 1 forward, -1 reverse
int ppr = 0;                                  // pulses per revolution, 0 = unknown
unsigned long runStart = 0;

void IRAM_ATTR onPulse() {
  unsigned long now = micros();
  if (now - lastEdgeUs >= minPulseUs) {
    pulseCount++;
    lastEdgeUs = now;
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

  pinMode(PIN_D0, INPUT_PULLUP);              // pull-up keeps the pin defined if the wire comes loose
  attachInterrupt(digitalPinToInterrupt(PIN_D0), onPulse, RISING);

  Serial.begin(115200);
  Serial.setTimeout(50);
  delay(500);
  Serial.println("phase6_lm393_rpm v0.2  (f/r/s, +/-, p<n>, m<us>, c)  ppr=unknown");
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
    else if (c == 'p') {
      int v = Serial.parseInt();
      ppr = (v > 0) ? v : 0;
      Serial.printf("ppr=%d\n", ppr);
    }
    else if (c == 'm') {
      int v = Serial.parseInt();
      if (v >= 100) minPulseUs = v;
      Serial.printf("min_us=%lu\n", minPulseUs);
    }
    else if (c == 'c') { pulseCount = 0; Serial.println("total cleared"); }
  }

  if (dir != 0 && millis() - runStart > RUN_MS) {
    dir = 0;
    Serial.println("auto-stop");
    applyOutputs();
  }

  static unsigned long lastReport = 0;
  static unsigned long lastCount = 0;
  unsigned long nowMs = millis();
  if (nowMs - lastReport >= WINDOW_MS) {
    unsigned long count = pulseCount;
    if (count < lastCount) lastCount = 0;     // total was cleared
    unsigned long elapsed = nowMs - lastReport;
    unsigned long d = count - lastCount;
    float hz = d * 1000.0f / elapsed;
    Serial.printf("dir=%d duty=%d d0=%d pulses=%lu total=%lu hz=%.1f",
                  dir, duty, digitalRead(PIN_D0), d, count, hz);
    if (ppr > 0) Serial.printf(" rpm=%.0f", hz * 60.0f / ppr);
    Serial.println();
    lastCount = count;
    lastReport = nowMs;
  }
}
