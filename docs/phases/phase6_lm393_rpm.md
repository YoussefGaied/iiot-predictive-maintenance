# Phase 6: LM393 slot-type speed sensor and RPM

Date: 2026-10-17. Firmware: `firmware/phase6_lm393_rpm/` v0.2 (v0.1 was used for the first logs and is superseded).

## Objective
Count pulses from the LM393 slot sensor while the motor turns and derive RPM. Document what is and is not reliable.

## Parts
- LM393 slot-type (fork) speed sensor module, header labels in board order: A0, D0, GND, VCC. Two small LEDs, no adjustment potentiometer seen in the photo. Slot gap about 5-6 mm (user estimate).
- Blade on the motor shaft: a rectangular piece of laminated cardboard/paper, centred on the shaft (two arms). Motor taped to a polystyrene block, sensor taped separately.
- ESP32 DevKit V1, L298N, 6 V "310DC" motor, 6 AA pack (voltage never measured).

## Wiring (plan given in chat; the colour-to-pin mapping was not typed out and cannot be read from the photo)
| LM393 pin | ESP32 | Purpose |
|---|---|---|
| VCC | 3V3 | Power, keeps D0 at or below 3.3 V |
| GND | GND | Common ground |
| D0 | GPIO 18 | Pulse input (RISING interrupt, INPUT_PULLUP) |
| A0 | not connected | Unused |

Motor wiring unchanged from Phase 4 (ENA 25, IN1 26, IN2 27).

## Firmware v0.2
Interrupt counts RISING edges on D0 with a settable chatter filter (default 500 us, command `m<us>`). Reports every 500 ms: `dir duty d0 pulses total hz [rpm]`. `rpm` is printed only after `p<n>` sets pulses per revolution. Commands: `f r s + - p<n> m<us> c`. Motor auto-stops after 3 s.

## Observations
- Idle: no pulses with the shaft still. `d0=0` with the slot clear, `d0=1` when the blade rests in the slot (inferred from the stopped states in the logs).
- v0.1 (500 us filter) hand-turning 3 full turns gave `total=10`: more than 1 or 2 pulses per turn. Slow motion through the beam edge produces extra edges (likely switching noise at the comparator; not scoped). Raised the filter with `m500000` for hand tests.
- Pulses per revolution: the user chose **PPR = 2** after a hand count with `m500000`. That count was not pasted into the chat, so PPR = 2 is user-reported, not independently logged. It is plausible for a two-armed blade.
- Always run motor tests with `m500` (or reboot); a large filter blocks real pulses at motor speed.
- Peak pulse rate seen: 504 Hz, well below the 2 kHz the 500 us filter allows.

## Results (mean of the 500 ms windows after the first one of each run; RPM = Hz x 30 assuming PPR = 2)
Old (used) 6 AA pack:
| Run | Mean Hz | About RPM | Range Hz | Note |
|---|---|---|---|---|
| fwd 255 #1 | 239 | 7,180 | 200-268 | |
| fwd 255 #2 | 210 | 6,310 | 140-290 | |
| fwd 195 | 77 | 2,320 | 46-118 | still rising at 3 s |
| fwd 145 | 10 | 300 | 4-18 | nearly stalled |
| rev 145 | 78 | 2,330 | 74-84 | steadiest run |
| rev 255 #1 | 31 | 940 | 10-56 | erratic |
| rev 255 #2 | 41 | 1,220 | 26-56 | erratic |
| fwd 255 (later) | 108 | 3,250 | 44-186 | still rising at 3 s |

Brand-new 6 AA pack, forward, duty 255 (5 runs):
| Run | Mean Hz | About RPM | Range Hz |
|---|---|---|---|
| 1 | 238 | 7,130 | 200-316 |
| 2 | 271 | 8,140 | 238-316 |
| 3 | 261 | 7,820 | 138-342 |
| 4 | 429 | 12,880 | 242-504 |
| 5 | 382 | 11,470 | 350-450 |

## Conclusions and limits
- The sensor and firmware count pulses reliably across the observed range (a few Hz to about 500 Hz).
- Motor speed is **not repeatable**: it depends strongly on the battery pack (old pack gave 24-186 Hz at forward 255 in one run, fresh pack 200-504 Hz), differs by direction, was not monotonic in duty on the old pack, and rose across consecutive runs on the fresh pack (238 to 429 Hz mean). The causes (battery state, motor brush behaviour, friction, warm-up) are not separated; no multimeter and no motor temperature measurement.
- Fresh 6 AA at duty 255 gave about 11,000-15,000 RPM (at PPR = 2) on a 6 V motor, with an unmeasured pack voltage and a paper blade on the shaft. Keep runs short; consider lower duty or a lower-voltage pack.
- Windows vary by up to about 20 % within a run even when the speed looks steady; 500 ms windows are coarse at low speed (resolution 2 pulses/s, 1 RPM step = 30 RPM at PPR = 2).
- The blade adds imbalance and the sensor is not rigidly tied to the motor mount. Vibration baselines in Phase 7 must be taken with the blade mounted.
- Reverse was not tested on the fresh pack. Duty values below 255 were not tested on the fresh pack.
- RPM is a measured quantity only as good as PPR = 2.

## Done criteria
- d0 changes with the blade in and out of the slot: met.
- Pulse count follows blade passes: partly met (PPR = 2 is user-reported).
- Steady, repeatable RPM from the motor: **not met**. The supply and motor speed repeatability go forward as the first task of Phase 7.

## Evidence still wanted
`docs/images/phase6_lm393_wiring.jpg` (sensor and blade setup) and `phase6_serial_output.png`.

## Logs
`docs/logs/phase6_old_battery_forward.txt`, `phase6_old_battery_reverse.txt`, `phase6_fresh_battery_forward255.txt` (pasted by the user; long idle stretches shortened to marker lines). The v0.1 runs and the hand-turn captures were pasted in chat but are not saved as files.
