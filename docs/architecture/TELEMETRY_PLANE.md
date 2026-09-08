# AetherOS Telemetry Plane — 7-Layer Architecture

**Document class:** Architecture specification — binding interface description
**Governing contracts:** `SPEC.md` §"Layer Contracts", §"Emulated-environment plumbing", §"Layer 1 API (FIXED)"
**Environment:** QEMU-emulated guest, native binaries only, no Docker anywhere
**Doctrine:** `docs/procedures/FIRST_PRINCIPLES_OPERATING_DOCTRINE.md` governs how this spec is amended (spec-first, consensus before destructive change, honest uncertainty)

---

## 1. Overview and Layer Map

### 1.1 Model

1.1.1 The plane is OSI-modeled: seven layers, each with one responsibility and one contract. A layer consumes the contract of the layer below and exposes the contract stated in its own row. Implementations are substitutable if and only if they satisfy the contract row (Doctrine goal 4, Adaptability).

### 1.2 Layer map (ratified — mirrors `SPEC.md` §"Layer Contracts")

| # | Layer | Component | Contract (summary) |
|---|-------|-----------|--------------------|
| L1 | Instrumentation | aetherProbe C SDK + OpenTelemetry SDKs | Fixed C API (`seb.h`, `probe.h`); OTel SDKs for managed services |
| L2 | Collection & Routing | aether-collect + OpenTelemetry Collector (otelcol) | aether-collect drains SEB rings → OTLP gRPC to otelcol `0.0.0.0:4317`; otelcol routes by signal |
| L3 | Metrics & Monitoring | Prometheus | otelcol prometheus exporter on `:9464`; `prometheus.yml` scrapes otelcol + node/port targets; server `:9090` |
| L4 | Transaction Tracing | Jaeger | otelcol OTLP exporter → Jaeger all-in-one `:14317` (OTLP gRPC ingest, SPEC AMEND-001); UI `:16686` |
| L5 | Logging & Aggregation | Grafana Loki | otelcol `otlphttp/loki` exporter → Loki 3.x native OTLP endpoint `http://localhost:3100/otlp` (the deprecated `loki` exporter is removed from otelcol-contrib v0.119.0) |
| L6 | Integrity & Security | Wazuh + osquery | Wazuh manager `:1514`/`1515`; osquery FIM + process telemetry; drift/integrity alerts also emitted as `SEB_ALERT` into L1 |
| L7 | Visualization | Grafana | `:3000`; provisioned datasources Prometheus (`:9090`), Jaeger (`:16686`), Loki (`:3100`); dashboard `aether-overview` |

### 1.3 Failure classes owned by the plane

1.3.1 The plane exists to make five failure classes detectable and explainable: **crash**, **drift**, **integrity violation**, **performance regression**, **security event**. The layer ↔ failure-class mapping with notebook references is in §6.

---

## 2. Layer Specifications

Each subsection states: purpose, component, data contract, interfaces, failure modes, and the verification probe that proves the layer live after bring-up.

### 2.1 Layer 1 — Instrumentation

| Attribute | Specification |
|-----------|---------------|
| **Purpose** | Emit facts (metrics, traces, logs, alerts, events) at the source without corrupting the source; decouple emission from collection via a bounded ordered buffer |
| **Component** | aetherProbe C SDK (`telemetry/layer1-instrumentation/aether-probe/{seb.h,seb.c,probe.h,probe.c}`); OpenTelemetry SDKs for managed services |
| **Data contract** | SEB ring: 64 KB shared ring. `struct seb_event` = { magic, type, flags, len, timestamp (ns), payload ≤ 1024 B }. `probe.h` payloads are JSON, ≤ 1024 B. API (FIXED — signatures may not change, per SPEC.md): `seb_create`, `seb_open`, `seb_close`, `seb_publish`, `seb_consume`, `seb_peek`, `seb_now`; `aether_probe_init`, `aether_probe_close`, `aether_metric`, `aether_trace_begin`/`aether_trace_end`, `aether_log(level 1..5)`, `aether_alert(severity 1..3)`, `aether_event` |
| **Interfaces** | In-process C ABI only; no sockets at this layer. Ring handle obtained via `seb_create`/`seb_open`; consumed by L2 via `seb_consume`/`seb_peek` |
| **Failure modes** | (a) Ring overflow under burst — MUST be observable via the `dropped` counter, never silent; (b) payload truncation above 1024 B — rejected at publish; (c) emitter crash leaves ring intact for post-mortem drain (ring survives the process) |
| **Verification probe** | `tests/test_seb.c`: create ring, publish N=1000 mixed events, consume all, assert count + ordering + `dropped==0`; overflow case asserts `dropped>0`. Compiled with `gcc -Wall -Wextra -O2 -std=c11`, zero warnings (SPEC.md, Verification gates 1–2) |

