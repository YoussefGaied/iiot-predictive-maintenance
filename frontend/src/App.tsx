import { useEffect, useMemo, useState } from "react";
import type { ReactNode } from "react";
import {
  CartesianGrid,
  Line,
  LineChart,
  ResponsiveContainer,
  Tooltip,
  XAxis,
  YAxis,
} from "recharts";
import { fetchAnomalies, fetchLatest, fetchStatus } from "./api";
import { usePolling } from "./usePolling";
import { useTelemetryHistory } from "./useTelemetryHistory";

const STALE_AFTER_S = 5; // telemetry is sent every 500 ms

function useNow(intervalMs: number): number {
  const [now, setNow] = useState(() => Date.now());
  useEffect(() => {
    const id = setInterval(() => setNow(Date.now()), intervalMs);
    return () => clearInterval(id);
  }, [intervalMs]);
  return now;
}

function fmtAge(s: number): string {
  if (s < 60) return `${Math.round(s)} s`;
  if (s < 3600) return `${Math.floor(s / 60)} min ${Math.round(s % 60)} s`;
  if (s < 86400) return `${(s / 3600).toFixed(1)} h`;
  return `${(s / 86400).toFixed(1)} days`;
}

const card: React.CSSProperties = {
  background: "#1a1d24",
  border: "1px solid #2c313c",
  borderRadius: 8,
  padding: 14,
  minWidth: 210,
  flex: "1 1 210px",
  textAlign: "left",
};
const big: React.CSSProperties = { fontSize: 28, fontWeight: 700, margin: "4px 0" };
const small: React.CSSProperties = { fontSize: 12, color: "#9aa3b2" };

function Card({ title, children }: { title: string; children: ReactNode }) {
  return (
    <div style={card}>
      <div style={small}>{title}</div>
      {children}
    </div>
  );
}

interface ChartRow {
  label: string;
  speed: number | null;
  vib: number | null;
  current: number | null;
}

function Chart({
  title,
  note,
  data,
  dataKey,
  color,
}: {
  title: string;
  note: string;
  data: ChartRow[];
  dataKey: "speed" | "vib" | "current";
  color: string;
}) {
  return (
    <div style={{ ...card, flex: "1 1 100%" }}>
      <div style={{ fontWeight: 600 }}>{title}</div>
      <div style={{ ...small, marginBottom: 8 }}>{note}</div>
      <div style={{ width: "100%", height: 220 }}>
        <ResponsiveContainer>
          <LineChart data={data}>
            <CartesianGrid stroke="#2c313c" />
            <XAxis dataKey="label" minTickGap={60} stroke="#9aa3b2" />
            <YAxis stroke="#9aa3b2" />
            <Tooltip
              contentStyle={{ background: "#1a1d24", border: "1px solid #2c313c" }}
            />
            <Line
              type="monotone"
              dataKey={dataKey}
              stroke={color}
              dot={false}
              connectNulls={false}
              isAnimationActive={false}
            />
          </LineChart>
        </ResponsiveContainer>
      </div>
    </div>
  );
}

