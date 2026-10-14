import sys
import csv
import time
import statistics

# Columns saved to CSV. Keys not listed here are ignored when logging.
FIELDS = ["dir", "duty", "mean_mV", "min", "max", "std_mV",
          "delta_mV", "approx_A", "offset_mV"]


def parse_line(line):
    values = {}
    for part in line.split():
        if "=" in part:
            key, text = part.split("=", 1)
            try:
                values[key] = float(text)
            except ValueError:
                pass
    return values


def load(path):
    """Return a list of dicts. Each has 't' (seconds for CSV, line number for txt)."""
    rows = []
    if path.endswith(".csv"):
        with open(path, newline="") as f:
            for record in csv.DictReader(f):
                row = {}
                for key, text in record.items():
                    if text not in ("", None):
                        try:
                            row[key] = float(text)
                        except ValueError:
                            pass
                rows.append(row)
    else:
        with open(path, errors="replace") as f:
            for line in f:
                row = parse_line(line)
                if row:
                    row["t"] = float(len(rows))
                    rows.append(row)
    return rows


def summarize(rows, label, unit):
    print(f"\n== {label}: {len(rows)} lines ==")
    skip = {"t", "dir", "duty"}
    keys = sorted({k for r in rows for k in r} - skip)
    for key in keys:
        points = [(r["t"], r[key]) for r in rows if key in r and "t" in r]
        vals = [v for _, v in points]
        if len(vals) < 2:
            print(f"{key:10} n={len(vals)} (need at least 2 values)")
            continue
        text = (f"{key:10} n={len(vals):4d} mean={statistics.mean(vals):9.2f} "
                f"std={statistics.stdev(vals):8.2f} "
                f"min={min(vals):9.2f} max={max(vals):9.2f}")
        times = [t for t, _ in points]
        if len(set(times)) > 1:
            slope = statistics.linear_regression(times, vals).slope
            text += f" drift={slope:.4f} per {unit}"
        print(text)


def analyze(path):
    rows = load(path)
    unit = "s" if path.endswith(".csv") else "line"
    groups = {}
    for r in rows:
        if "dir" in r and "duty" in r:
            key = f"dir={int(r['dir'])} duty={int(r['duty'])}"
        else:
            key = "no dir/duty"
        groups.setdefault(key, []).append(r)
    for label, group_rows in groups.items():
        summarize(group_rows, label, unit)


def log(port, out, baud=115200):
    import serial  # pyserial
    ser = serial.Serial(port, baud, timeout=1)
    print(f"Logging {port} to {out}. Ctrl+C to stop.")
    t0 = time.time()
    with open(out, "w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=["t"] + FIELDS,
                                extrasaction="ignore")
        writer.writeheader()
        try:
            while True:
                raw = ser.readline()
                if not raw:
                    continue
                text = raw.decode("utf-8", errors="replace").strip()
                print(text)
                row = parse_line(text)
                if row:
                    row["t"] = round(time.time() - t0, 3)
                    writer.writerow(row)
                    f.flush()
        except KeyboardInterrupt:
            print("\nStopped.")
    ser.close()


if len(sys.argv) >= 3 and sys.argv[1] == "analyze":
    analyze(sys.argv[2])
elif len(sys.argv) >= 4 and sys.argv[1] == "log":
    log(sys.argv[2], sys.argv[3])
else:
    print("Usage:")
    print("  python3 logtool.py analyze FILE.txt|FILE.csv")
    print("  python3 logtool.py log /dev/ttyUSB0 out.csv")
