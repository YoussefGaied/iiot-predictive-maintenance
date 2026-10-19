#!/usr/bin/env python3
"""api.py v0.1: read-only FastAPI over the SQLite database.
Run:  uvicorn api:app --host 127.0.0.1 --port 8000   (from backend/). Docs: http://127.0.0.1:8000/docs"""
import os
import sqlite3

from fastapi import FastAPI, HTTPException, Query
from fastapi.middleware.cors import CORSMiddleware

DB_PATH = os.environ.get("DB_PATH", os.path.join(os.path.dirname(os.path.abspath(__file__)), "data", "iiot.db"))
NOTES = [
    "rpm assumes ppr_assumed pulses per revolution; this was never verified.",
    "current_dmv is a mV change from the rest level set with 'z', uncalibrated, not amps; it was noisy with Wi-Fi on.",
    "Use vib_rms_g only when vib_valid is 1; while the motor runs it is normally 0 (I2C errors).",
    "No temperature: the DS18B20 is broken.",
]

app = FastAPI(title="IIoT predictive maintenance demo rig", version="0.1")
app.add_middleware(CORSMiddleware, allow_origins=["http://localhost:5173", "http://127.0.0.1:5173"],
                   allow_methods=["GET"], allow_headers=["*"])


def query(sql, args=()):
    if not os.path.exists(DB_PATH):
        raise HTTPException(503, "database not found")
    db = sqlite3.connect(f"file:{DB_PATH}?mode=ro", uri=True)
    db.row_factory = sqlite3.Row
    try:
        return [dict(r) for r in db.execute(sql, args)]
    finally:
        db.close()


@app.get("/api/health")
def health():
    return {"ok": True}


@app.get("/api/status")
def status():
    rows = query("SELECT received_at, payload FROM messages WHERE topic='iiot/rig1/status' ORDER BY id DESC LIMIT 1")
    if not rows:
        return {"device_status": None, "as_of": None}
    return {"device_status": rows[0]["payload"], "as_of": rows[0]["received_at"]}


@app.get("/api/latest")
def latest():
    rows = query("SELECT * FROM telemetry ORDER BY id DESC LIMIT 1")
    return {"data": rows[0] if rows else None, "notes": NOTES}


@app.get("/api/telemetry")
def telemetry(limit: int = Query(200, ge=1, le=5000), after_id: int = Query(0, ge=0)):
    """Newest `limit` rows with id > after_id, returned oldest first (ready to plot)."""
    rows = query("SELECT * FROM (SELECT * FROM telemetry WHERE id > ? ORDER BY id DESC LIMIT ?) ORDER BY id",
                 (after_id, limit))
    return {"count": len(rows), "data": rows, "notes": NOTES}
