# BRINGUP.md — AetherOS Telemetry Plane, deterministic bring-up (emulated env)

**NO DOCKER — anywhere in this plane.** First-principles rationale: telemetry
must observe the real kernel/process/filesystem state of the guest, and a
container runtime would interpose its own namespaces and cgroups between the
observer and the observed, adding a failure domain the plane exists to detect.

Scope: one QEMU guest (x86_64 Linux) runs every layer as native binaries.
Host reaches the guest only through user-mode-networking hostfwd ports.
Pinned versions and sha256 policy: see `VERSIONS.md`.

---

## 0. QEMU invocation (host)

```sh
qemu-system-x86_64 \
  -m 4096 -smp 2 -enable-kvm \
  -drive file=aetheros-lab.qcow2,format=qcow2 \
  -netdev user,id=n0,hostfwd=tcp::3000-:3000,hostfwd=tcp::9090-:9090,hostfwd=tcp::16686-:16686,hostfwd=tcp::3100-:3100,hostfwd=tcp::9464-:9464 \
  -device virtio-net-pci,netdev=n0 \
  -nographic
```

Hostfwd map: `3000` Grafana, `9090` Prometheus, `16686` Jaeger UI, `3100` Loki,
`9464` otelcol Prometheus exporter. **No forward for 14317** (Jaeger OTLP
ingest, SPEC AMEND-001) or 4317/1514 — those are guest-local only.

All remaining steps run **inside the guest** unless stated otherwise.

## 1. Fetch + verify (deterministic)

```sh
mkdir -p /opt/aether/{bin,lib,include,dist} /var/lib/loki /var/log/osquery
cd /opt/aether/dist

# Fetch each artifact URL from VERSIONS.md, then for EVERY artifact:
sha256sum <artifact>            # compare against VERSIONS.md
# If VERSIONS.md says verify-at-fetch: record the printed hash there and
# commit before first run. A hash mismatch aborts bring-up — never proceed.
```

Unpack/install per component: otelcol-contrib → `/opt/aether/bin/otelcol-contrib`;
prometheus → `/opt/aether/bin/prometheus` + `/etc/prometheus/prometheus.yml`;
jaeger → per `layer4-tracing/jaeger/README.md`; loki → `/opt/aether/bin/loki`
+ `/etc/loki/loki-config.yaml`; grafana → `/opt/grafana` with BOTH provisioning
trees installed: `layer7-visualization/grafana/provisioning/datasources` →
`/opt/grafana/conf/provisioning/datasources` and
`layer7-visualization/grafana/provisioning/dashboards` →
`/opt/grafana/conf/provisioning/dashboards`, plus the dashboard JSON
`layer7-visualization/grafana/dashboards/aether-overview.json` copied into
`/opt/grafana/conf/provisioning/dashboards/` (the provider's `options.path`
in `dashboards.yaml` points there);
wazuh agent+manager 4.11.0 per upstream packages (agent config merged from
`layer6-integrity/wazuh/ossec-agent.conf`); osquery deb +
`/etc/osquery/osquery.conf` from `layer6-integrity/osquery/osquery.conf`.

## 2. Build Layer 1/2 C components

```sh
cd telemetry/            # this directory
make                     # gcc -Wall -Wextra -O2 -std=c11 — zero warnings gate
make test                # ./test-seb must print ALL PASS, exit 0
make install             # /opt/aether/{bin,lib,include}
```

`make test` is a hard gate: a failed SEB contract test means Layer 1 cannot
be trusted to carry Layers 2–7.

**SEB rendezvous path (OI-2, fixed by `seb.c`):** rings live at
`/dev/shm/seb_<name>` — e.g. a probe initialized with service `kernel` creates
`/dev/shm/seb_kernel`, and `aether-collect kernel` attaches to the same file.
`/dev/shm` must be a writable tmpfs in the guest (default on systemd images).

## 3. Run order (strict — each layer depends on the previous being up)

```sh
# L5 first: log sink must exist before anything emits.
/opt/aether/bin/loki -config.file=/etc/loki/loki-config.yaml &

# L4: tracing sink; OTLP gRPC on :14317 per SPEC AMEND-001.
/opt/aether/bin/jaeger-all-in-one --collector.otlp.enabled=true \
    --collector.otlp.grpc.host-port=:14317 --memory.max-traces=100000 &

# L3: metrics store (scraping starts immediately; otelcol target will be
# DOWN until the next step — expected).
/opt/aether/bin/prometheus --config.file=/etc/prometheus/prometheus.yml \
    --storage.tsdb.path=/var/lib/prometheus &

# L2: routing fabric. Config from this repo.
/opt/aether/bin/otelcol-contrib --config=/etc/otelcol-contrib/otelcol-config.yaml &

# L6: integrity — manager first (binds :1514), then agent, then osquery.
/var/ossec/bin/wazuh-control start        # manager (in-guest, OI-3)
systemctl start wazuh-agent               # agent -> 127.0.0.1:1514
osqueryd --config_path=/etc/osquery/osquery.conf --disable_events=false &

# L7: visualization.
/opt/grafana/bin/grafana-server --homepath=/opt/grafana &

# Finally: drain SEB rings (Layer 1 -> 2). NOTE — WAVE-2 SCOPE:
# aether-collect is STDOUT-ONLY this wave; it emits no OTLP, so the
# L2 -> L7 forwarding path (otelcol routing into Prometheus/Jaeger/Loki)
# carries no SEB-derived traffic yet. Layers 3/4/5/7 are still started and
# verified for reachability so the wave-2 forwarding work lands on a
# proven-runnable stack.
/opt/aether/bin/aether-collect kernel daemon integrity &
```

