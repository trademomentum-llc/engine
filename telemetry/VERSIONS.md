# VERSIONS.md — Pinned versions, AetherOS telemetry plane

Determinism contract: every external binary is pinned by version and verified
by sha256 at fetch time. `verify-at-fetch` means the field MUST be filled from
the upstream release's published checksum (or a computed `sha256sum` of the
fetched artifact) at bring-up, and the filled value committed back here.
**No hash below was invented; unverified fields are explicitly marked.**

> Honesty note: these are the newest stable versions verifiable from the
> build engineer's knowledge base (training cutoff early 2025). If a newer
> stable exists at bring-up time, bump the pin AND record the new sha256 in
> the same commit. Never run an unpinned version.

| Component | Layer | Pinned version | Upstream artifact (linux/amd64) | sha256 |
|-----------|-------|----------------|----------------------------------|--------|
| OpenTelemetry Collector (contrib) | 2 | v0.119.0 | `https://github.com/open-telemetry/opentelemetry-collector-releases/releases/download/v0.119.0/otelcol-contrib_0.119.0_linux_amd64.tar.gz` | verify-at-fetch |
| Prometheus | 3 | v3.2.1 | `https://github.com/prometheus/prometheus/releases/download/v3.2.1/prometheus-3.2.1.linux-amd64.tar.gz` | verify-at-fetch |
| Jaeger all-in-one (1.x line — flag-compatible with `--collector.otlp.*`) | 4 | v1.65.0 | `https://github.com/jaegertracing/jaeger/releases/download/v1.65.0/jaeger-1.65.0-linux-amd64.tar.gz` | verify-at-fetch |
| Grafana Loki (single binary) | 5 | v3.4.0 | `https://github.com/grafana/loki/releases/download/v3.4.0/loki-linux-amd64.zip` | verify-at-fetch |
| Wazuh (manager + agent) | 6 | 4.11.0 | `https://packages.wazuh.com/4.x/apt/pool/main/w/wazuh-agent/wazuh-agent_4.11.0-1_amd64.deb` (agent); manager per Wazuh 4.11 install guide | verify-at-fetch |
| osquery | 6 | 5.15.0 | `https://github.com/osquery/osquery/releases/download/5.15.0/osquery_5.15.0-1.linux_amd64.deb` | verify-at-fetch |
| Grafana (OSS) | 7 | v11.5.1 | `https://dl.grafana.com/oss/release/grafana-11.5.1.linux-amd64.tar.gz` | verify-at-fetch |

Build toolchain (Layer 1/2 C code): `gcc` with `-Wall -Wextra -O2 -std=c11
-D_GNU_SOURCE`; any gcc ≥ 11 satisfying zero-warning builds is acceptable.
QEMU: any `qemu-system-x86_64` ≥ 8.0 with user-mode networking (`-netdev user`).