### 2.2 Layer 2 — Collection & Routing

| Attribute | Specification |
|-----------|---------------|
| **Purpose** | Drain L1 rings and route each signal type (metrics, traces, logs) to its layer-appropriate backend |
| **Component** | aether-collect (`telemetry/layer2-collection/aether-collect/collect.c`) + OpenTelemetry Collector (`telemetry/layer2-collection/otelcol/otelcol-config.yaml`) |
| **Data contract** | aether-collect consumes `struct seb_event` records from every registered SEB ring and re-emits them as OTLP. otelcol receives OTLP and routes by signal: metrics → L3 exporter, traces → L4 exporter, logs → L5 exporter |
| **Interfaces** | aether-collect → otelcol: OTLP gRPC to `0.0.0.0:4317`. otelcol exporters: prometheus on `:9464` (to L3), OTLP gRPC to Jaeger `:14317` (to L4, SPEC AMEND-001), OTLP/HTTP to Loki `http://localhost:3100/otlp` (to L5) |
| **Failure modes** | (a) otelcol down → aether-collect backpressures into the ring; sustained outage surfaces as rising `dropped` at L1 — the failure is visible in L3 as the `dropped` metric; (b) malformed event (bad magic) — skipped and counted; (c) misrouted signal — detectable by absence at exactly one backend, presence at otelcol self-metrics (`:8888`, internal telemetry endpoint) |
| **Verification probe** | Publish a known metric + trace + log via `probe.h`; confirm the metric appears on `:9464`, the trace in Jaeger UI `:16686`, the log via Loki query at `:3100`. Config must parse under `yaml.safe_load` (Verification gate 3) |
| **Resolved item** | **Port contention — RESOLVED by SPEC AMEND-001 (ratified):** the SPEC originally recorded otelcol's OTLP gRPC receiver on `0.0.0.0:4317` and Jaeger all-in-one's OTLP gRPC ingest also on `:4317`, a certain bind conflict on one guest interface. AMEND-001 assigns Jaeger OTLP gRPC ingest to `:14317`; otelcol keeps `0.0.0.0:4317`. Reflected in §2.4, §3.1, §5.4, `BRINGUP.md`, and `otelcol-config.yaml`. |

### 2.3 Layer 3 — Metrics & Monitoring

| Attribute | Specification |
|-----------|---------------|
| **Purpose** | Durable, queryable storage of numeric time series; the primary crash/performance-regression surface |
| **Component** | Prometheus (`telemetry/layer3-metrics/prometheus/prometheus.yml`) |
| **Data contract** | Prometheus text exposition format scraped over HTTP; scrape targets = otelcol exporter plus node/port targets as enumerated in the committed `prometheus.yml` |
| **Interfaces** | Scrape source: otelcol prometheus exporter `:9464`. Server/UI/API: `:9090`. Consumed by L7 as provisioned datasource `Prometheus (:9090)` |
| **Failure modes** | (a) Target down → `up==0` per target (crash detection); (b) scrape gap → staleness markers, visible as series gaps; (c) cardinality explosion from unbounded labels — mitigated by fixed label sets in aether-collect mapping |
| **Verification probe** | `GET http://localhost:9090/api/v1/targets` returns all configured targets `up`; `GET /api/v1/query?query=up` returns 1 for the otelcol job |

### 2.4 Layer 4 — Transaction Tracing

