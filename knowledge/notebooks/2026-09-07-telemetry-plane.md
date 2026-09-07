---
title: "Telemetry Plane — 2026-09-07"
date: 2026-09-07
generated_at: "2026-09-07T20:45:00Z"
event_count: 10
source: "session:2026-09-07-aetheros-telemetry"
---

# Telemetry Plane — 2026-09-07

## Summary

This notebook records 10 action event(s) for 2026-09-07, spanning 2026-09-07T19:28:56Z to 2026-09-07T20:45:00Z UTC.

- Actions: created: 6, decided: 1, verified: 3
- Layers: knowledge, procedures, recon, telemetry
- Actors: kimi-orchestrator, subagent:docs-writer, subagent:kg-engineer, subagent:telemetry-engineer, subagent:verifier

## Timeline

| Time (UTC) | Actor | Layer | Action | Target | Summary |
| --- | --- | --- | --- | --- | --- |
| 19:28:56 | kimi-orchestrator | recon | verified | github:trademomentum-llc | Established deterministic repo ground truth before any build |
| 19:35:00 | kimi-orchestrator | procedures | created | plan.md + SPEC.md | Wrote wave plan and binding SPEC.md before implementation |
| 19:45:00 | subagent:docs-writer | procedures | created | engine:docs/procedures/FIRST_PRINCIPLES_OPERATING_DOCTRINE.md | Codified the operating doctrine |
| 19:45:00 | subagent:docs-writer | telemetry | created | engine:docs/architecture/TELEMETRY_PLANE.md | Specified the 7-layer telemetry plane |
| 19:50:00 | subagent:telemetry-engineer | telemetry | created | engine:telemetry/ | Materialized and extended the telemetry tree |
| 19:55:00 | kimi-orchestrator | telemetry | decided | SPEC.md AMEND-001 | Moved Jaeger OTLP gRPC ingest to port 14317 |
| 20:05:00 | subagent:kg-engineer | knowledge | created | temporal-knowledge-graph:notebook compiler | Built the temporal-KG to markdown notebook compiler |
| 20:20:00 | subagent:verifier | knowledge | verified | temporal-knowledge-graph deliverables | Independent verification of the notebook compiler |
| 20:30:00 | kimi-orchestrator | telemetry | verified | engine:telemetry/ C + configs | Main-agent gate re-run |
| 20:45:00 | kimi-orchestrator | knowledge | created | engine:knowledge/ | Cataloged the session and compiled the first knowledgebase notebook |

## Actions

### AETH-2026-09-07-0001 — Established deterministic repo ground truth before any build

- **Actor:** kimi-orchestrator
- **Layer:** recon
- **Action:** verified
- **Target:** github:trademomentum-llc
- **Recorded at:** 2026-09-07T19:28:56Z
- **Valid from:** 2026-09-07T19:28:56Z

**Rationale**

Recon-before-build: prior chat context maxed out, so nothing was assumed committed. Verified via GitHub API that engine (default branch master) holds the microkernel, temporal-knowledge-graph holds the KG prototype, morehlex-deterministic (created 2026-09-07) holds the aetherProbe generator scripts and compiler swarm task specs, and hubserve is Docker-based therefore deprecated as a deployment model under the emulated-environment decision.

**Inputs**

- github org trademomentum-llc repository listing

**Outputs**

- verified repo map

**Hashes**

_No artifact hashes recorded._

**Replication Steps**

1. Search repositories: org:trademomentum-llc sorted by updated
2. Read engine root, .observability/agent.yml, nnos/TODO.md, README.md
3. Read morehlex-deterministic aetherProbe/ scripts 1-5 and compiler_requirements.md
4. Read temporal-knowledge-graph README.md and SPEC.md, hubserve root listing

### AETH-2026-09-07-0002 — Wrote wave plan and binding SPEC.md before implementation

