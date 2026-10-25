# Serial logger and analysis tool

A small Python tool (`logtool.py`, standard library plus pyserial) for the
Industrial IoT Predictive Maintenance rig. It was built in Phase 5 for raw
serial captures. Since Phase 8, live telemetry no longer goes through it: the
ESP32 publishes over MQTT, `backend/consumer.py` stores it in SQLite, and the
dashboard reads the API. Use this tool for serial-only experiments (sketches
that print `key=value` lines) and for analysing saved serial output. It does two jobs:

1. **`log`**: read lines from the ESP32 serial port and save each parsed
   line to a CSV file with a host timestamp.
2. **`analyze`**: compute mean, standard deviation, min, max and drift
   (slope over time) for each field, per `dir`/`duty` group when present.

## Usage

```
python3 logtool.py log /dev/ttyUSB0 out.csv
python3 logtool.py analyze out.csv
python3 logtool.py analyze saved_serial_output.txt
```

- Close the Arduino Serial Monitor first. It holds the port.
- Opening the port resets the ESP32, so boot lines appear first.
- The port is usually `/dev/ttyUSB0` or `/dev/ttyACM0`. Baud rate is 115200.
- `Permission denied` on the port usually means the user is not in the
  `dialout` group.
- Stop logging with Ctrl+C. Each row is flushed to disk as it arrives.

## Input format

The firmware prints `key=value` pairs separated by spaces, for example:

```
mean_mV=2585.6 min=2547 max=2643 std_mV=17.4
dir=0 duty=145 mean_mV=... delta_mV=... approx_A=...
```

Anything that is not a numeric `key=value` pair (banners, boot noise,
garbage characters) is ignored without crashing.

CSV columns: `t, dir, duty, mean_mV, min, max, std_mV, delta_mV, approx_A,
offset_mV`. Keys not in this list are not saved when logging. Edit
`FIELDS` in `logtool.py` to add more.

## Output

For each group (`dir=.. duty=..`, or `no dir/duty`) and each field:
`n`, `mean`, `std` (sample standard deviation), `min`, `max`, and `drift`.
Drift is the least-squares slope: per second for CSV files (real host
timestamps), per line for `.txt` files (no timestamps).

## What was validated

- **`parse_line`, grouping, statistics and drift:** tested on synthetic
  data with known answers (`tests/make_fake.py`). A planted drift of
  -0.1 mV per line was recovered as -0.1008, and a flat group gave 0.0003.
  Group means and standard deviations matched hand calculations.
- **`log`:** tested against a pseudo-terminal (`tests/fake_esp32.py`)
  emitting a ramp of -0.1 mV every 0.5 s. The CSV analysis recovered
  -0.1999 per s.
- **Not yet tested:** logging from a real ESP32 on a real serial port, and
  garbage boot characters from real hardware.

## Limitations

- Synthetic tests prove the tool computes correctly. They say nothing
  about the sensors.
- Current values in the Phase 5 data are **uncalibrated** (datasheet
  sensitivity only, no reference ammeter).
- Drift is a straight-line fit. It cannot describe non-linear behaviour,
  and a short log gives an unreliable slope.
- No plotting and no real-time display. Anomaly rules live in `backend/rules.py`, not here.

## Tests

```
cd tools/tests
python3 make_fake.py                      # writes fake_test.txt
python3 ../logtool.py analyze fake_test.txt

python3 fake_esp32.py                     # prints a fake port, e.g. /dev/pts/N
python3 ../logtool.py log /dev/pts/N fake_log.csv    # use the printed path
```

Do not commit generated `fake_*` files.