| Attribute | Specification |
|-----------|---------------|
| **Purpose** | End-to-end trace of a transaction across instrumented components; the primary tool for causal (not merely correlational) diagnosis |
| **Component** | Jaeger all-in-one, native run in guest (`telemetry/layer4-tracing/jaeger/README.md`) |
| **Data contract** | OTLP trace protobufs ingested from otelcol; spans originate at L1 via `aether_trace_begin`/`aether_trace_end` (or OTel SDKs for managed services) |
| **Interfaces** | Ingest: OTLP gRPC `:14317` (SPEC AMEND-001, ratified — moved off `:4317`, which otelcol's own receiver owns). Query/UI: `:16686`. Consumed by L7 as provisioned datasource `Jaeger (:16686)` |
| **Failure modes** | (a) Ingest down → otelcol exporter queue growth, visible in otelcol self-metrics at `:8888`; (b) broken span parentage from misused begin/end pairing — detected as orphan-span rate; (c) all-in-one in-memory store loses history on restart — accepted for this wave (single-guest dev topology), recorded here as a known limitation |
| **Verification probe** | Emit a `aether_trace_begin`/`end` pair with a fixed test operation name; confirm the trace retrievable in the UI at `:16686` and via the Grafana Jaeger datasource |

### 2.5 Layer 5 — Logging & Aggregation

| Attribute | Specification |
|-----------|---------------|
| **Purpose** | Centralized, queryable log aggregation; the narrative record around any failure the other layers detect |
| **Component** | Grafana Loki (`telemetry/layer5-logging/loki/loki-config.yaml`) |
| **Data contract** | Log entries pushed by the otelcol `otlphttp/loki` exporter as OTLP logs; entries originate at L1 via `aether_log(level 1..5)` (levels map to severity labels) |
| **Interfaces** | Push: OTLP/HTTP `http://localhost:3100/otlp` (Loki 3.x native OTLP, `/otlp/v1/logs`). Query: `:3100` (LogQL). Consumed by L7 as provisioned datasource `Loki (:3100)` |
| **Failure modes** | (a) Push endpoint down → otelcol queue growth (`:8888` self-metrics); (b) label cardinality explosion — mitigated by low-cardinality labels only (level, source, layer); (c) storage growth unbounded — retention per committed `loki-config.yaml` |
| **Verification probe** | Emit `aether_log(3, "...")` with a unique marker string; confirm retrievable by LogQL query at `:3100` within the scrape/push interval |

### 2.6 Layer 6 — Integrity & Security

| Attribute | Specification |
|-----------|---------------|
| **Purpose** | Detect drift, integrity violations, and security events against the guest itself; close the loop by feeding its alerts back into the plane as first-class events |
| **Component** | Wazuh (manager + agent) + osquery (`telemetry/layer6-integrity/wazuh/ossec-agent.conf` snippet, `telemetry/layer6-integrity/osquery/osquery.conf`, `telemetry/layer6-integrity/INTEGRATION.md`) |
| **Data contract** | osquery supplies file-integrity monitoring (FIM) and process telemetry; Wazuh correlates and raises alerts. Drift/integrity alerts are additionally emitted as `SEB_ALERT` events into Layer 1 (via `aether_alert(severity 1..3)`), so a security event travels the same pipeline as any other fact — one plane, one record |
| **Interfaces** | Wazuh manager: `:1514` (agent events), `:1515` (agent enrollment). Both guest-internal: NOT in the hostfwd set (§5.2). Alert loop: `aether_alert` → SEB ring → aether-collect → otelcol → L3/L4/L5 → L7 |
| **Failure modes** | (a) Agent disconnected → enrollment/event silence at `:1514` — detectable as missing heartbeat; (b) FIM baseline stale after legitimate change — mitigated by doctrine 5.2: drift check before/after each session re-baselines expectations; (c) alert loop flood under mass-change events — severity gating at `aether_alert(1..3)` |
| **Verification probe** | Touch a monitored file inside the guest; confirm (i) Wazuh alert raised, and (ii) the corresponding `SEB_ALERT` appears in Loki (`:3100`) and as an alert metric in Prometheus (`:9090`) via the L1 loop |

### 2.7 Layer 7 — Visualization

| Attribute | Specification |
|-----------|---------------|
| **Purpose** | One pane per operator question; the only layer a human routinely touches |
| **Component** | Grafana (`telemetry/layer7-visualization/grafana/provisioning/datasources/datasources.yaml`, `telemetry/layer7-visualization/grafana/dashboards/aether-overview.json`) |
| **Data contract** | Provisioned, committed datasources and dashboards; no click-ops state. Dashboard of record: `aether-overview` |
| **Interfaces** | UI/API `:3000` (host-reachable via hostfwd, §5.2). Datasources: Prometheus (`:9090`), Jaeger (`:16686`), Loki (`:3100`) |
| **Failure modes** | (a) Datasource unreachable → panel errors naming the failing layer (by design, failure is attributed, not ambiguous); (b) dashboard drift from committed JSON — prevented by provisioning from committed files only |
| **Verification probe** | `GET http://localhost:3000/api/health` OK; `GET /api/datasources` lists exactly the three provisioned datasources; dashboard `aether-overview` renders all panels without datasource errors |

---

## 3. Data-Flow Diagram

### 3.1 Primary signal path (L1 → L7)

```
+------------------+   publish    +-------------+   consume/peek   +----------------+
| Instrumented     | ===========> | SEB ring    | ===============> | aether-collect |
| source           |  seb_event   | 64 KB,      |   (L2 reader)    | (collect.c)    |
| (aetherProbe C   |  <=1024B     | ordered,    |                  +-------+--------+
|  SDK / OTel SDKs)|  JSON        | drop-counted|                          | OTLP gRPC
+------------------+              +-------------+                          v
                                                                    +-------------+
                                                                    |  otelcol    |
                                                                    |  receiver   |
                                                                    | 0.0.0.0:4317|
                                                                    +------+------+
                                                                           | route by signal
              +------------------------------+-----------------------------+--------------------+
              | (metrics)                    | (traces)                    | (logs)             |
              v                              v                             v                    |
      +---------------+              +---------------+             +------------------+       |
      | prometheus    |   scrape     | OTLP gRPC     |   ingest    | otlphttp/loki    | push  |
      | exporter      | <=========== | exporter      | ==========> | exporter         | ======+
      | :9464         |              | :14317        |             +------------------+  HTTP
      +-------+-------+              +-------+-------+                          |  :3100/otlp/v1/logs
              | /metrics                     |                                v
              v                              v                        +--------------+
      +---------------+              +---------------+                | Loki  :3100  |
      | Prometheus    |              | Jaeger        |                +------+-------+
      | :9090         |              | all-in-one    |                       |
      +-------+-------+              | UI :16686     |                       |
              |                      +-------+-------+                       |
              | datasource :9090             | datasource :16686             | datasource :3100
              v                              v                               v
      +-----------------------------------------------------------------------------+
      | Grafana :3000  — provisioned datasources + dashboard "aether-overview" (L7) |
      +-----------------------------------------------------------------------------+
```

### 3.2 Layer-6 integrity loop

```
+-----------+   FIM/process    +----------------+   correlate   +------------------+
| osquery   | ==============>  | Wazuh agent -->| ============> | Wazuh manager    |
| (guest)   |   telemetry      |  :1514 events  |               | :1514 / :1515    |
+-----------+                  +----------------+               +--------+---------+
                                                                         | alert
                                                                         v
                                                              +---------------------+
                                                              | aether_alert(1..3)  |
                                                              | SEB_ALERT event     |
                                                              +----------+----------+
                                                                         |
                                                                         v  into L1 ring
                                                              (travels the primary
                                                               path of §3.1: SEB ring
                                                               -> aether-collect ->
                                                               otelcol -> Prom/Jaeger/
                                                               Loki -> Grafana)
```

3.2.1 Design intent: an integrity alert is not a side channel. It becomes a timestamped `seb_event` with the same provenance, ordering, and drop-accounting as every other fact, so the operator correlates security events with metrics, traces, and logs in one timeline at L7.

---

## 4. Determinism and Pinned-Version Policy

4.1 Every component version is pinned in `telemetry/VERSIONS.md` with the sha256 of its upstream tarball. Fetch-then-verify precedes install; an unverifiable artifact is not installed.

4.2 All configuration is committed text (otelcol, prometheus, loki, grafana provisioning, osquery, ossec snippet). No runtime-generated config; no click-ops state.

4.3 Bring-up is the single runbook `telemetry/BRINGUP.md`: pinned versions → fetch + sha256 verify → build/install → fixed run order **Loki → Jaeger → Prometheus → otelcol → Wazuh → Grafana** → per-layer verification probes (§2.1–2.7) → teardown.

4.4 Runtime network policy: no network calls except inter-process localhost inside the guest, plus the five host-forwarded operator ports (§5.2). A component that initiates external network traffic at runtime is a doctrine violation (Doctrine 4.2.4).

---

## 5. Emulated-Environment Deployment

### 5.1 QEMU guest-native model

5.1.1 The entire stack runs as native binaries inside the QEMU guest: systemd units for production-like runs, foreground processes for development (per SPEC.md §"Emulated-environment plumbing").

5.1.2 QEMU invocation pattern (worked example in `telemetry/BRINGUP.md`):

```
qemu-system-x86_64 ... \
  -netdev user,id=n0,\
hostfwd=tcp::3000-:3000,\
hostfwd=tcp::9090-:9090,\
hostfwd=tcp::16686-:16686,\
hostfwd=tcp::3100-:3100,\
hostfwd=tcp::9464-:9464 \
  -device virtio-net-pci,netdev=n0 ...
```

### 5.2 Host-forwarding table (user-mode networking, hostfwd)

| Host port | Guest port | Layer | Service | Purpose |
|-----------|-----------|-------|---------|---------|
| 3000 | 3000 | L7 | Grafana | Operator UI and API |
| 9090 | 9090 | L3 | Prometheus | Query/API, targets inspection |
| 16686 | 16686 | L4 | Jaeger | Trace UI; Grafana Jaeger datasource query port |
| 3100 | 3100 | L5 | Loki | LogQL queries; push endpoint is guest-local (`localhost:3100/otlp` OTLP/HTTP) |
| 9464 | 9464 | L2 | otelcol prometheus exporter | Direct scrape inspection/debug of collector output |

5.2.1 Deliberately NOT forwarded: Wazuh `:1514`/`1515` (guest-internal integrity plane), otelcol receiver `:4317` and Jaeger OTLP ingest (guest-internal hops). Minimum-forwarded-surface is the security posture: the host reaches exactly the five read/inspect endpoints an operator needs.

### 5.3 No Docker, anywhere — challenge-assumption rationale (summary)

5.3.1 Full analysis: Doctrine §2.3.3. Summary: containerization is a convention, not a physical requirement. Inside a QEMU guest it duplicates isolation the hypervisor already provides, introduces a daemon and registry pulls that violate the localhost-only runtime policy (§4.4), and interposes an abstraction between Layer 6 and the process truth it must observe. The hubserve repository is therefore DEPRECATED as a deployment model and retained for config mining only; no Docker artifacts may be introduced (SPEC.md, Deterministic source of truth).

### 5.4 Open items (honest uncertainty — Doctrine 4.6)

| # | Item | Uncertainty statement | Resolution path |
|---|------|----------------------|-----------------|
| OI-1 | `:4317` contention — RESOLVED (SPEC AMEND-001, ratified) | otelcol (`0.0.0.0:4317` receiver) and Jaeger all-in-one could not co-bind `:4317` on one guest interface | AMEND-001 assigns Jaeger OTLP gRPC ingest `:14317`; reflected in §2.2/§2.4, `BRINGUP.md`, and `otelcol-config.yaml` |
| OI-2 | SEB ring rendezvous | The ring's filesystem/shm rendezvous path between aetherProbe producers and aether-collect is not pinned in SPEC.md | To be fixed by the L1/L2 implementers and recorded in `BRINGUP.md`; must be a committed, absolute guest path |
| OI-3 | Wazuh manager placement | SPEC pins ports `:1514`/`1515` but not whether the manager runs in-guest. Assumed in-guest for determinism (all other components are); probability moderate that a future wave wants an external manager | Confirm at integration; if external, amend this section and the determinism clause together |

---

## 6. Layer ↔ Failure-Class ↔ Notebook Map

Each failure class is detectable at the layer(s) marked ●, secondarily corroborated at ○. The final column names where a user reads the worked explanation: the compiled knowledge notebooks produced by the notebook compiler from the wave's action catalog.

| Failure class | L1 | L2 | L3 | L4 | L5 | L6 | L7 | Read about it in the notebooks |
|---------------|----|----|----|----|----|----|----|--------------------------------|
| **Crash** (process/service death) | ○ (emitter silence) | ○ (`dropped` rise, drain gap) | ● (`up==0`, `up` query at `:9090`) | ○ (span truncation) | ● (last log lines before death, `:3100`) | ○ (process telemetry) | ● (`aether-overview` panels) | `knowledge/notebooks/2026-09-07-*.md` → Actions for L2/L3 bring-up and verification probes |
| **Drift** (config/binary change vs. baseline) | ○ (SEB_ALERT ingress) | ○ | ○ | — | ● (alert text searchable in Loki) | ● (Wazuh FIM + osquery; doctrine 5.2 drift check before/after) | ● (alert panel) | Notebooks → L6 INTEGRATION actions; drift-check events (`layer: "telemetry"`, `target: nnos/scripts/binary_drift_check.sh`) |
| **Integrity violation** (unauthorized modification) | ● (`aether_alert` 1..3) | ○ | ○ (alert metric) | — | ● (alert records) | ● (Wazuh correlation, `:1514`) | ● | Notebooks → L6 verification-probe action (touch monitored file → SEB_ALERT observed end-to-end) |
| **Performance regression** | ● (ns timestamps, metrics at source) | ○ (queue/lag) | ● (rate/histogram queries, `:9090`) | ● (span latency distributions, `:16686`) | ○ | ○ | ● (trend panels) | Notebooks → L1 `test_seb` and L3/L4 probe actions (baseline measurements recorded with hashes) |
| **Security event** | ● (SEB_ALERT) | ○ | ○ | — | ● (full alert narrative) | ● (osquery process/socket telemetry, Wazuh rules) | ● | Notebooks → L6 events and any `action: "decided"` records raising severity gates |

6.1 Reading path: notebooks are grouped by ISO date (UTC), one per date, named `YYYY-MM-DD-<slug>.md`, with a `## Timeline` table and per-action `## Actions` sections carrying rationale, inputs/outputs, hashes, and numbered replication steps; `notebooks/INDEX.md` links all notebooks sorted by date descending.

---

## 7. Cross-References

| Ref | Target | Location |
|-----|--------|----------|
| Governing spec | Interface contracts, deliverable map, verification gates | `SPEC.md` (wave root, 2026-09-07) |
| Operating doctrine | Conduct, challenge-assumption analysis, session protocol | `docs/procedures/FIRST_PRINCIPLES_OPERATING_DOCTRINE.md` |
| Bring-up runbook | Pinned versions, fetch+verify, run order, QEMU invocation, probes, teardown | `telemetry/BRINGUP.md` |
| Version pins | Exact versions + sha256 of upstream tarballs | `telemetry/VERSIONS.md` |
| Hash manifest | Per-file + category sha256 of all produced files | `telemetry/HASHES.sha256` |
| Action events | This wave's temporal action catalog (JSONL, one action per line) | `engine/knowledge/actions/2026-09-07-telemetry-plane.jsonl` |
| Notebook compiler | Events JSONL → dated markdown notebooks (`python3 -m temporal_kg notebook --input actions.jsonl --out-dir notebooks/`); deterministic, stdlib-only | `temporal-knowledge-graph` repo: `temporal_kg/notebook.py`, CLI extension, `docs/NOTEBOOK_COMPILER.md` |
| Compiled notebooks | User knowledgebase for this plane | `engine/knowledge/notebooks/` (+ `INDEX.md`) |
| Action schema | JSON Schema for action events | `temporal-knowledge-graph/schemas/action_event.schema.json` |
| Telemetry registration | Non-destructive extension | `engine/.observability/agent.yml` |
