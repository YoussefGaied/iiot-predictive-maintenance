export const API = "http://127.0.0.1:8000";

export interface Telemetry {
  id: number;
  received_at: string;
  device_id: string;
  fw: string;
  seq: number;
  uptime_ms: number;
  run: number;
  motor_dir: "fwd" | "rev" | "stop";
  duty: number;
  settling: number;
  pulses_per_s: number;
  rpm: number;
  ppr_assumed: number;
  current_dmv: number | null;
  vib_rms_g: number;
  vib_n: number;
  vib_err: number;
  amean_g: number;
  vib_valid: number;
  rssi: number;
}

export interface StatusResponse {
  device_status: string;
  as_of: string;
}

export interface LatestResponse {
  data: Telemetry | null;
  notes: string[];
}

export interface TelemetryResponse {
  count: number;
  data: Telemetry[];
  notes: string[];
}

async function getJson<T>(path: string): Promise<T> {
  const res = await fetch(`${API}${path}`);
  if (!res.ok) throw new Error(`${path}: HTTP ${res.status}`);
  return (await res.json()) as T;
}

export const fetchStatus = () => getJson<StatusResponse>("/api/status");
export const fetchLatest = () => getJson<LatestResponse>("/api/latest");
export const fetchTelemetry = (limit = 200, afterId = 0) =>
  getJson<TelemetryResponse>(`/api/telemetry?limit=${limit}&after_id=${afterId}`);