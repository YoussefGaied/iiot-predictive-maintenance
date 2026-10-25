# Dashboard (Phases 9-10)

React + TypeScript + Vite + Recharts. Reads the read-only FastAPI backend.

Run (from this folder): `npm install`, then `npm run dev`, then open http://localhost:5173.
The API must be running on 127.0.0.1:8000 (see ../backend). Other checks: `npx tsc -b`, `npx eslint src`, `npm run build`.

The dashboard shows API connection, device status and staleness, the condition-rule alerts from `/api/anomalies`
(Phase 10), the last motor state, speed (pulses/s; RPM is assumed, unverified), vibration only when vib_valid = 1, and
current as an uncalibrated mV change. Windows flagged `settling` are left out of the charts, and only the newest 600
rows are kept.
