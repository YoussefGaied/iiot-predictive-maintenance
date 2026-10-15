# Phase 5 - ACS712 current sensing (20 A module)

Status: done as a measurement of **uncalibrated** current. No reference
ammeter or multimeter was available, so no absolute accuracy is claimed.

## Setup
- Module chip marking: "ACS712TELC-20A25331" -> 20 A variant, 100 mV/A
  (datasheet value). Module signal pins: VCC, OUT, GND.
- Signal wiring (5A and 5B): VCC -> ESP32 VIN (5 V); GND -> ESP32 GND;
  OUT -> 10 kOhm series resistor -> GPIO 34 (ADC1, input-only, 11 dB
  attenuation, `analogReadMilliVolts`). The series resistor limits fault
  current into the pin (design judgement, not a datasheet guarantee).
- 5A: USB power only, nothing in the current path, motor off.
- 5B: sensor inserted in the motor supply path: battery adapter positive
  wire -> ACS712 screw terminal -> second terminal -> L298N +12V.
  Ground wires unchanged. Motor supply: 6 fresh AA, voltage never measured.
- Firmware: `phase5_acs712_idle` v0.1 (5A), `phase5_acs712_motor` v0.1 (5B,
  commands f r s + - z, 3 s auto-stop, 200-sample average per line).

## 5A results (motor off, USB only)
- Offset about 2578-2587 mV; std about 15-18 mV per line (500 samples);
  min-max span about 100 mV per line.
- Mean drifted from about 2585 to 2578 mV over roughly one minute
  (about 70 mA apparent at 100 mV/A): re-zero before each test.

## 5B results
Run 1: the sketch's boot offset was about 2500 mV, wrong versus the real
rest level (about 2582 mV), so its delta/approx_A columns are invalid.
Recomputed from raw mean_mV against the rest mean (about 2582.5 mV):

| State | Change vs rest | Magnitude (uncalibrated) |
|---|---|---|
| Fwd / rev, duty 145 | about -13 / -14 mV | about 0.13 / 0.14 A |
| Fwd / rev, duty 255 | about -21 / -17 mV | about 0.21 / 0.17 A |
| First line of a run | about -45 mV | about 0.45 A (200 ms average) |

Run 2 (clean, after `z` at rest with battery on; offsets 2584.5-2585.6 mV;
average of lines after the first of each run):

| State | Change vs rest | Magnitude (uncalibrated) |
|---|---|---|
| Rest, motor stopped | 0, scatter about +/-1 mV between lines | about 0 |
| Fwd / rev, duty 145 | about -11 / -10.5 mV | about 0.11 / 0.10 A |
| Fwd / rev, duty 255 | about -16.6 / -16.2 mV | about 0.17 / 0.16 A |
| First line of a run | about -41 mV | about 0.41 A (200 ms average) |

Notes (observations, not specifications):
- Current reads as a **drop** (negative delta) in both directions: the
  module's terminals are connected opposite to its IP+ -> IP-. Only the
  magnitude is used.
- Battery on with the motor stopped does not move the reading.
- Run versus stop is clearly detectable (10-20 mV against about +/-1-2 mV
  scatter). Duty 145 versus 255 differed by about 5-6 mV in both runs, but
  run-to-run values differed by 3-5 mV, so duty levels are not reliably
  separated.
- Within a run the reading creeps back toward rest; cause not investigated.
- Motor heat was not measured.

## Limits
- 20 A range on a motor drawing roughly 0.1-0.2 A: resolution is poor.
- No reference instrument: amps are datasheet scaling only. ESP32 ADC
  absolute accuracy unverified. A 5 A variant would give about 185 mV/A.
- Do not publish `current_a` as calibrated; label it uncalibrated.

## Definition of done: met
Sensor read safely, offset established with battery on, run vs stop
detectable, limits documented.

## Next
Phase 6: LM393 speed sensor and RPM.
