# Phase 9: React + TypeScript dashboard

Date: 2026-10-20. Tested on stored data only (ESP32 not connected).

## What was built
- frontend/ (Vite + React + TypeScript + Recharts), dev server on port 5173.
- api.ts: typed API client. usePolling.ts: 2 s polling with connection state.
  useTelemetryHistory.ts: loads up to 600 stored rows, then only new rows via after_id.
- App.tsx: status cards, staleness ("last data X ago"), three charts, API caveats.

## Honesty rules in the UI
- Vibration shown only when vib_valid = 1, otherwise "not valid" with n, err, amean_g.
- RPM labelled as assuming 2 pulses/rev, unverified.
- Current shown as raw mV change from the 'z' rest level, uncalibrated, not amps.
- Settling windows left out of the charts. No temperature (DS18B20 broken).

## Results (observed)
- API polling worked; the page reported connection loss and recovered (9B test).
- Stored data, 359 rows: stop 335, fwd 12, rev 12 (settling rows included).
- Speed chart: bursts of about 200-220 pulses/s in the run sessions.
- Vibration chart: valid only in motor-off windows, about 0.005 g with a few short spikes (cause unknown).
- Current chart: noisy, also moves while the motor is stopped. Not usable.

## Problems found
- The API returns the NEWEST `limit` rows with id > after_id, not the oldest. With limit 200 the
  dashboard missed older runs. Fixed with limit 600. Found by curl checks; my first guess was wrong.
- After editing the hook, Vite hot reload kept old state; a hard refresh (Ctrl+Shift+R) was needed.

## Not tested
- Live data from the ESP32 (online status, growing rows, incremental fetch). To do when home.

## Limits
- History keeps the last 600 rows. Time axis is real time, so gaps between sessions are compressed.
