# Layer 4 — Jaeger all-in-one (native binary, NO Docker)

Distributed tracing backend for the AetherOS telemetry plane. Runs as a single
native binary inside the QEMU guest; receives OTLP/gRPC spans from otelcol
(Layer 2) and serves the query UI to the host via hostfwd.

## Pinned version

| Component | Version | Artifact |
|-----------|---------|----------|
| Jaeger all-in-one | **v1.65.0** (last 1.x line; see note below) | `jaeger-1.65.0-linux-amd64.tar.gz` |

> **Why 1.x and not 2.x:** the v1 all-in-one exposes the explicit collector
> flags used below (`--collector.otlp.enabled`, `--collector.otlp.grpc.host-port`).
> Jaeger 2.x restructured the binary around an embedded OTel Collector with a
> YAML config and different flag surface. If the plane later moves to Jaeger
> 2.x, the equivalent is a config file with an `otlp` receiver bound to
> `:14317`. This wave pins 1.65.0 so the invocation stays flag-driven and
> deterministic.

## Fetch + verify

```sh
cd /opt/aether/dist
curl -fLO https://github.com/jaegertracing/jaeger/releases/download/v1.65.0/jaeger-1.65.0-linux-amd64.tar.gz
# Verify against the hash recorded in VERSIONS.md (verify-at-fetch field).
sha256sum jaeger-1.65.0-linux-amd64.tar.gz
tar xzf jaeger-1.65.0-linux-amd64.tar.gz
install -m 755 jaeger-1.65.0-linux-amd64/jaeger-all-in-one /opt/aether/bin/jaeger-all-in-one
```

Never skip the sha256 check: a tracing backend sees every span the OS emits,
so a tampered binary is a telemetry-plane compromise.

## Run (foreground, dev)

```sh
/opt/aether/bin/jaeger-all-in-one \
  --collector.otlp.enabled=true \
  --collector.otlp.grpc.host-port=:14317 \
  --memory.max-traces=100000
```

- `--collector.otlp.enabled=true` turns on OTLP ingest (off by default in 1.x).
- `--collector.otlp.grpc.host-port=:14317` moves OTLP gRPC **off** the default
  4317, per **SPEC AMEND-001**: otelcol's own OTLP receiver owns
  `0.0.0.0:4317` and both processes are co-located in this guest. 14317 is
  guest-local only — no QEMU hostfwd entry is needed for it.
- Storage: in-memory (all-in-one default). Deterministic for the lab; spans
  do not survive restart.

## Ports

| Port | Consumer |
|------|----------|
| 14317 | OTLP gRPC ingest (from otelcol `otlp/jaeger` exporter) |
| 16686 | Query UI + HTTP API (hostfwd'd to host) |
| 16685 | gRPC query API (Grafana Jaeger datasource uses HTTP on 16686) |

## Verify

```sh
# UI reachable (from host, via hostfwd tcp::16686-:16686)
curl -sf http://localhost:16686/api/services
# After otelcol + aether-collect are up, service names from Layer 1 appear here.
```