- **Actor:** kimi-orchestrator
- **Layer:** procedures
- **Action:** created
- **Target:** plan.md + SPEC.md
- **Recorded at:** 2026-09-07T19:35:00Z
- **Valid from:** 2026-09-07T19:35:00Z

**Rationale**

Spec-first doctrine: interface contracts (Layer 1 C API, ports, action-event schema, notebook markdown format) fixed before any subagent wrote code, so modules integrate cleanly and no agent makes unilateral changes.

**Inputs**

- recon findings from AETH-2026-09-07-0001
- user directive: 7-layer OSI-modeled telemetry, OSS stack, temporal KG notebooks

**Outputs**

- plan.md
- SPEC.md

**Hashes**

_No artifact hashes recorded._

**Replication Steps**

1. Write plan.md staging recon, doctrine, telemetry, knowledge graph, integration
2. Write SPEC.md with deliverable map for both repos, layer contracts, schema, notebook format, verification gates

### AETH-2026-09-07-0003 — Codified the operating doctrine

- **Actor:** subagent:docs-writer
- **Layer:** procedures
- **Action:** created
- **Target:** engine:docs/procedures/FIRST_PRINCIPLES_OPERATING_DOCTRINE.md
- **Recorded at:** 2026-09-07T19:45:00Z
- **Valid from:** 2026-09-07T19:45:00Z

**Rationale**

Ground rules precede construction: first-principles loop (identify, decompose to primitives, challenge assumptions, rebuild from verified truth), six core goals with measurable tests, engineering procedures (spec-first, deterministic builds, hash provenance, action cataloging, consensus before destructive actions, honest uncertainty), session protocol with stage gates G0-G5.

**Inputs**

- SPEC.md 'Procedures Doctrine outline'

**Outputs**

- engine:docs/procedures/FIRST_PRINCIPLES_OPERATING_DOCTRINE.md

**Hashes**

| Path | SHA-256 |
| --- | --- |
| docs/procedures/FIRST_PRINCIPLES_OPERATING_DOCTRINE.md | `sha256:f7bfa9acb6879b33817a0313010ae686056b8ac412b964f611f0c9b99787ddc4` |

**Replication Steps**

1. Read SPEC.md doctrine outline
2. Write hierarchical-numbered doctrine with challenge-assumption table and compliance map

### AETH-2026-09-07-0004 — Specified the 7-layer telemetry plane

- **Actor:** subagent:docs-writer
- **Layer:** telemetry
- **Action:** created
- **Target:** engine:docs/architecture/TELEMETRY_PLANE.md
- **Recorded at:** 2026-09-07T19:45:00Z
- **Valid from:** 2026-09-07T19:45:00Z

**Rationale**

Telemetry mirrors the system it observes: OSI-modeled layers 1-7 mapped to aetherProbe SDK, OTel Collector + aether-collect, Prometheus, Jaeger, Loki, Wazuh+osquery, Grafana; guest-native in QEMU emulation with hostfwd ports 3000/9090/16686/3100/9464; no Docker anywhere.

**Inputs**

- SPEC.md 'Layer Contracts'
- engine:telemetry contracts

**Outputs**

- engine:docs/architecture/TELEMETRY_PLANE.md

**Hashes**

| Path | SHA-256 |
| --- | --- |
| docs/architecture/TELEMETRY_PLANE.md | `sha256:2ee3a83c72fe78cdd4a088a3826ddb216d201176a5813373e80b6554abeea71f` |

**Replication Steps**

1. Read SPEC.md layer contracts
2. Write per-layer spec tables, data-flow diagrams, failure-class map, emulation plumbing

### AETH-2026-09-07-0005 — Materialized and extended the telemetry tree

- **Actor:** subagent:telemetry-engineer
- **Layer:** telemetry
- **Action:** created
- **Target:** engine:telemetry/
- **Recorded at:** 2026-09-07T19:50:00Z
- **Valid from:** 2026-09-07T19:50:00Z

**Rationale**

