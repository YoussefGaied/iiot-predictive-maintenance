# Safety Notes

Read before wiring.

1. Disconnect all power before changing wiring.
2. Never power a motor from an ESP32 GPIO or 3.3 V pin.
3. Do not connect a fresh 6×AA alkaline pack directly to the 6 V motor
   without checking voltage and planning a safe arrangement. The L298N is a
   driver, not a regulator, and drops voltage internally.
4. Power the ESP32 through USB at first. Never connect the battery pack to
   the 3V3 pin. Do not feed an unknown voltage into VIN/5V.
5. With a separate motor supply, the ESP32 and L298N need a common ground.
6. Verify the L298N terminal labels, enable jumper, and regulator
   arrangement on the actual board.
7. Verify adapter polarity from its label or with a multimeter.
8. Do not connect or disconnect the motor while powered. Keep fingers, hair,
   loose wires, and objects away from the shaft.
9. Test one subsystem at a time. Avoid prolonged runs, intentional stalls,
   and deliberate overheating.
10. Confirm the DS18B20 marking and pinout from its datasheet before
    powering.
11. The DS18B20 data line needs a pull-up to 3.3 V (4.7 kOhm standard).
12. The MPU6050 uses I2C. Use ESP32-safe logic levels; never expose ESP32
    pins to 5 V.
13. Many ACS712 modules run from 5 V and can output voltages above the
    ESP32 ADC's safe range. Design a safe interface before connecting.
14. Verify the LM393 module's output type and voltage; the ESP32 input must
    never see 5 V.
15. If anything smells unusual, becomes hot, or the ESP32 resets
    unexpectedly, disconnect power immediately.
