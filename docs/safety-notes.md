# Safety Notes

Read before wiring. Rules 1-16 are from the start of the project; 17-22 were added during Phases 4-9.

1. Disconnect all power before changing wiring.
2. Never power a motor from an ESP32 GPIO or 3.3 V pin.
3. Do not connect a fresh 6xAA alkaline pack directly to the 6 V motor without checking voltage and planning a safe
   arrangement. The L298N is a driver, not a regulator, and drops voltage internally.
   *(Status: this rig runs six fresh AA cells through the L298N, about 9 V nominal, voltage never measured, the
   builder's choice. Keep runs short.)*
4. Power the ESP32 through USB. Never connect the battery pack to the 3V3 pin. Do not feed an unknown voltage into
   VIN/5V.
5. With a separate motor supply, the ESP32 and L298N need a common ground.
6. Verify the L298N terminal labels, enable jumper, and regulator arrangement on the actual board.
7. Verify adapter polarity from its label or with a multimeter. *(Not yet measured; the L298N power LED lights.)*
8. Do not connect or disconnect the motor while powered. Keep fingers, hair, loose wires, and objects away from the
   shaft.
9. Test one subsystem at a time. Avoid prolonged runs, intentional stalls, and deliberate overheating.
10. Confirm the DS18B20 marking and pinout from its datasheet before powering. Reversed polarity heats the part
    quickly; this applies to any replacement sensor.
11. The DS18B20 data line needs a pull-up to 3.3 V (4.7 kOhm standard).
12. The MPU6050 uses I2C. Use ESP32-safe logic levels; never expose ESP32 pins to 5 V. Power the IMU and the LM393
    from 3V3 only.
13. Many ACS712 modules run from 5 V and can output voltages above the ESP32 ADC's safe range. Keep the series
    resistor between ACS712 OUT and the ADC pin.
14. Verify the LM393 module's output type and voltage; the ESP32 input must never see 5 V.
15. If anything smells unusual, becomes hot, or the ESP32 resets unexpectedly, disconnect power immediately.
16. With the L298N +5V terminal fed from ESP32 VIN, the 5V-enable jumper/cap must stay removed.
17. Keep face and hands out of the plane of the target disc: with fresh cells the motor reaches very high speed.
18. Motor sketches drive GPIO 25/26/27 LOW in `setup()`. A previously uploaded sketch can leave those pins floating and
    make the motor hum, so check the Serial Monitor banner before testing hardware.
19. Firmware stops the motor after 3 s, and Wi-Fi/MQTT reconnects are never attempted while the motor runs, so a slow
    network cannot delay the stop. Keep both properties if the firmware is changed.
20. Mosquitto listens on the LAN with a password but without encryption. Use it only on a trusted network, never forward
    port 1883, never use public Wi-Fi.
21. The API binds to 127.0.0.1 and has no authentication. Do not expose it.
22. `secrets.h` (Wi-Fi and MQTT credentials) and `backend/data/*.db` stay out of git.

## Still unverified (as of 2026-10-25)
- Battery pack voltage and the voltage at the motor.
- Barrel adapter polarity (not measured).
- Label of the jumper cap beside the L298N power terminal (assumed to be the 5V-enable cap).