Rebuild from verified source: heredoc payloads from the five aetherProbe generator scripts (morehlex-deterministic @0818112d) materialized verbatim as C sources (Layer 1 API untouched); new test_seb.c proves the SEB ring; configs for otelcol, Prometheus, Loki, Wazuh, osquery, Grafana written for guest-native deployment.

**Inputs**

- morehlex-deterministic:aetherProbe/1-seb-header.sh..5-makefile.sh

**Outputs**

- engine:telemetry/BRINGUP.md
- engine:telemetry/HASHES.sha256
- engine:telemetry/Makefile
- engine:telemetry/VERSIONS.md
- engine:telemetry/layer1-instrumentation/aether-probe/probe.c
- engine:telemetry/layer1-instrumentation/aether-probe/probe.h
- engine:telemetry/layer1-instrumentation/aether-probe/seb.c
- engine:telemetry/layer1-instrumentation/aether-probe/seb.h
- engine:telemetry/layer1-instrumentation/aether-probe/tests/test_seb.c
- engine:telemetry/layer2-collection/aether-collect/collect.c
- engine:telemetry/layer2-collection/otelcol/otelcol-config.yaml
- engine:telemetry/layer3-metrics/prometheus/prometheus.yml
- engine:telemetry/layer4-tracing/jaeger/README.md
- engine:telemetry/layer5-logging/loki/loki-config.yaml
- engine:telemetry/layer6-integrity/INTEGRATION.md
- engine:telemetry/layer6-integrity/osquery/osquery.conf
- engine:telemetry/layer6-integrity/wazuh/ossec-agent.conf
- engine:telemetry/layer7-visualization/grafana/dashboards/aether-overview.json
- engine:telemetry/layer7-visualization/grafana/provisioning/datasources/datasources.yaml

**Hashes**

| Path | SHA-256 |
| --- | --- |
| telemetry/BRINGUP.md | `sha256:d28ecbad001db1e4901ff96346d98968f3d365c99fe8118695dba2b463ad161a` |
| telemetry/HASHES.sha256 | `sha256:a7e808949de8130bcffbdf1a10a182405fc94046e5ede0bee62e90f802782b2d` |
| telemetry/Makefile | `sha256:26b5e4860b8ed6efbd14668f075911f56c9008c8c49ab8978bbfe33e375f3519` |
| telemetry/VERSIONS.md | `sha256:3b8360eb0808eac746eb13ee69671d80bd327bda4a4b00b3cef1821c702bf358` |
| telemetry/layer1-instrumentation/aether-probe/probe.c | `sha256:eda52a2188bd5e307e03c7d600e0a1394bdcdcadeb7e6eccc28400fc968737cb` |
| telemetry/layer1-instrumentation/aether-probe/probe.h | `sha256:350f424c8d998962277ead4caf19a46f07e966a0a6fd60c60745421007a2ee3b` |
| telemetry/layer1-instrumentation/aether-probe/seb.c | `sha256:5b56451c9355f6fbbe11f2aec6c6b379e43690fccfeb598aed2509e872731e13` |
| telemetry/layer1-instrumentation/aether-probe/seb.h | `sha256:fdaaba402190784dd21a576c42df765fc5d96032a689f93d318343e98dda36f4` |
| telemetry/layer1-instrumentation/aether-probe/tests/test_seb.c | `sha256:21702a7e9e98b8676b9c96a65c5a971a632be9e6f4d8df710266ff2404f4041f` |
| telemetry/layer2-collection/aether-collect/collect.c | `sha256:b1899263c14644aaa6ce2644233e31427c33ce1a1569e88d2c4559b1d5046cc2` |
| telemetry/layer2-collection/otelcol/otelcol-config.yaml | `sha256:6f30c80f51de95551f0e6a3a7c8fd09777d57b20b2ba351fe9b24abadd0081e1` |
| telemetry/layer3-metrics/prometheus/prometheus.yml | `sha256:ef53b8537abf3b269901877bfa245e87b641e1e5575a92715b0184d700ddf055` |
| telemetry/layer4-tracing/jaeger/README.md | `sha256:a858e4bec94c96de70803b6e852b8a6cf851489aa0ca1f2e374b0aa807caf029` |
| telemetry/layer5-logging/loki/loki-config.yaml | `sha256:31621631dd3b2d887157cc38c71d97a2eda40c1d37213060e35f1dc6609d0c83` |
| telemetry/layer6-integrity/INTEGRATION.md | `sha256:c7c0adeb587e1efefc03e62fa9cc2cee8375b7aad00d8e75f3243cb181f062ac` |
| telemetry/layer6-integrity/osquery/osquery.conf | `sha256:896ddad02785e431ab91bd17dc69f2c6bec27ad7011de8a07614d1c722898fb0` |
| telemetry/layer6-integrity/wazuh/ossec-agent.conf | `sha256:a6d17a9ff9796c06e852481dd39ba20557b8d8db9a1d4c31591cd917f32fe19a` |
| telemetry/layer7-visualization/grafana/dashboards/aether-overview.json | `sha256:3dc72acb06b9be3dd5af3d141080f736b0b5f7d45fb6e04a3aab9cda2264228d` |
| telemetry/layer7-visualization/grafana/provisioning/datasources/datasources.yaml | `sha256:27e449c6074fb766f76efac360c5b6aca268580dd753ca4e91c8870b4cad1d7b` |

