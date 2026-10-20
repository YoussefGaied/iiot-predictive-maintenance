# Dashboard (Phase 9)

React + TypeScript + Vite + Recharts. Reads the read-only FastAPI backend.

Run (from this folder): `npm install`, then `npm run dev`, then open http://localhost:5173.
The API must be running on 127.0.0.1:8000 (see ../backend).

The dashboard shows device status and staleness, speed (pulses/s; RPM is assumed, unverified),
vibration only when vib_valid = 1, and current as an uncalibrated mV change.
Windows flagged `settling` are left out of the charts.