export default function App() {
  const now = useNow(1000);
  const status = usePolling(fetchStatus, 2000);
  const latest = usePolling(fetchLatest, 2000);
  const anomalies = usePolling(fetchAnomalies, 2000);
  const history = useTelemetryHistory(2000);

  const row = latest.data?.data ?? null;
  const notes = latest.data?.notes ?? [];
  const apiOk = status.error === null && status.lastOkAt !== null;

  const ageS = row ? (now - Date.parse(row.received_at)) / 1000 : null;
  const stale = ageS === null || ageS > STALE_AFTER_S;
  const deviceStatus = status.data?.device_status ?? "unknown";
  const live = apiOk && deviceStatus === "online" && !stale;

  const chartData: ChartRow[] = useMemo(
    () =>
      history.rows.map((r) => {
        const usable = r.settling === 0; // exclude acceleration windows
        return {
          label: new Date(r.received_at).toLocaleTimeString(),
          speed: usable ? r.pulses_per_s : null,
          vib: usable && r.vib_valid === 1 ? r.vib_rms_g : null,
          current:
            usable && r.current_dmv !== null && r.current_dmv !== undefined
              ? r.current_dmv
              : null,
        };
      }),
    [history.rows],
  );

  return (
    <div
      style={{
        padding: 24,
        maxWidth: 1200,
        margin: "0 auto",
        fontFamily: "system-ui, sans-serif",
      }}
    >
      <h1 style={{ marginBottom: 4 }}>Motor condition monitor (rig1)</h1>
      <div style={{ ...small, marginBottom: 16 }}>
        Small 6 V demo rig. Condition monitoring only: no failure prediction.
      </div>

      <div style={{ display: "flex", gap: 12, flexWrap: "wrap", marginBottom: 12 }}>
        <Card title="API">
          <div style={{ ...big, color: apiOk ? "limegreen" : "tomato" }}>
            {apiOk ? "connected" : status.error ? "unreachable" : "connecting..."}
          </div>
          {status.error && <div style={small}>{status.error}</div>}
        </Card>

        <Card title="Device">
          <div
            style={{
              ...big,
              color: live ? "limegreen" : deviceStatus === "online" ? "orange" : "tomato",
            }}
          >
            {deviceStatus}
          </div>
          <div style={small}>
            Last data: {ageS === null ? "none" : `${fmtAge(Math.max(ageS, 0))} ago`}
            {stale && row ? " (STALE)" : ""}
          </div>
          <div style={small}>Wi-Fi RSSI: {row ? `${row.rssi} dBm` : "-"}</div>
        </Card>

        <Card title="Condition rules (threshold, last ~60 s)">
          {anomalies.data === null ? (
            <div style={{ ...big, color: "orange" }}>{anomalies.error ? "unavailable" : "..."}</div>
          ) : anomalies.data.alert_count === 0 ? (
            <div style={{ ...big, color: "limegreen" }}>no active alerts</div>
          ) : (
            <>
              <div style={{ ...big, color: "orange" }}>
                {anomalies.data.alert_count} alert{anomalies.data.alert_count > 1 ? "s" : ""}
              </div>
              {anomalies.data.alerts.map((al) => (
                <div key={al.rule} style={small}>
                  <b>{al.rule}</b>: {al.message}
                </div>
              ))}
            </>
          )}
          <div style={small}>
            Rules from measured baselines, not failure prediction. No current, temperature or
            running-vibration rule.
          </div>
        </Card>

        <Card title="Motor (last stored row)">
          <div style={big}>{row ? row.motor_dir : "-"}</div>
          <div style={small}>
            duty {row ? row.duty : "-"} | settling window: {row ? (row.settling ? "yes" : "no") : "-"}
          </div>
        </Card>
      </div>

      <div style={{ display: "flex", gap: 12, flexWrap: "wrap", marginBottom: 12 }}>
        <Card title="Speed (measured)">
          <div style={big}>{row ? `${row.pulses_per_s} pulses/s` : "-"}</div>
          <div style={small}>
            RPM {row ? row.rpm : "-"}: assumes {row ? row.ppr_assumed : "?"} pulses/rev,
            UNVERIFIED
          </div>
        </Card>

        <Card title="Vibration RMS">
          {row && row.vib_valid === 1 ? (
            <>
              <div style={big}>{row.vib_rms_g.toFixed(3)} g</div>
              <div style={small}>valid window (n={row.vib_n})</div>
            </>
          ) : (
            <>
              <div style={{ ...big, color: "orange" }}>not valid</div>
              <div style={small}>
                {row
                  ? `n=${row.vib_n}, err=${row.vib_err}, amean=${row.amean_g} g, settling=${row.settling}`
                  : "no data"}
              </div>
            </>
          )}
        </Card>

        <Card title="Current (uncalibrated)">
          <div style={big}>
            {row && row.current_dmv !== null ? `${row.current_dmv} mV` : "not zeroed"}
          </div>
          <div style={small}>
            Raw field current_dmv: mV change from the rest level set with &quot;z&quot;
            (negative while running). Not amps. Noisy with Wi-Fi on.
          </div>
        </Card>
      </div>

      <div style={{ display: "flex", gap: 12, flexWrap: "wrap" }}>
        <Chart
          title="Speed (pulses/s)"
          note="Settling windows are left out (gaps). Rows from different sessions are shown one after another."
          data={chartData}
          dataKey="speed"
          color="#4cc9f0"
        />
        <Chart
          title="Vibration RMS (g)"
          note="Only windows with vib_valid = 1. Gaps mean invalid or settling. No valid data exists while the motor runs."
          data={chartData}
          dataKey="vib"
          color="#f72585"
        />
        <Chart
          title="Current (mV change from rest, uncalibrated)"
          note="Only after a 'z' zero. Gaps mean not zeroed or settling. Not usable as amps."
          data={chartData}
          dataKey="current"
          color="#ffd166"
        />
      </div>

      {history.error && (
        <p style={{ color: "tomato" }}>History error: {history.error}</p>
      )}

      <details style={{ marginTop: 16 }}>
        <summary>Condition rule definitions</summary>
        <ul>
          {Object.entries(anomalies.data?.rules ?? {}).map(([k, v]) => (
            <li key={k}>
              <b>{k}</b>: {v}
            </li>
          ))}
        </ul>
      </details>

      <details style={{ marginTop: 16 }}>
        <summary>Data caveats from the API</summary>
        <ul>
          {notes.map((n, i) => (
            <li key={i}>{n}</li>
          ))}
        </ul>
      </details>
    </div>
  );
}