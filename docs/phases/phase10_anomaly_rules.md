# Phase 10 (part B): threshold condition rules

Date: 2026-10-25. Code: `backend/rules.py` v0.1, `backend/replay_rules.py`, `backend/tests/test_rules.py`,
`backend/api.py` v0.2 (`/api/anomalies`), dashboard card in `frontend/src/App.tsx`.

## Scope (read this first)
These are **transparent threshold rules that flag unusual operating conditions** on a small 6 V demo rig.
They are **not** failure prediction and **not** a validated detector: there is no labelled failure data, and the
thresholds were read off the same stored data they are replayed on. No machine learning is used, because the data
does not contain enough representative, labelled behaviour to support it.

## Rules
| Rule | Fires when | Numbers and where they come from |
|---|---|---|
| DATA_STALE | newest telemetry older than 5 s, or device status "offline" | Cadence is 500 ms (median gap 0.51 s). Stored reboot/session gaps were 4.7-10.9 s and longer. |
| NO_SPEED_WHILE_COMMANDED | commanded fwd/rev at duty 195, not settling, pulses_per_s < 50 for 2 windows | All settled running windows with pulses were >= 154 pulses/s (about 200 typical) at duty 195. Only duty 195 was ever measured, so other duties are **not evaluated**. |
| SPEED_WHILE_STOPPED | commanded stop, after 3 grace windows, pulses_per_s > 20 for 2 windows | Pulses after a stop appeared only in the first 2 stop windows (coast-down). 0 pulses in all later stop windows (300+). |
| VIBRATION_AT_REST | stopped (after grace), IMU window valid, vib_rms_g > 0.02 g for 3 valid windows | Quiet session 2026-10-19: median 0.005 g, max 0.0118 g (229 valid windows). Single spikes up to 0.5 g occurred on 2026-10-24, so one window never alarms. Invalid windows neither count nor reset. |

Counters reset on reboot (`seq` goes backwards) and whenever the motor is commanded to run.

## Replay over the stored data (834 rows, 2026-10-19 .. 2026-10-24)
Raw output: `docs/logs/phase10_replay_output.txt`.
- NO_SPEED_WHILE_COMMANDED: 5 episodes, all on 2026-10-24 (five commanded "fwd" runs with 0 pulses/s throughout).
  0 on 2026-10-19, where all four commanded runs reached about 200 pulses/s. The data alone does not show why the
  2026-10-24 runs had no pulses (that session was a live dashboard test with the motor off).
- VIBRATION_AT_REST: 5 episodes, all on 2026-10-24; 0 on the quiet 2026-10-19 session. The cause of those vibrations
  is unknown; a bump of the table or board is a guess, not an observation.
- SPEED_WHILE_STOPPED: 0 episodes. **No real event exists in the data**, so it is tested only on synthetic rows.
- DATA_STALE: 7 gap episodes (reboots and sessions between runs), including the 4.8-day gap between sessions.
- "0 alarms on 2026-10-19" holds partly by construction: the thresholds were chosen from that data.

## Tests
`cd backend && python3 -m unittest discover -s tests -v` : 9 tests on synthetic rows (quiet rest, healthy run,
no-speed, other duty ignored, speed-while-stopped, 3-window vibration rule, invalid windows, reboot reset, staleness).
Output: `docs/logs/phase10_unit_tests.txt`. The `/api/anomalies` endpoint and the dashboard card were exercised
against a copy of the stored database (API test client, `tsc`, ESLint, Vite build). **Not yet run live** with the
ESP32, broker and browser.

## What cannot be done yet
- Current rules: current is unusable with Wi-Fi on and uncalibrated.
- Temperature rules: the DS18B20 is broken.
- Vibration rules while the motor runs: the IMU readings are invalid then (I2C errors), so no baseline exists.
- Anything about real failures: no failure data. Rule thresholds are valid only for this rig, this mount, duty 195.

## How to run
Backend and dashboard as before; `/api/anomalies` returns active alerts for the newest 120 windows (about 60 s).
Replay: `cd backend && python3 replay_rules.py`.
