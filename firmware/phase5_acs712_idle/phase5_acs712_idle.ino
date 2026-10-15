// phase5_acs712_idle  v0.1 -- ACS712 idle offset/noise, motor OFF
const int PIN_ACS = 34;
const int N = 500;                  // samples per report
int v[N];

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("phase5_acs712_idle v0.1");
  for (int p : {25, 26, 27}) { pinMode(p, OUTPUT); digitalWrite(p, LOW); }
  analogReadResolution(12);
  analogSetPinAttenuation(PIN_ACS, ADC_11db);
}

void loop() {
  long sum = 0; int mn = 99999, mx = 0;
  for (int i = 0; i < N; i++) {
    v[i] = analogReadMilliVolts(PIN_ACS);
    sum += v[i];
    if (v[i] < mn) mn = v[i];
    if (v[i] > mx) mx = v[i];
    delayMicroseconds(1000);
  }
  float mean = sum / (float)N, var = 0;
  for (int i = 0; i < N; i++) var += (v[i] - mean) * (v[i] - mean);
  Serial.printf("mean_mV=%.1f min=%d max=%d std_mV=%.1f\n", mean, mn, mx, sqrt(var / N));
}
