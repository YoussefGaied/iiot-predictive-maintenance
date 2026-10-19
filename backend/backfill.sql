-- One-off: copy telemetry rows already stored in `messages` into the typed table.
-- Run ONCE, after consumer v0.2 created the table and BEFORE you start it again.
INSERT INTO telemetry (received_at, device_id, fw, seq, uptime_ms, run, motor_dir, duty, settling,
    pulses_per_s, rpm, ppr_assumed, current_dmv, vib_rms_g, vib_n, vib_err, amean_g, vib_valid, rssi)
SELECT received_at,
    json_extract(payload,'$.device_id'), json_extract(payload,'$.fw'), json_extract(payload,'$.seq'),
    json_extract(payload,'$.uptime_ms'), json_extract(payload,'$.run'), json_extract(payload,'$.motor_dir'),
    json_extract(payload,'$.duty'), json_extract(payload,'$.settling'),
    json_extract(payload,'$.pulses_per_s'), json_extract(payload,'$.rpm'), json_extract(payload,'$.ppr_assumed'),
    json_extract(payload,'$.current_dmv'), json_extract(payload,'$.vib_rms_g'), json_extract(payload,'$.vib_n'),
    json_extract(payload,'$.vib_err'), json_extract(payload,'$.amean_g'), json_extract(payload,'$.vib_valid'),
    json_extract(payload,'$.rssi')
FROM messages WHERE topic='iiot/rig1/telemetry' AND json_valid(payload) ORDER BY id;