**Replication Steps**

1. Fetch aetherProbe scripts 1-5 via GitHub API
2. Extract heredoc payloads verbatim into layered layout
3. Write tests/test_seb.c (1000-event FIFO + overflow-drop cases)
4. Write otelcol/prometheus/loki/grafana/osquery/wazuh configs
5. Write BRINGUP.md and VERSIONS.md with pinned versions and verify-at-fetch sha256 policy
6. Build with gcc -Wall -Wextra -O2 -std=c11: zero warnings; run test-seb: ALL PASS

### AETH-2026-09-07-0006 — Moved Jaeger OTLP gRPC ingest to port 14317

- **Actor:** kimi-orchestrator
- **Layer:** telemetry
- **Action:** decided
- **Target:** SPEC.md AMEND-001
- **Recorded at:** 2026-09-07T19:55:00Z
- **Valid from:** 2026-09-07T19:55:00Z

**Rationale**

Physical constraint found by the writer agent: otelcol OTLP receiver (0.0.0.0:4317) and Jaeger all-in-one OTLP ingest (default :4317) conflict when co-located in one guest. Amended SPEC rather than letting agents deviate unilaterally. Open item: Jaeger 2.x migration changes the flag surface; pinned 1.65.0 documented with migration path.

**Inputs**

- docs-writer open item OI-1

**Outputs**

- SPEC.md AMEND-001
- otelcol exporter endpoint localhost:14317
- jaeger README --collector.otlp.grpc.host-port=:14317

**Hashes**

_No artifact hashes recorded._

**Replication Steps**

1. Confirm conflict (both components bind 4317 in-guest)
2. Amend SPEC.md layer-4 row
3. Notify telemetry engineer to apply to configs and BRINGUP

### AETH-2026-09-07-0007 — Built the temporal-KG to markdown notebook compiler

- **Actor:** subagent:kg-engineer
- **Layer:** knowledge
- **Action:** created
- **Target:** temporal-knowledge-graph:notebook compiler
- **Recorded at:** 2026-09-07T20:05:00Z
- **Valid from:** 2026-09-07T20:05:00Z

**Rationale**

Users need to reference facts about the system: every action is a temporal action event (JSONL, schema-validated) compiled deterministically into per-date markdown notebooks with timeline, replication steps, artifact hashes, and provenance sha256. Stdlib only; generated_at derives from event data, never wall clock.

**Inputs**

- SPEC.md 'Action Event Schema' + 'Notebook Compiler Contract'
- temporal-knowledge-graph existing package (models/storage/cli)

**Outputs**

