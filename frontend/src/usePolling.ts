import { useEffect, useState } from "react";

export interface PollState<T> {
  data: T | null;
  error: string | null;
  lastOkAt: number | null; // browser time (ms) of last successful fetch
}

export function usePolling<T>(fn: () => Promise<T>, intervalMs: number): PollState<T> {
  const [state, setState] = useState<PollState<T>>({
    data: null,
    error: null,
    lastOkAt: null,
  });

  useEffect(() => {
    let cancelled = false;
    const tick = async () => {
      try {
        const data = await fn();
        if (!cancelled)
          setState({ data, error: null, lastOkAt: Date.now() });
      } catch (e: unknown) {
        if (!cancelled)
          setState((s) => ({ ...s, error: String(e) }));
      }
    };
    tick();
    const id = setInterval(tick, intervalMs);
    return () => {
      cancelled = true;
      clearInterval(id);
    };
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [intervalMs]);

  return state;
}