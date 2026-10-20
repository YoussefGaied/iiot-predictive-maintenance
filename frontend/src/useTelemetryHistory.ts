import { useEffect, useRef, useState } from "react";
import { fetchTelemetry } from "./api";
import type { Telemetry } from "./api";

const LIMIT = 600; // the API returns the newest `limit` rows with id > after_id
const MAX_ROWS = 600;

export function useTelemetryHistory(intervalMs: number) {
  const [rows, setRows] = useState<Telemetry[]>([]);
  const [error, setError] = useState<string | null>(null);
  const lastId = useRef(0);

  useEffect(() => {
    let cancelled = false;

    const tick = async () => {
      try {
        const res = await fetchTelemetry(LIMIT, lastId.current);
        if (cancelled) return;
        setError(null);
        if (res.data.length > 0) {
          lastId.current = res.data[res.data.length - 1].id;
          setRows((prev) => [...prev, ...res.data].slice(-MAX_ROWS));
        }
      } catch (e: unknown) {
        if (!cancelled) setError(String(e));
      }
    };

    tick();
    const id = setInterval(tick, intervalMs);
    return () => {
      cancelled = true;
      clearInterval(id);
    };
  }, [intervalMs]);

  return { rows, error };
}