- temporal-knowledge-graph:temporal_kg/notebook.py
- temporal-knowledge-graph:temporal_kg/cli.py (additive notebook subcommand)
- temporal-knowledge-graph:schemas/action_event.schema.json
- temporal-knowledge-graph:tests/test_notebook.py
- temporal-knowledge-graph:docs/NOTEBOOK_COMPILER.md
- temporal-knowledge-graph:examples/telemetry_plane_actions.jsonl
- temporal-knowledge-graph:README.md (appended section)

**Hashes**

| Path | SHA-256 |
| --- | --- |
| README.md | `sha256:ee7321b69ffe52bec3725ff1ffa6867aa447d31ff13c4c72e1f2b101fff33f9f` |
| docs/NOTEBOOK_COMPILER.md | `sha256:10e9e79509c6154c629deec09db29cfaf11ad8f8b9d4fbbdae348509a1b943d4` |
| examples/telemetry_plane_actions.jsonl | `sha256:0019f246da534821add203a5a9a9a3e0d6c27f8c10f1eef4ff4a37023d0803aa` |
| schemas/action_event.schema.json | `sha256:6557f46864b2ed9a0b78502672eeb5a76e6f6a449b474846a3a3f5c4a783b201` |
| temporal_kg/cli.py | `sha256:a1884faf0c98eac7436749a7da7eafcd59d23f3f149c4c3a994ae79c8e8b71cd` |
| temporal_kg/notebook.py | `sha256:14bf4e19e4b74c8508a20eb1ccf0bb74b60249be2f8fc08c58319d5791c7b782` |
| tests/test_notebook.py | `sha256:0cad356044808d80b0b138714333797a231c2dd79a33d3b054b016ae6665098c` |

**Replication Steps**

1. Fetch existing temporal_kg package to match house style
2. Implement compile_notebooks with deterministic ordering and provenance hashing
3. Extend cli.py additively with the notebook subcommand
4. Write 7 unittest cases including determinism and validation-error naming
5. Run python3 -m unittest discover -s tests -v: 18/18 OK

### AETH-2026-09-07-0008 — Independent verification of the notebook compiler

- **Actor:** subagent:verifier
- **Layer:** knowledge
- **Action:** verified
- **Target:** temporal-knowledge-graph deliverables
- **Recorded at:** 2026-09-07T20:20:00Z
- **Valid from:** 2026-09-07T20:20:00Z

**Rationale**

Deterministic safety decision: main agent does not trust builder self-reports; a read-only verifier re-fetched the full package from GitHub main @d6fc60c, overlaid deliverables, and re-ran all gates adversarially.

**Inputs**

- deliverables overlay
- repo main @d6fc60c

**Outputs**

- verification report: 18/18 tests OK; 3x byte-identical compiles; schema Draft202012 valid; zero wall-clock leakage

**Hashes**

_No artifact hashes recorded._

**Replication Steps**

1. Fetch full package fresh from GitHub
2. Overlay deliverables
3. Run full unittest suite (18 tests)
4. Run CLI smoke 3 times into separate dirs; diff -r byte-identical
5. Adversarial: reversed input order, multi-date, malformed input handling

### AETH-2026-09-07-0009 — Main-agent gate re-run

- **Actor:** kimi-orchestrator
- **Layer:** telemetry
- **Action:** verified
- **Target:** engine:telemetry/ C + configs
- **Recorded at:** 2026-09-07T20:30:00Z
- **Valid from:** 2026-09-07T20:30:00Z

**Rationale**

Independent gate execution: rebuilt all Layer 1/2 C with gcc -Wall -Wextra -O2 -std=c11 (zero warnings), ran test-seb (ALL PASS: 1000-event FIFO + overflow), parsed all YAML/JSON configs.

**Inputs**

- deliverables tree

**Outputs**

- gate results: compile 0 warnings, test-seb ALL PASS, 4 YAML + 3 JSON parse OK

**Hashes**

_No artifact hashes recorded._

**Replication Steps**