## 4. Per-layer verification probes

| Layer | Probe (inside guest unless noted) | Expected |
|-------|-----------------------------------|----------|
| 1 SEB | `make test` → `./test-seb` | `ALL PASS`, exit 0 |
| 1 SEB | `ls /dev/shm/seb_*` | one file per instrumented service |
| 2 collect | `aether-collect <ring>` stdout | JSON event lines flowing |
| 2 otelcol | `curl -sf localhost:9464/metrics \| head` | Prometheus exposition text — PIPELINE metrics only (empty until OTLP traffic flows in wave 2) |
| 2 otelcol | `curl -sf localhost:8888/metrics \| grep otelcol_` | collector SELF-telemetry (`otelcol_*`); do NOT expect `otelcol_*` on :9464 |
| 3 prom | `curl -sf 'localhost:9090/api/v1/targets?state=active'` | `prometheus`, `otelcol` (:9464) + `otelcol-self` (:8888) jobs `health:up` |
| 3 prom | `promtool check config /etc/prometheus/prometheus.yml` | `SUCCESS` |
| 4 jaeger | `curl -sf localhost:16686/api/services` | REACHABILITY-ONLY this wave: HTTP 200 + valid JSON (may be an empty service list — aether-collect emits no OTLP spans until wave 2, so no service names are guaranteed) |
| 5 loki | `curl -sf localhost:3100/ready` | `ready` |
| 5 loki | `curl -sf -G localhost:3100/loki/api/v1/labels` | JSON label list |
| 6 wazuh | `/var/ossec/bin/agent_control -l` (on manager) | agent `Active` |
| 6 osquery | `osqueryi --json "SELECT 1;"` and check `/var/log/osquery/osqueryd.results.log` grows | scheduled rows |
| 7 grafana | from **host**: `curl -sf localhost:3000/api/health` | `{"database":"ok"}` |
| 7 dash | from **host**: open `http://localhost:3000/d/aether-overview` | 5 panels render |

End-to-end smoke (THIS WAVE — L1 → L2 stdout capture only): emit one event
with any Layer 1 probe (or run `test-seb` against a live ring name), then
confirm it appears as a JSON event line in `aether-collect` stdout. That is
the full guaranteed path this wave: aether-collect is stdout-only and emits
no OTLP.

**Wave-2 expectations (NOT guaranteed now):** once aether-collect gains OTLP
emission, the same event must also appear in `:9464/metrics` (pipeline
metrics), in Loki (LogQL query at `:3100`), as a trace in Jaeger (`:16686`,
ingest on `:14317`), and — for alerts — in the Grafana "Integrity alerts"
panel. Do not report those downstream sightings as pass/fail this wave.

## 5. Teardown

```sh
kill %1 %2 %3 %4 %5 %6 %7 %8 2>/dev/null   # if foregrounded as above, or:
pkill -f aether-collect; pkill grafana-server; pkill osqueryd
/var/ossec/bin/wazuh-control stop; systemctl stop wazuh-agent
pkill otelcol-contrib; pkill prometheus; pkill jaeger-all-in-one; pkill loki
rm -f /dev/shm/seb_*                       # release SEB rendezvous files
# Guest state dirs kept for forensics: /var/lib/{loki,prometheus,grafana}
# Host side: Ctrl-A X (or kill qemu-system-x86_64).
```

Order matters in reverse too: stop producers (aether-collect) before sinks so
no ring is orphaned mid-drain.

## 6. Known contract notes

- **SPEC AMEND-001 (ratified):** Jaeger OTLP gRPC ingest is `:14317`, not
  `:4317`; otelcol owns `0.0.0.0:4317`. Reflected in
  `layer2-collection/otelcol/otelcol-config.yaml` and the Jaeger README.
- **OI-2:** SEB rendezvous path is `/dev/shm/seb_<name>` (fixed in `seb.c`).
- **OI-3:** Wazuh manager runs in-guest; agent targets `127.0.0.1:1514`.
- Loki retention requires the compactor section — already enabled in
  `layer5-logging/loki/loki-config.yaml`.
