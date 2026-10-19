#!/usr/bin/env python3
"""consumer.py v0.2: MQTT -> SQLite.
Every message under iiot/# is stored raw in `messages`; telemetry JSON is also stored typed in `telemetry`."""
import json
import os
import sqlite3
import sys
from datetime import datetime, timezone

import paho.mqtt.client as mqtt

HOST = os.environ.get("MQTT_HOST", "localhost")
PORT = int(os.environ.get("MQTT_PORT", "1883"))
USER = os.environ.get("MQTT_USER", "rig1")
PASS = os.environ.get("MQTT_PASS")
DB_PATH = os.environ.get("DB_PATH", os.path.join(os.path.dirname(os.path.abspath(__file__)), "data", "iiot.db"))
TOPIC = "iiot/#"
TELEMETRY_TOPIC = "iiot/rig1/telemetry"

SCHEMA = """
CREATE TABLE IF NOT EXISTS messages (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    received_at TEXT NOT NULL, topic TEXT NOT NULL, payload TEXT NOT NULL,
    device_id TEXT, seq INTEGER, retained INTEGER NOT NULL);
CREATE INDEX IF NOT EXISTS idx_messages_topic_time ON messages(topic, received_at);
CREATE TABLE IF NOT EXISTS telemetry (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    received_at TEXT NOT NULL,            -- host UTC time (the ESP32 has no clock)
    device_id TEXT, fw TEXT, seq INTEGER, uptime_ms INTEGER, run INTEGER,
    motor_dir TEXT, duty INTEGER, settling INTEGER,
    pulses_per_s REAL, rpm REAL, ppr_assumed INTEGER,
    current_dmv REAL,                     -- NULL until 'z'; mV change, UNCALIBRATED
    vib_rms_g REAL, vib_n INTEGER, vib_err INTEGER, amean_g REAL, vib_valid INTEGER,
    rssi INTEGER);
CREATE INDEX IF NOT EXISTS idx_telemetry_time ON telemetry(received_at);
"""
COLS = ["device_id", "fw", "seq", "uptime_ms", "run", "motor_dir", "duty", "settling",
        "pulses_per_s", "rpm", "ppr_assumed", "current_dmv",
        "vib_rms_g", "vib_n", "vib_err", "amean_g", "vib_valid", "rssi"]
BOOLS = {"settling", "vib_valid"}


def telemetry_row(obj):
    """Typed values in COLS order; missing keys -> None; booleans -> 0/1."""
    row = []
    for c in COLS:
        v = obj.get(c)
        if c in BOOLS and v is not None:
            v = int(bool(v))
        row.append(v)
    return row


def store(db, topic, text, retained, now=None):
    now = now or datetime.now(timezone.utc).isoformat(timespec="milliseconds")
    device_id, seq, obj = None, None, None
    try:
        obj = json.loads(text)
        if isinstance(obj, dict):
            device_id, seq = obj.get("device_id"), obj.get("seq")
        else:
            obj = None
    except json.JSONDecodeError:
        pass  # e.g. "online"/"offline": stored raw
    db.execute("INSERT INTO messages (received_at, topic, payload, device_id, seq, retained) VALUES (?,?,?,?,?,?)",
               (now, topic, text, device_id, seq, int(retained)))
    if topic == TELEMETRY_TOPIC and obj is not None:
        db.execute(f"INSERT INTO telemetry (received_at, {','.join(COLS)}) VALUES (?,{','.join('?' * len(COLS))})",
                   [now] + telemetry_row(obj))
    db.commit()
    return now


def main():
    if not PASS:
        sys.exit("Set MQTT_PASS first:  export MQTT_PASS='your_password'")
    db = sqlite3.connect(DB_PATH)
    db.executescript(SCHEMA)

    def on_connect(client, userdata, flags, reason_code, properties):
        if reason_code.is_failure:
            print(f"connect failed: {reason_code}")
            return
        print(f"connected to {HOST}:{PORT}, subscribing to {TOPIC}")
        client.subscribe(TOPIC)

    def on_message(client, userdata, msg):
        text = msg.payload.decode("utf-8", errors="replace")
        now = store(db, msg.topic, text, msg.retain)
        print(f"{now} {msg.topic} {text[:80]}")

    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id="consumer1")
    client.username_pw_set(USER, PASS)
    client.on_connect, client.on_message = on_connect, on_message
    client.reconnect_delay_set(min_delay=1, max_delay=30)
    client.connect(HOST, PORT, keepalive=30)
    try:
        client.loop_forever()
    except KeyboardInterrupt:
        print("\nstopping")
    finally:
        client.disconnect()
        db.close()


if __name__ == "__main__":
    main()