1. gcc -Wall -Wextra -O2 -std=c11 -D_GNU_SOURCE each C source
2. link and run test-seb in sandbox /dev/shm
3. yaml.safe_load / json.load every config

### AETH-2026-09-07-0010 — Cataloged the session and compiled the first knowledgebase notebook

- **Actor:** kimi-orchestrator
- **Layer:** knowledge
- **Action:** created
- **Target:** engine:knowledge/
- **Recorded at:** 2026-09-07T20:45:00Z
- **Valid from:** 2026-09-07T20:45:00Z

**Rationale**

Self-hosting the doctrine: the action-cataloging mandate is exercised on the wave that created it. Nine action events (recon through verification) recorded per schemas/action_event.schema.json, then compiled deterministically via temporal_kg notebook into a markdown notebook with replication steps and per-file sha256 provenance. The push-to-GitHub event is chronologically last and is recorded in the next session's action log.

**Inputs**

- session action history 0001-0009
- temporal-knowledge-graph notebook compiler

**Outputs**

- engine:knowledge/actions/2026-09-07-telemetry-plane.jsonl
- engine:knowledge/notebooks/2026-09-07-telemetry-plane.md
- engine:knowledge/notebooks/INDEX.md

**Hashes**

| Path | SHA-256 |
| --- | --- |
| knowledge/actions/2026-09-07-telemetry-plane.jsonl | `sha256:SELF-REFERENTIAL-AT-WRITE` |
| knowledge/notebooks/2026-09-07-telemetry-plane.md | `sha256:computed-post-compile` |
| knowledge/notebooks/INDEX.md | `sha256:computed-post-compile` |

**Replication Steps**

1. Append this event to knowledge/actions/2026-09-07-telemetry-plane.jsonl
2. python3 -m temporal_kg notebook --input knowledge/actions/2026-09-07-telemetry-plane.jsonl --out-dir knowledge/notebooks --title 'Telemetry Plane'
3. sha256sum the compiled notebooks into knowledge/HASHES.sha256

## Artifacts & Hashes

