// Phase 3: DS18B20 temperature test
// Firmware v0.1 | ESP32 DevKit V1 | DQ = GPIO 4 | 3V3 powered (not parasite)

#include <OneWire.h>
#include <DallasTemperature.h>

const int ONE_WIRE_PIN = 4;
const unsigned long READ_INTERVAL_MS = 2000;
const unsigned long CONVERSION_WAIT_MS = 800;  // 12-bit conversion takes up to 750 ms

OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature sensors(&oneWire);
DeviceAddress addr;

unsigned long lastRequest = 0;
unsigned long conversionStart = 0;
bool conversionPending = false;

void printAddress(const DeviceAddress a) {
  for (uint8_t i = 0; i < 8; i++) Serial.printf("%02X", a[i]);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println("=== DS18B20 test v0.1 ===");

  sensors.begin();
  int count = sensors.getDeviceCount();
  Serial.printf("Devices on bus (GPIO %d): %d\n", ONE_WIRE_PIN, count);
  if (count == 0) {
    Serial.println("ERROR: no device found. Check wiring, pull-up resistor, and pin.");
    while (true) delay(1000);
  }

  if (!sensors.getAddress(addr, 0)) {
    Serial.println("ERROR: could not read device address");
    while (true) delay(1000);
  }
  Serial.print("ROM address: ");
  printAddress(addr);
  Serial.println();
  Serial.printf("Family code: 0x%02X (DS18B20 = 0x28)\n", addr[0]);
  Serial.printf("Parasite power mode: %s\n", sensors.isParasitePowerMode() ? "yes" : "no");

  sensors.setResolution(addr, 12);
  sensors.setWaitForConversion(false);   // do not block while the sensor converts
  Serial.println("Columns: t_ms, temperature_C");
}

void loop() {
  unsigned long now = millis();

  if (!conversionPending && now - lastRequest >= READ_INTERVAL_MS) {
    sensors.requestTemperatures();
    lastRequest = now;
    conversionStart = now;
    conversionPending = true;
  }

  if (conversionPending && now - conversionStart >= CONVERSION_WAIT_MS) {
    conversionPending = false;
    float t = sensors.getTempC(addr);
    if (t == DEVICE_DISCONNECTED_C) {
      Serial.printf("%lu, read error (-127: no device / bad connection)\n", now);
    } else if (fabsf(t - 85.0f) < 0.01f) {
      Serial.printf("%lu, %.2f  <- 85.00 is the power-on default, not a real reading\n", now, t);
    } else {
      Serial.printf("%lu, %.2f\n", now, t);
    }
  }
}