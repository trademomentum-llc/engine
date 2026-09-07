# Layer 6 — Integrity & Security Integration (Wazuh + osquery)

Layer 6 watches the telemetry plane itself. Two engines run in the guest:

- **Wazuh agent** (`wazuh/ossec-agent.conf`) — FIM + rootcheck, reporting to
  the Wazuh **manager over `127.0.0.1:1514/tcp`**.
- **osqueryd** (`osquery/osquery.conf`) — scheduled SQL telemetry: `processes`
  (60s), `listening_ports` (60s), telemetry-binary re-hash (300s), and
  `file_events` FIM stream (30s) over `/opt/aether` and all telemetry config
  trees.

## Manager placement: in the guest (confirmed, OI-3)

The manager runs **in-guest**, not on the host. (1) Every other telemetry
endpoint in the plane is guest-loopback, so keeping `:1514` on loopback adds
zero hostfwd surface and zero new trust boundaries. (2) If the manager lived on
the host, agent enrollment (`:1515`) and alerting would depend on the user-mode
NAT path — a moving part that must not sit underneath integrity alerting.

## Integrity alerts flow through the SAME pipeline (SEB_ALERT)

Layer 6 alerts are **also** emitted into Layer 1 as `SEB_ALERT` events, so they
travel the identical path as all other telemetry:

```
wazuh alerts.json / osquery results log
        │
        ▼  (aether-integrity-bridge: tails both logs, linked against libaether.so)
aether_alert(probe, severity, category, description)     ← Layer 1 SDK, FIXED API
        │
        ▼  seb_publish(ring, SEB_ALERT, json, len)
/dev/shm/seb_integrity  (SEB ring — rendezvous path /dev/shm/seb_<name>)
        │
        ▼  aether-collect integrity   (Layer 2 drain)
otelcol (OTLP :4317) ──► logs pipeline ──► Loki (:3100)   alert record, searchable
                     └─► metrics pipeline ──► Prometheus (:9464)
                            aether_alerts_total{category,severity}  counter
        │
        ▼
Grafana aether-overview → "Integrity alerts" panel (Layer 7)
```

Design rules for the bridge:

1. **One-way, append-only.** The bridge only *publishes*; it never consumes.
   Layer 6 cannot be blinded by a full ring — overflow increments `dropped`,
   which is itself a Prometheus-visible integrity signal.
2. **Severity mapping:** Wazuh rule level 1–6 → `aether_alert` severity 1
   (info), 7–11 → 2 (warning), ≥12 → 3 (critical). osquery integrity rows
   (hash drift, unexpected listening port) → severity 2, `category="integrity"`.
3. **Payload ≤ 1024 B** JSON per the Layer 1 contract; long Wazuh fields are
   truncated, never dropped silently.
4. The bridge is a ~150-line C program built by the telemetry Makefile against
   `libaether.so`; it does not re-implement SEB — it calls `aether_probe_init`
   on service name `integrity` and `aether_alert()` only.

## Files

| File | Deploys to (guest) |
|------|--------------------|
| `wazuh/ossec-agent.conf` | merged into `/var/ossec/etc/ossec.conf` |
| `osquery/osquery.conf` | `/etc/osquery/osquery.conf` |
| (bridge source) | built from `telemetry/` Makefile wave 2 |