| Path | SHA-256 |
| --- | --- |
| README.md | `sha256:ee7321b69ffe52bec3725ff1ffa6867aa447d31ff13c4c72e1f2b101fff33f9f` |
| docs/NOTEBOOK_COMPILER.md | `sha256:10e9e79509c6154c629deec09db29cfaf11ad8f8b9d4fbbdae348509a1b943d4` |
| docs/architecture/TELEMETRY_PLANE.md | `sha256:2ee3a83c72fe78cdd4a088a3826ddb216d201176a5813373e80b6554abeea71f` |
| docs/procedures/FIRST_PRINCIPLES_OPERATING_DOCTRINE.md | `sha256:f7bfa9acb6879b33817a0313010ae686056b8ac412b964f611f0c9b99787ddc4` |
| examples/telemetry_plane_actions.jsonl | `sha256:0019f246da534821add203a5a9a9a3e0d6c27f8c10f1eef4ff4a37023d0803aa` |
| knowledge/actions/2026-09-07-telemetry-plane.jsonl | `sha256:SELF-REFERENTIAL-AT-WRITE` |
| knowledge/notebooks/2026-09-07-telemetry-plane.md | `sha256:computed-post-compile` |
| knowledge/notebooks/INDEX.md | `sha256:computed-post-compile` |
| schemas/action_event.schema.json | `sha256:6557f46864b2ed9a0b78502672eeb5a76e6f6a449b474846a3a3f5c4a783b201` |
| telemetry/BRINGUP.md | `sha256:d28ecbad001db1e4901ff96346d98968f3d365c99fe8118695dba2b463ad161a` |
| telemetry/HASHES.sha256 | `sha256:a7e808949de8130bcffbdf1a10a182405fc94046e5ede0bee62e90f802782b2d` |
| telemetry/Makefile | `sha256:26b5e4860b8ed6efbd14668f075911f56c9008c8c49ab8978bbfe33e375f3519` |
| telemetry/VERSIONS.md | `sha256:3b8360eb0808eac746eb13ee69671d80bd327bda4a4b00b3cef1821c702bf358` |
| telemetry/layer1-instrumentation/aether-probe/probe.c | `sha256:eda52a2188bd5e307e03c7d600e0a1394bdcdcadeb7e6eccc28400fc968737cb` |
| telemetry/layer1-instrumentation/aether-probe/probe.h | `sha256:350f424c8d998962277ead4caf19a46f07e966a0a6fd60c60745421007a2ee3b` |
| telemetry/layer1-instrumentation/aether-probe/seb.c | `sha256:5b56451c9355f6fbbe11f2aec6c6b379e43690fccfeb598aed2509e872731e13` |
| telemetry/layer1-instrumentation/aether-probe/seb.h | `sha256:fdaaba402190784dd21a576c42df765fc5d96032a689f93d318343e98dda36f4` |
| telemetry/layer1-instrumentation/aether-probe/tests/test_seb.c | `sha256:21702a7e9e98b8676b9c96a65c5a971a632be9e6f4d8df710266ff2404f4041f` |
| telemetry/layer2-collection/aether-collect/collect.c | `sha256:b1899263c14644aaa6ce2644233e31427c33ce1a1569e88d2c4559b1d5046cc2` |
| telemetry/layer2-collection/otelcol/otelcol-config.yaml | `sha256:6f30c80f51de95551f0e6a3a7c8fd09777d57b20b2ba351fe9b24abadd0081e1` |
| telemetry/layer3-metrics/prometheus/prometheus.yml | `sha256:ef53b8537abf3b269901877bfa245e87b641e1e5575a92715b0184d700ddf055` |
| telemetry/layer4-tracing/jaeger/README.md | `sha256:a858e4bec94c96de70803b6e852b8a6cf851489aa0ca1f2e374b0aa807caf029` |
| telemetry/layer5-logging/loki/loki-config.yaml | `sha256:31621631dd3b2d887157cc38c71d97a2eda40c1d37213060e35f1dc6609d0c83` |
| telemetry/layer6-integrity/INTEGRATION.md | `sha256:c7c0adeb587e1efefc03e62fa9cc2cee8375b7aad00d8e75f3243cb181f062ac` |
| telemetry/layer6-integrity/osquery/osquery.conf | `sha256:896ddad02785e431ab91bd17dc69f2c6bec27ad7011de8a07614d1c722898fb0` |
| telemetry/layer6-integrity/wazuh/ossec-agent.conf | `sha256:a6d17a9ff9796c06e852481dd39ba20557b8d8db9a1d4c31591cd917f32fe19a` |
| telemetry/layer7-visualization/grafana/dashboards/aether-overview.json | `sha256:3dc72acb06b9be3dd5af3d141080f736b0b5f7d45fb6e04a3aab9cda2264228d` |
| telemetry/layer7-visualization/grafana/provisioning/datasources/datasources.yaml | `sha256:27e449c6074fb766f76efac360c5b6aca268580dd753ca4e91c8870b4cad1d7b` |
| temporal_kg/cli.py | `sha256:a1884faf0c98eac7436749a7da7eafcd59d23f3f149c4c3a994ae79c8e8b71cd` |
| temporal_kg/notebook.py | `sha256:14bf4e19e4b74c8508a20eb1ccf0bb74b60249be2f8fc08c58319d5791c7b782` |
| tests/test_notebook.py | `sha256:0cad356044808d80b0b138714333797a231c2dd79a33d3b054b016ae6665098c` |

## Open Items

- **AETH-2026-09-07-0006** — Moved Jaeger OTLP gRPC ingest to port 14317

## Provenance

- Input SHA-256: `b77a9facb10bad37fcf76313bf9b2312eeb1237b3d1f4d63d172e761de7b1c11`
- Events in this notebook: 10
- Total input events: 10
- Compiler: `temporal_kg.notebook` (stdlib-only, deterministic; no wall-clock reads)
