import unittest
from datetime import datetime, timedelta, timezone
from rules import RuleEngine, check_staleness


def row(seq, d="stop", pps=0.0, vib=0.005, valid=1, settling=False, duty=195):
    return dict(seq=seq, motor_dir=d, duty=duty, settling=settling, pulses_per_s=pps, vib_rms_g=vib, vib_valid=valid)


def run(rows):
    e, out = RuleEngine(), []
    for r in rows:
        out.append({a.rule for a in e.update(r)})
    return out


class Rules(unittest.TestCase):
    def test_quiet_rest_no_alerts(self):
        self.assertTrue(all(not a for a in run([row(i) for i in range(30)])))

    def test_healthy_run_no_alerts(self):
        rows = [row(0, "fwd", 14, settling=True)] + [row(i, "fwd", 200) for i in range(1, 6)] + \
               [row(6, "stop", 195), row(7, "stop", 40)] + [row(i) for i in range(8, 20)]
        self.assertTrue(all(not a for a in run(rows)))

    def test_no_speed_when_commanded(self):
        rows = [row(0, "fwd", 0, settling=True)] + [row(i, "fwd", 0) for i in range(1, 6)]
        out = run(rows)
        self.assertFalse(out[0] or out[1])                     # settling + 1st window: not yet
        self.assertIn("NO_SPEED_WHILE_COMMANDED", out[2])      # 2nd non-settling window
        self.assertIn("NO_SPEED_WHILE_COMMANDED", out[5])

    def test_other_duty_not_evaluated(self):
        self.assertTrue(all(not a for a in run([row(i, "fwd", 0, duty=145) for i in range(6)])))

    def test_speed_while_stopped_after_grace(self):
        rows = [row(i, "stop", 100) for i in range(8)]
        out = run(rows)
        self.assertTrue(all(not a for a in out[:4]))           # grace 3 windows + 1 to confirm
        self.assertIn("SPEED_WHILE_STOPPED", out[-1])

    def test_vibration_needs_three_valid_windows(self):
        out = run([row(i) for i in range(5)] + [row(5, vib=0.5), row(6, vib=0.5), row(7, vib=0.005)])
        self.assertTrue(all("VIBRATION_AT_REST" not in a for a in out))      # 2 spikes only
        out = run([row(i) for i in range(5)] + [row(5 + i, vib=0.4) for i in range(4)])
        self.assertIn("VIBRATION_AT_REST", out[-2])

    def test_invalid_windows_do_not_count_or_reset(self):
        rows = [row(i) for i in range(5)] + [row(5, vib=0.4), row(6, vib=0.0, valid=0), row(7, vib=0.4), row(8, vib=0.4)]
        self.assertIn("VIBRATION_AT_REST", run(rows)[-1])

    def test_reboot_resets_state(self):
        rows = [row(i, vib=0.4) for i in range(10)] + [row(0, vib=0.4), row(1, vib=0.4)]
        out = run(rows)
        self.assertIn("VIBRATION_AT_REST", out[9])
        self.assertFalse(out[-1])

    def test_staleness(self):
        now = datetime(2026, 10, 25, 12, 0, 10, tzinfo=timezone.utc)
        iso = lambda s: (now - timedelta(seconds=s)).isoformat()
        self.assertIsNone(check_staleness(iso(1), now, "online"))
        self.assertEqual(check_staleness(iso(8), now, "online").rule, "DATA_STALE")
        self.assertEqual(check_staleness(iso(1), now, "offline").rule, "DATA_STALE")
        self.assertEqual(check_staleness(None, now).rule, "DATA_STALE")


if __name__ == "__main__":
    unittest.main()
