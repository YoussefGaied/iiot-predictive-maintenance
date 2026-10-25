#!/usr/bin/env python3
"""replay_rules.py: run rules.py over every stored telemetry row (read-only) and list the alert episodes.

Run from backend/:   python3 replay_rules.py [path/to/iiot.db]
This shows WHERE the rules fire on the data they were tuned on. It is not a measure of detection quality.
"""
import os
import sqlite3
import sys
from datetime import datetime

from rules import Config, RuleEngine

DB = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(os.path.abspath(__file__)), "data", "iiot.db")
db = sqlite3.connect(f"file:{DB}?mode=ro", uri=True)
db.row_factory = sqlite3.Row
rows = [dict(r) for r in db.execute("SELECT * FROM telemetry ORDER BY id")]
cfg = Config()
engine, active, episodes, prev_t = RuleEngine(cfg), {}, [], None

for r in rows:
    t = datetime.fromisoformat(r["received_at"])
    if prev_t is not None and (t - prev_t).total_seconds() > cfg.stale_after_s:   # data gap = DATA_STALE episode
        episodes.append(["DATA_STALE", r["id"], r["id"], r["received_at"], f"gap of {(t - prev_t).total_seconds():.0f} s before this row"])
    prev_t = t
    now = {a.rule: a for a in engine.update(r)}
    for rule, a in now.items():
        if rule not in active:
            active[rule] = [rule, r["id"], r["id"], r["received_at"], a.message]
            episodes.append(active[rule])
        active[rule][2] = r["id"]
    for rule in [k for k in active if k not in now]:
        del active[rule]

print(f"{len(rows)} rows, ids {rows[0]['id']}-{rows[-1]['id']}, {rows[0]['received_at'][:10]} .. {rows[-1]['received_at'][:10]}\n")
print(f"{'rule':26} {'ids':>11}  {'start (UTC)':23} detail")
for rule, a, b, ts, msg in sorted(episodes, key=lambda e: e[1]):
    print(f"{rule:26} {a:>5}-{b:<5}  {ts[:23]:23} {msg}")
print("\nepisodes per rule and day:")
counts = {}
for rule, a, b, ts, msg in episodes:
    counts[(rule, ts[:10])] = counts.get((rule, ts[:10]), 0) + 1
for (rule, day), n in sorted(counts.items()):
    print(f"  {rule:26} {day}  {n}")
