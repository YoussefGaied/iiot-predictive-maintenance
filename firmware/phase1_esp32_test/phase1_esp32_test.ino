// Phase 1: ESP32 USB / upload / Serial test
// Firmware v0.1
// Board: ESP32 DevKit V1 (USB-C), Arduino IDE, "ESP32 Dev Module", 115200 baud

#ifndef LED_BUILTIN
#define LED_BUILTIN 2   // common onboard LED pin on many ESP32 DevKits; verify on your board
#endif

const unsigned long BLINK_INTERVAL_MS = 1000;
unsigned long lastToggle = 0;
bool ledState = false;
unsigned long counter = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);  // give the Serial Monitor time to connect

  pinMode(LED_BUILTIN, OUTPUT);

  Serial.println();
  Serial.println("=== ESP32 Phase 1 test ===");
  Serial.printf("Chip model:    %s\n", ESP.getChipModel());
  Serial.printf("Chip revision: %d\n", ESP.getChipRevision());
  Serial.printf("CPU cores:     %d\n", ESP.getChipCores());
  Serial.printf("CPU freq:      %d MHz\n", ESP.getCpuFreqMHz());
  Serial.printf("Flash size:    %u bytes\n", ESP.getFlashChipSize());
  Serial.printf("MAC address:   %012llX\n", ESP.getEfuseMac());
  Serial.printf("LED pin used:  GPIO %d\n", LED_BUILTIN);
  Serial.println("==========================");
}

void loop() {
  unsigned long now = millis();
  if (now - lastToggle >= BLINK_INTERVAL_MS) {
    lastToggle = now;
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState);
    Serial.printf("t=%lu ms | count=%lu | LED=%s\n",
                  now, counter++, ledState ? "ON" : "OFF");
  }
}
