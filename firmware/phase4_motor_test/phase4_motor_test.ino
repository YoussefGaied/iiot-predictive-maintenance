// Phase 4 - L298N motor test v0.1
// Serial commands: f = forward, r = reverse, s = stop, + / - = duty +/- 10
// Safety: duty capped, motor auto-stops after RUN_MS, outputs LOW at boot.

const int PIN_ENA = 25;
const int PIN_IN1 = 26;
const int PIN_IN2 = 27;f


const int PWM_FREQ = 5000;       // Hz
const int PWM_BITS = 8;          // duty 0..255
const int DUTY_MAX = 255;        // cap: about 78 %
const int DUTY_STEP = 10;
const unsigned long RUN_MS = 3000;  // auto-stop after 3 s

int duty = 255;                  // starting duty
int dir = 0;                     // 0 stop, 1 forward, -1 reverse
unsigned long runStart = 0;

void pwmWrite(int value) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(PIN_ENA, value);
#else
  ledcWrite(0, value);
#endif
}

void applyOutputs() {
  if (dir == 0) {
    pwmWrite(0);
    digitalWrite(PIN_IN1, LOW);
    digitalWrite(PIN_IN2, LOW);
  } else {
    digitalWrite(PIN_IN1, dir > 0 ? HIGH : LOW);
    digitalWrite(PIN_IN2, dir > 0 ? LOW : HIGH);
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
  Serial.begin(115200);
  Serial.println("Phase 4 motor test v0.1  (f/r/s, +/-)");
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
}