// phase8b_mqtt_hello v0.1: Wi-Fi + MQTT publish test (no sensors, no motor)
#include <WiFi.h>
#include <PubSubClient.h>
#include "secrets.h"

const char* FW_VERSION  = "0.1";
const char* DEVICE_ID   = "rig1";
const char* TOPIC_DATA  = "iiot/rig1/hello";
const char* TOPIC_STATE = "iiot/rig1/status";   // retained: online / offline
const uint32_t PUBLISH_MS = 2000;
const uint32_t RETRY_MS   = 5000;

const int PIN_ENA = 25, PIN_IN1 = 26, PIN_IN2 = 27;

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
uint32_t lastPub = 0, lastTry = 0, seq = 0;

void connectWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("WiFi connecting");
  uint32_t t = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t < 20000) {
    delay(500);
    Serial.print('.');
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("\nWiFi OK, ESP32 IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi FAILED (will retry in loop)");
  }
}

bool connectMqtt() {
  Serial.printf("MQTT connecting to %s:%d ... ", MQTT_HOST, MQTT_PORT);
  // Last Will: the broker publishes "offline" if we vanish without saying goodbye.
  bool ok = mqtt.connect(DEVICE_ID, MQTT_USER, MQTT_PASS,
                         TOPIC_STATE, 0, true, "offline");
  if (ok) {
    Serial.println("OK");
    mqtt.publish(TOPIC_STATE, "online", true);
  } else {
    Serial.printf("FAILED, rc=%d\n", mqtt.state());
  }
  return ok;
}

void setup() {
  pinMode(PIN_ENA, OUTPUT); pinMode(PIN_IN1, OUTPUT); pinMode(PIN_IN2, OUTPUT);
  digitalWrite(PIN_ENA, LOW); digitalWrite(PIN_IN1, LOW); digitalWrite(PIN_IN2, LOW);

  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.printf("=== phase8b_mqtt_hello v%s ===\n", FW_VERSION);

  connectWifi();
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
}

void loop() {
  uint32_t now = millis();

  if (WiFi.status() != WL_CONNECTED) {
    if (now - lastTry >= RETRY_MS) {
      lastTry = now;
      Serial.println("WiFi lost, reconnecting");
      WiFi.disconnect();
      WiFi.begin(WIFI_SSID, WIFI_PASS);
    }
    return;
  }

  if (!mqtt.connected()) {
    if (now - lastTry >= RETRY_MS) {
      lastTry = now;
      connectMqtt();
    }
    return;
  }

  mqtt.loop();

  if (now - lastPub >= PUBLISH_MS) {
    lastPub = now;
    char payload[160];
    snprintf(payload, sizeof(payload),
             "{\"device_id\":\"%s\",\"fw\":\"%s\",\"seq\":%lu,\"uptime_ms\":%lu,\"rssi\":%d}",
             DEVICE_ID, FW_VERSION, (unsigned long)seq++, (unsigned long)now, WiFi.RSSI());
    bool ok = mqtt.publish(TOPIC_DATA, payload);
    Serial.printf("pub %s -> %s\n", ok ? "ok " : "ERR", payload);
  }
}
