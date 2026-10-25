#!/usr/bin/env python3
"""rules.py v0.1: transparent threshold rules for the demo rig (no machine learning).

Every number below was read off the stored data (see docs/phases/phase10_anomaly_rules.md).
These are *condition flags*, not failure predictions: there is no labelled failure data.

Four rules:
  DATA_STALE               no fresh telemetry, or the device reported "offline"
  NO_SPEED_WHILE_COMMANDED motor commanded fwd/rev, not settling, but (almost) no speed pulses
  SPEED_WHILE_STOPPED      motor commanded stop for a while, but speed pulses keep coming
  VIBRATION_AT_REST        motor stopped, valid IMU windows, vibration above the resting baseline

Use:  engine = RuleEngine();  alerts = engine.update(row)   # rows oldest first, one call per 500 ms window
"""
from dataclasses import dataclass, field
from datetime import datetime, timezone


@dataclass(frozen=True)
class Config:
    window_s: float = 0.5               # firmware reports every 500 ms
    stale_after_s: float = 5.0          # 10 missed windows. Observed reboot gaps: 4.7-10.9 s.
    baseline_duties: tuple = (195,)     # speed/pulse limits were only measured at duty 195
    run_min_pps: float = 50.0           # running windows (not settling) were >= 154 pulses/s at duty 195
    run_need: int = 2                   # consecutive windows -> 1 s
    stop_grace_windows: int = 3         # pulses were seen only in the first 2 stop windows (coast-down)
    stop_max_pps: float = 20.0          # stopped and settled: 0 pulses seen in 300+ windows
    stop_need: int = 2
    vib_max_g: float = 0.02             # quiet-session max 0.0118 g, median 0.005 g
    vib_need: int = 3                   # consecutive valid windows (1.5 s); single spikes up to 0.5 g occurred


DESCRIPTIONS = {
    "DATA_STALE": "No new telemetry for more than 5 s, or the device status is 'offline'.",
    "NO_SPEED_WHILE_COMMANDED": "Motor commanded to run (duty 195), settling window excluded, but speed < 50 pulses/s "
                                "for 2 windows. Cause not determined (no power, no motor, blocked sensor, ...).",
    "SPEED_WHILE_STOPPED": "Motor commanded to stop for > 3 windows, but speed > 20 pulses/s for 2 windows.",
    "VIBRATION_AT_REST": "Motor stopped (after coast-down), IMU window valid, vibration RMS > 0.02 g for 3 valid "
                         "windows. Cause not determined (bump, mount, motor, ...). Not valid while the motor runs.",
}


@dataclass
class Alert:
    rule: str
    message: str
    severity: str = "warning"

    def as_dict(self):
        return {"rule": self.rule, "severity": self.severity, "message": self.message}


@dataclass
class _Counter:
    need: int
    n: int = 0

    def feed(self, cond):
        """cond True -> count up, False -> reset, None -> window not usable, keep the count."""
        if cond is True:
            self.n += 1
        elif cond is False:
            self.n = 0
        return self.n >= self.need


@dataclass
class RuleEngine:
    cfg: Config = field(default_factory=Config)

    def __post_init__(self):
        c = self.cfg
        self._prev_seq = None
        self._stop_idx = 0              # stop windows seen since the last non-stop window (or since boot)
        self._run = _Counter(c.run_need)
        self._stp = _Counter(c.stop_need)
        self._vib = _Counter(c.vib_need)

    def update(self, row):
        """Feed one telemetry row (dict). Returns the list of ACTIVE motor/vibration alerts after this row."""
        c = self.cfg
        seq = row.get("seq")
        if seq is not None and self._prev_seq is not None and seq < self._prev_seq:  # firmware rebooted
            self._stop_idx = 0
            self._run.n = self._stp.n = self._vib.n = 0
        self._prev_seq = seq

        d = row.get("motor_dir")
        pps = row.get("pulses_per_s")
        settling = bool(row.get("settling"))
        stopped = d == "stop"
        self._stop_idx = self._stop_idx + 1 if stopped else 0
        eligible_stop = stopped and (self._stop_idx - 1) >= c.stop_grace_windows  # 0-based index >= grace

        # NO_SPEED_WHILE_COMMANDED
        if d in ("fwd", "rev") and row.get("duty") in c.baseline_duties and not settling and pps is not None:
            run_cond = pps < c.run_min_pps
        elif d in ("fwd", "rev") and (settling or row.get("duty") not in c.baseline_duties):
            run_cond = None             # not evaluated: settling window, or a duty we have no data for
        else:
            run_cond = False
        # SPEED_WHILE_STOPPED
        stp_cond = (pps > c.stop_max_pps) if (eligible_stop and pps is not None) else False
        # VIBRATION_AT_REST
        if not eligible_stop:
            vib_cond = False
        elif row.get("vib_valid") != 1 or row.get("vib_rms_g") is None:
            vib_cond = None
        else:
            vib_cond = row["vib_rms_g"] > c.vib_max_g

        alerts = []
        if self._run.feed(run_cond):
            alerts.append(Alert("NO_SPEED_WHILE_COMMANDED",
                                f"commanded {d} at duty {row.get('duty')} but only {pps:.1f} pulses/s"))
        if self._stp.feed(stp_cond):
            alerts.append(Alert("SPEED_WHILE_STOPPED", f"commanded stop but {pps:.1f} pulses/s"))
        if self._vib.feed(vib_cond):
            alerts.append(Alert("VIBRATION_AT_REST", f"vibration {row['vib_rms_g']:.4f} g while stopped"))
        return alerts


def _parse(ts):
    return datetime.fromisoformat(ts)


def check_staleness(last_received_at, now=None, device_status=None, cfg=Config()):
    """DATA_STALE alert or None. `last_received_at`: ISO UTC string of the newest telemetry row (or None)."""
    now = now or datetime.now(timezone.utc)
    if last_received_at is None:
        return Alert("DATA_STALE", "no telemetry stored yet")
    age = (now - _parse(last_received_at)).total_seconds()
    if device_status == "offline":
        return Alert("DATA_STALE", f"device status is offline (last telemetry {age:.0f} s ago)")
    if age > cfg.stale_after_s:
        return Alert("DATA_STALE", f"no telemetry for {age:.0f} s")
    return None
