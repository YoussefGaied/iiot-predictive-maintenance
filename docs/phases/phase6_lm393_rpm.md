# Phase 6: LM393 slot-type speed sensor and RPM (revised 2026-10-18)

This file REPLACES the first version written on 2026-10-17. That version reported RPM from rising-edge counts at
motor speed; those RPM values are invalid (see "What went wrong"). The raw logs of those runs are kept as evidence
(`phase6_*.txt`, `phase7b_*.txt`) but their pulse, hz and rpm columns must not be used.

## Objective
Measure motor speed with the LM393 slot sensor and document how far the result can be trusted.

## Parts and wiring
- LM393 slot-type (fork) module, header A0, D0, GND, VCC; slot gap about 5-6 mm (user estimate); A0 unused.
- Wiring (as instructed in chat; the colour-to-pin mapping was never typed out): VCC to 3V3, GND to GND, D0 to GPIO 18.
- Motor wiring unchanged from Phase 4 (ENA 25, IN1 26, IN2 27). 6 AA pack, voltage never measured.
- Target 1 (used first): rectangular cardboard/paper bar centred on the shaft.
- Target 2 (final): thin cardboard disc about 25-30 mm with two opposite sectors cut away (designed as two quarter
  sectors; the real cut was not photographed), black tape added on the sectors later.

## Firmware
- `phase6_lm393_rpm` v0.1 / v0.2 (earlier phase, RISING-edge counter): superseded for motor-speed use.
- `phase6b_pulse_width` v0.1: no filter; measures the HIGH time of every pulse on D0 and counts pulses in width
  buckets (b0 <20 us, b1 20-49, b2 50-99, b3 100-199, b4 200-499, b5 500-999, b6 >=1000), plus rising edges (`rise`).
- `phase7b_baseline` v0.5: counts a pulse only when D0 goes HIGH and then LOW after at least `w` us (default 100),
  reports `pulses`, `rise`, `pps`, and `rpm = pps * 60 / ppr` with a settable `ppr`.

## What went wrong, and how it was found
1. With the motor off, hand-turning the shaft slowly gave more counts than turns (3 turns gave 10 counts with a
   500 us filter): slow movement through the beam edge makes the comparator output flicker.
2. At motor speed the RISING-edge counter reported 800-1000 per second even with the blade REMOVED, and about 900 with
   the old blade fitted. A wait-and-recheck filter of 50 us or more removed all of it (see
   `phase7b_v04_blade_on_filter_sweep.txt`). So those counts were very short glitches, not blade passes. This makes
   every RPM figure taken at motor speed with the RISING-edge sketches invalid, including the Phase 6 first-version
   tables and the Phase 7B v0.1-v0.4 logs.
3. The pulse-width scan with the old bar blade (`phase6b_old_blade_scan.txt`) showed almost no pulse wider than 50 us
   at motor speed: the bar triggered the sensor only rarely (alignment or flutter; cause not isolated).
4. A thin cardboard disc with two opposite sectors (`phase6b_new_disc_untaped_scan.txt`) produced real pulses at motor
   speed. Black tape on the sectors (`phase6b_taped_disc_scan.txt`) lengthened the pulses and reduced extra edges a
   little. The completed-pulse counts were already steady before the tape.

## Result (6 AA pack, forward, completed pulses per 500 ms in the steady windows, two runs each after taping)
| Duty | Untaped disc | Taped disc, run 1 | Taped disc, run 2 | Pulses per second (taped) |
|---|---|---|---|---|
| 145 | 72.8 | 68.2 | 66.5 | about 134 |
| 195 | 97.5 | 93.5 | 92.2 | about 186 |
| 255 | 138.2 | 136.5 | 136.2 | about 273 |

Run-to-run agreement is about 2 % in the taped runs; the count rises steadily with duty. `rise` is 1.5-4 times higher
than the completed pulses, so rising edges must not be used.

## RPM
RPM = pulses per second / pulses per revolution. **Pulses per revolution of the final disc was NOT verified**: the
planned slow hand-turn count was skipped at the user's request. With the assumed 2 per revolution the speeds would be
about 4,000 / 5,600 / 8,200 RPM at duty 145 / 195 / 255; with 1 per revolution they would be double. The pulses are
only about 10-20 % of each cycle, narrower than two quarter sectors would give (about 50 %), so the disc as cut may
differ from the design. No tachometer was available, so no absolute RPM accuracy is claimed. The measured, valid
quantity is pulses per second.

## Limits
- Pulses per revolution assumed, not verified. Reverse and duty values other than 145 / 195 / 255 not tested with the
  final disc. Windows are 500 ms; the first window of each run includes acceleration and is excluded above.
- Speed depends strongly on the pack (an older pack gave much lower and erratic speeds in earlier runs). Pack voltage
  was never measured.
- Motor-speed readings need the completed-pulse method; slow hand-turning still shows flicker.

## Done criteria
- Repeatable speed signal at motor speed (pulses per second): met.
- Pulses per revolution verified: NOT met (assumed 2).
- Absolute RPM validated against a reference: NOT met (no reference).

## Evidence still wanted
`docs/images/phase6_target_disc_and_sensor.jpg` (disc mounted next to the sensor), `phase6_serial_output.png`.

## Check with firmware phase7b_baseline v0.5 (2026-10-18, `phase7b_v05_check.txt`)
v0.5 counts completed pulses (HIGH then LOW after at least 100 us). Steady windows, completed pulses per 500 ms:
duty 195: about 106 and 100 (two runs, about 210 and 200 pulses per second); duty 255: about 145 and 108 (about 290 and
216 pulses per second). Rising edges were 2-6 times higher than completed pulses, which confirms the glitchy edges.
The counting method works. The speed itself did not repeat as closely as in the earlier scan: at duty 255 the two runs
differ by about 25 %, and duty 195 is about 10-15 % above the earlier scan. The cause (pack, motor warm-up, brush state,
tape or disc condition) was not isolated, so the "about 2 %" agreement above applies only to that earlier pair of runs and
must not be quoted as the repeatability of the speed. These runs were done with the IMU on.
