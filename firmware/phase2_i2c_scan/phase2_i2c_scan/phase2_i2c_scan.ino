// Phase 2a: I2C scanner
// Firmware v0.1 | ESP32 DevKit V1 | SDA = GPIO 21, SCL = GPIO 22

#include <Wire.h>

const int SDA_PIN = 21;
const int SCL_PIN = 22;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);   // 100 kHz: slowest standard speed, easiest to debug
  Serial.println();
  Serial.println("=== I2C scanner v0.1 ===");
}

void loop() {
  int found = 0;
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    uint8_t err = Wire.endTransmission();
    if (err == 0) {
      Serial.printf("Device found at 0x%02X\n", addr);
      found++;
    } else if (err == 4) {
      Serial.printf("Unknown error at 0x%02X\n", addr);
    }
  }
  if (found == 0) Serial.println("No I2C devices found");
  else Serial.printf("%d device(s) found\n", found);
  Serial.println("---");
  delay(3000);
}