# Phase 4: Drive the 6 V motor with the L298N

**Status:** Done (2026-10-11)

## Objective

Run the 6 V DC motor from the ESP32 through the L298N: forward, reverse,
stop, and PWM speed control, with a stop that always works.

## Hardware

- ESP32 DevKit V1 (USB power), L298N red module, mini 6 V DC motor "310DC"
- 6xAA holder with barrel plug, plugged into the female barrel-jack
  screw-terminal adapter
- No multimeter and no LED were available, so no voltage or polarity
  measurement was possible.

## Identification (read from the board / told by owner)

- L298N screw terminals: OUT1, OUT2, +12V, GND, +5V, OUT3, OUT4.
- Edge pin header, in order: ENA, IN1, IN2, IN3, IN4, ENB.
- Jumper caps: one on each end of the edge header (ENA/ENB, inferred from
  layout) and one beside the 3-way power terminal. Its label was not
  readable in photos, so it was not read; it is assumed to be the 5 V
  regulator enable jumper.
- Holder slots are marked + / -. Slot contacts are continuous (series).

## Wiring

| From | To | Purpose |
|---|---|---|
| Motor wire 1 / 2 | L298N OUT1 / OUT2 | Motor A |
| Adapter terminals | L298N +12V / GND | Motor supply (6 AA) |
| ESP32 VIN | L298N +5V terminal | Logic supply from USB 5 V |
| ESP32 GND | L298N GND | Common ground |
| ESP32 GPIO 25 | ENA | PWM speed (ENA jumper cap removed) |
| ESP32 GPIO 26 | IN1 | Direction |
| ESP32 GPIO 27 | IN2 | Direction |

The cap beside the power terminal was removed so the board's regulator is
disconnected from the +5V terminal, which is now an input from the ESP32.
Do not put it back while the +5V terminal is fed from the ESP32 VIN pin.
ENB cap left in place (motor B unused).

## Firmware

`firmware/phase4_motor_test` v0.1. Serial commands at 115200 baud:
`f` forward, `r` reverse, `s` stop, `+` / `-` duty by 10. Outputs are LOW
at boot, and the motor auto-stops after 3 s. PWM 5 kHz, 8-bit. As run:
`DUTY_MAX = 255`, initial duty 255.

## Observations

| Check | Result |
|---|---|
| Motor wired directly to 4 AA | Spins fine |
| L298N power LED | Red LED lit when supply connected |
| L298N driven by hand (IN1 touched to +5V terminal, logic from board regulator, 4 AA) | Motor did not turn |
| Same by-hand test with logic fed from ESP32 VIN (regulator jumper removed) | Motor spun |
| `f` / `r` (duty 255) | Spins in both directions |
| `s` | Stops motor (f then s, r then s) |
| 3 s auto-stop | Observed (`auto-stop` in log) |
| Speed change with `-` | Speed changes |
| Lowest duty that still starts from rest | 145 (duty step is 10, so resolution is coarse) |
| Supply used for these results | 6 fresh AA, no bridge wires |
| 4 AA pack | Owner judged the motor weak, so he changed to 6 AA |
| Motor temperature | Owner reports it did not heat up (not measured) |

Raw capture: `docs/logs/phase4_run1_serial.txt`.

## Issues

- A first firmware test hummed without turning because the old Phase 3
  DS18B20 sketch was still uploaded: GPIO 25/26/27 were never set up. After
  uploading the Phase 4 sketch the motor ran. (Floating inputs are the
  likely cause; not tested directly.)

## Limitations

- Pack voltage and motor terminal voltage were never measured (no
  multimeter), and the barrel adapter polarity was not measured. The
  L298N power LED lit and nothing misbehaved.
- 6 fresh AA is about 9 V nominal for a 6 V motor. After the L298N drop the
  motor voltage is unknown. Runs were short (3 s limit). Long runs and
  stalls remain to be avoided.
- The lowest starting duty of 145 belongs to this motor, this pack's
  charge state and the 10-step grid. It is not a specification.
- The L298N datasheet gives about 1.2-1.35 V typical drop per side at 1 A
  (about 2.5 V for both sides). The drop at this motor's actual current
  was not measured.

## Done criteria

Forward, reverse, stop, auto-stop and speed change all work from code. Met.

## Next

Phase 5: ACS712 current sensing. Read the module variant and pin labels
first. A multimeter is strongly recommended for zero-current offset
calibration and for a safe ADC interface check.
