# First-Principles Operating Doctrine

**Project:** AetherOS (lineage: NeuroDiOS / JMK — renamed per principal; existing repo and file names are NOT renamed in this wave)
**Principal:** Jason Jarmacz, Trident Markets
**Document class:** Procedures — binding ground rules
**Status:** Ratified for the telemetry-plane build wave of 2026-09-07
**Authority:** This doctrine operationalizes the outline in `SPEC.md` §"Procedures Doctrine outline". Where this document and `SPEC.md` disagree, `SPEC.md` governs interfaces; this document governs conduct.

---

## 1. Purpose and Scope

### 1.1 Purpose

1.1.1 This doctrine codifies how engineering work on AetherOS is reasoned about, executed, recorded, and verified. It exists so that any competent engineer — human or agent — can replicate any action taken under it exactly, from the written record alone.

1.1.2 It is the procedural complement to `SPEC.md`. `SPEC.md` fixes *what* interfaces exist; this doctrine fixes *how* decisions are reached and *how* work is proven.

### 1.2 Scope

1.2.1 Applies to all AetherOS build waves, all repositories under `trademomentum-llc`, and all agents (orchestrator and subagents) acting on the principal's behalf.

1.2.2 The current wave ("telemetry wave", 2026-09-07) is the first wave executed under this doctrine and serves as its worked example. Section 6 maps every doctrine clause to the artifacts that satisfy it.

### 1.3 Deterministic language convention

1.3.1 Statements of fact in doctrine artifacts must be checkable. Every claim must resolve to a file, a hash, a command, a port, or a cited observation.

1.3.2 Uncertainty is expressed only in probability language (Section 4.6). Words such as "obviously", "clearly", or "it works" are prohibited in ratified documents.

---

## 2. The First-Principles Loop

### 2.0 Overview

2.0.1 All non-trivial engineering decisions pass through a four-stage loop, in order. Skipping a stage is a doctrine violation and must be recorded as such in the action catalog.

| Stage | Name | Output artifact |
|-------|------|-----------------|
| 1 | Identify the problem | Problem statement, free of embedded solutions |
| 2 | Decompose to primitives | List of irreducible elements and their relationships |
| 3 | Challenge assumptions | Classification: physical/logical limit vs. convention |
| 4 | Rebuild from verified truth | Design justified from the deterministic source of truth |

### 2.1 Stage 1 — Identify the problem

2.1.1 State what is being attempted and why, *without naming a technical fix*. A problem statement that contains a product, framework, or protocol name has failed this stage.

2.1.2 Conformance test: the statement must remain true even if every currently proposed tool were removed from consideration.

2.1.3 Worked example (this wave). Non-conforming: "we need Docker observability." Conforming: "the operator cannot currently observe process crashes, configuration drift, integrity violations, performance regressions, or security events inside the emulated AetherOS environment; each such failure must become detectable and explainable from a recorded data trail."

### 2.2 Stage 2 — Decompose to primitives

2.2.1 Reduce the problem to its smallest elements ("quarks") and the relationships between them ("sparks"). A primitive is an element that cannot be decomposed further without leaving the problem domain.

2.2.2 Worked decomposition of the observability problem:

| Primitive ("quark") | Relationship ("spark") |
|---------------------|------------------------|
| A fact occurring in a running system (event, metric sample, span, log line) | Must be *emitted* at the source without corrupting the source |
| A bounded, ordered transport for facts | Must decouple emitter rate from consumer rate (ring buffer) |
| A routing decision by fact type | Metrics, traces, logs, alerts each need a different store |
| A durable, queryable store per fact type | Retention and query semantics differ per type |
| A human-readable rendering | One pane per question the operator asks |
| A record of what was done to the system itself | Feeds the temporal knowledge graph; closes the loop |

2.2.3 The 7-layer telemetry plane in `TELEMETRY_PLANE.md` is the rebuild (Stage 4) over exactly these primitives; each layer owns one primitive or one relationship.

### 2.3 Stage 3 — Challenge assumptions

2.3.1 For every rule, tool, framework, and method in scope, classify each constraint it imposes as either:

- **Physical/logical limit** — a bound that cannot be removed without violating mathematics, physics, or the problem statement (e.g., finite memory, ordering of events in time, hash collision resistance bounds).
- **Convention** — a bound that exists only because a community, vendor, or prior decision chose it (e.g., "containers are how you deploy", "YAML is how you configure").

2.3.2 Conventions are candidates for rejection. Physical/logical limits are design inputs. The classification must be written down; an unwritten challenge is an unchallenged assumption.

2.3.3 Worked table — this wave's challenges, as ratified:

| Rule / Tool / Method | Claim it makes | Physical or logical limit? | Convention? | Verdict and first-principles rationale |
|----------------------|----------------|----------------------------|-------------|-----------------------------------------|
| Docker containerization (hubserve model) | "Isolation and reproducibility require containers" | No. Reproducibility requires pinned inputs and deterministic bring-up, not namespaces | Yes — industry deployment fashion of the 2015–2025 period | **Rejected for the emulated environment.** The target is a QEMU guest; adding a container runtime inside a guest duplicates an isolation layer the hypervisor already provides, adds an unpinned moving part (daemon, image registry pulls at runtime — forbidden by SPEC determinism clause), and obscures process-level truth that Layer 6 must observe. hubserve is retained for config mining only; no Docker artifacts may be introduced (SPEC.md, Deterministic source of truth). |
| Ring-buffer IPC (SEB ring, 64KB, shared) | "Emitters and consumers should be decoupled by a bounded ordered buffer" | Yes — bounded memory and producer/consumer rate decoupling are physical facts; a 64KB bound makes the memory cost a compile-time constant | Partially — the *choice* of shared-memory ring vs. socket is conventional, but boundedness is not | **Accepted.** It satisfies the Stage-2 primitive directly: payload ≤1024B, timestamp in ns, explicit drop counter (`dropped`) so backpressure is observable rather than silent. Overflow behavior is tested, not assumed (`tests/test_seb.c`: N=1000 assert `dropped==0`; overflow case asserts `dropped>0`). |
| systemd (guest service management) | "Services need a supervisor for start order and restart" | Partially — start ordering and restart-on-failure are logical requirements of a multi-process stack | Yes — the specific implementation (systemd vs. runit vs. foreground) is conventional | **Conditionally accepted.** systemd units are permitted in the guest for production-like runs; foreground execution is the dev path (SPEC.md, Emulated-environment plumbing). Accepted because it is native to the guest OS image, adds no network dependency, and its unit files are committable text — satisfying determinism. Rejected features: anything pulling from the network at boot. |
| YAML configuration | "Config must be YAML" | No | Yes — pure convention | **Accepted as convention, not as necessity.** Accepted because every selected component (otelcol, Prometheus, Loki, Grafana provisioning, osquery) consumes YAML natively; introducing a translation layer would add failure modes without adding truth. All YAML is committed and must parse under `yaml.safe_load` (SPEC.md, Verification gate 3). |
| Centralized collectors (hubserve pattern: one observability hub outside the system) | "Telemetry backends live on a separate central host" | No | Yes — a deployment topology convention | **Rejected for this wave; guest-native collectors accepted.** In the emulated environment the guest *is* the system boundary; an external hub reintroduces network dependencies the determinism clause forbids ("no network calls at runtime except inter-process localhost"). All collectors (otelcol, Prometheus, Jaeger, Loki, Wazuh, Grafana) run as native binaries inside the guest; the host reaches them only through QEMU user-mode hostfwd on the five pinned ports (3000, 9090, 16686, 3100, 9464). |

2.3.4 Any future challenge that overturns a ratified verdict must be recorded as an action event with `action: "decided"` and a rationale referencing this section.

### 2.4 Stage 4 — Rebuild from verified deterministic source of truth

2.4.1 The only permissible foundation for a rebuild is the **deterministic source of truth**: the verified, committed state of the repositories enumerated in `SPEC.md` §"Deterministic source of truth". Chat transcripts, recollections, and prior-session summaries are *leads*, never *truth*.

2.4.2 A design element is "verified" when it traces to: (a) a committed file at a recorded sha256; (b) an interface contract in `SPEC.md`; or (c) a recon observation recorded with timestamp and method (SPEC.md: "verified via GitHub recon 2026-09-07T19:2xZ").

2.4.3 The rebuild is expressed spec-first (Section 4.1) before any implementation begins.

---

## 3. Core Goals

### 3.0 Goal set

3.0.1 Six goals govern all work. They are ordered; in a conflict, the earlier goal wins, and the conflict itself is recorded as a `decided` action event.

| # | Goal | Measurable interpretation (how we know it is true) |
|---|------|-----------------------------------------------------|
| 1 | **Usefulness** | Every artifact answers a named operator question. Test: for each deliverable, the doctrine or spec cites the failure class or decision it serves. This wave: each telemetry layer maps to at least one failure class in `TELEMETRY_PLANE.md` §6. An artifact serving no named question is deleted or deferred. |
| 2 | **Reliability** | Behavior is reproducible from the record. Test: bring-up follows `telemetry/BRINGUP.md` exactly, on a clean guest, with pinned versions from `telemetry/VERSIONS.md`, and every per-layer verification probe passes. C code compiles under `gcc -Wall -Wextra -O2 -std=c11` with zero warnings; `test_seb` passes (SPEC.md, Verification gates 1–2). |
| 3 | **Understanding** | A reader can reconstruct *why* each decision was made without interviewing the author. Test: every non-trivial action exists as an action event with `rationale` and numbered `replication_steps`; every event compiles into a dated notebook by the notebook compiler. |
| 4 | **Adaptability** | A layer can be replaced without redesigning the plane. Test: layer boundaries are the contracts in `SPEC.md` §"Layer Contracts"; a replacement candidate needs only to satisfy the contract row (e.g., any OTLP-speaking collector can substitute at L2). Evidence of adaptability this wave: hubserve configs were mined without inheriting hubserve's deployment model. |
| 5 | **Sustainability** | The system can be maintained by a small team indefinitely. Test: stdlib-only where a new repo is created (notebook compiler is stdlib-only per SPEC.md); no runtime network calls except inter-process localhost; every added dependency carries a pinned version and a sha256 of its upstream tarball. |
| 6 | **Simplicity** | The fewest moving parts that satisfy goals 1–5. Test: the challenge-assumptions table (2.3.3) shows each admitted component survived an explicit necessity challenge; Docker was rejected on exactly this ground. Count of hostfwd ports is five; each is justified in `TELEMETRY_PLANE.md` §5.2. |

3.0.2 Goal measurements are reported per wave in the compliance table (Section 6).

---

## 4. Engineering Procedures

### 4.1 Spec-first

4.1.1 `SPEC.md` is written and ratified *before* implementation begins on any wave. Interface contracts in it are sacred; subagents implement to them exactly.

4.1.2 Implementation may not introduce interfaces not present in `SPEC.md`. A discovered gap halts the affected work item and is escalated as a `decided` action event proposing a SPEC amendment — never patched silently.

4.1.3 This wave's artifacts implement `SPEC.md` dated 2026-09-07; the Layer 1 C API (`seb.h`, `probe.h`) is FIXED and signatures may not change.

### 4.2 Deterministic builds

4.2.1 Every third-party component is pinned by exact version in `telemetry/VERSIONS.md`, accompanied by the sha256 of its upstream tarball.

4.2.2 Every configuration file is committed to the repository; runtime-generated config is prohibited.

4.2.3 Bring-up is reproducible: `telemetry/BRINGUP.md` specifies fetch + sha256 verify, build/install steps, the fixed run order (Loki → Jaeger → Prometheus → otelcol → Wazuh → Grafana), a QEMU invocation example using user-mode networking hostfwd, per-layer verification probes, and teardown.

4.2.4 No network calls at runtime except inter-process localhost. A component that phones home is a doctrine violation.

### 4.3 Hash-everything provenance

4.3.1 Every produced file receives a sha256. Provenance is recorded at two granularities: per-file hashes and per-category manifests.

4.3.2 This wave's hash manifest is committed as `telemetry/HASHES.sha256` in the engine repo (SPEC.md, Verification gate 5) and reported back to the orchestrator.

4.3.3 Action events carry their own `hashes` map (`{"<path>": "sha256:..."}`) so the knowledge graph can later prove which bytes a decision referred to.

### 4.4 Action cataloging mandate

4.4.1 Every action taken under this doctrine is recorded as one temporal action event, one JSON object per line (JSONL), conforming to the schema fixed in `SPEC.md` §"Action Event Schema": `action_id`, `recorded_at`, `valid_from`, `actor`, `layer`, `action`, `target`, `summary`, `rationale`, `inputs`, `outputs`, `hashes`, `replication_steps`, `supersedes`, `source`.

4.4.2 **Replication sufficiency test:** `replication_steps` must be descriptive enough that an engineer with the pinned toolchain can reproduce the action *exactly* — same commands, same inputs, same expected hashes. "Updated the config" fails; "Inserted receiver block `otlp: protocols: grpc: endpoint: 0.0.0.0:4317` at line N of `telemetry/layer2-collection/otelcol/otelcol-config.yaml`" passes.

4.4.3 Temporal semantics follow the knowledge-graph engine: `[valid_from, valid_to)` edges, non-destructive invalidation via `supersedes`. Nothing is ever deleted from the record; it is superseded.

4.4.4 This wave's catalog is `engine/knowledge/actions/2026-09-07-telemetry-plane.jsonl`. Events are compiled to dated markdown notebooks (`knowledge/notebooks/`) by the notebook compiler in the `temporal-knowledge-graph` repo (`temporal_kg notebook`); compilation is deterministic — ordering by `recorded_at` then `action_id`, `generated_at` taken from max `recorded_at`, never wall clock.

### 4.5 Consensus before destructive actions

4.5.1 A destructive action is any action that deletes data, invalidates an interface, force-pushes history, removes a service, or supersedes a ratified decision.

4.5.2 No destructive action executes unilaterally. It requires: (a) a written proposal with rationale and blast radius; (b) explicit approval from the principal or the orchestrator acting on recorded principal authority; (c) a `decided` action event *before* execution and a second event after execution carrying the resulting hashes.

4.5.3 This wave contained one destructive-class decision executed correctly: the deprecation of the hubserve Docker deployment model. It is recorded in `SPEC.md` as DEPRECATED, its configs preserved for mining — the record was superseded, not erased.

### 4.6 Honest uncertainty

4.6.1 Uncertainty is expressed only in probability language: explicit likelihoods, confidence qualifiers, or enumerated alternatives. Never diagnose beyond the evidence.

4.6.2 Example — conforming: "The otelcol-to-Jaeger path is unverified on this guest build; probability of port contention at :4317 is high because both contracts name it (see TELEMETRY_PLANE.md §2.2, Open Items)." Non-conforming: "Jaeger will conflict" or "it will be fine."

4.6.3 An agent that does not know says so and states what *would* resolve the uncertainty (a command, a probe, a recon step). Invented certainty is a doctrine violation worse than admitted ignorance.

### 4.7 No unilateral renames

4.7.1 Identity is data. Renaming repos, files, branches, or services is a destructive-class action under 4.5 and requires consensus.

4.7.2 Lineage is recorded: NeuroDiOS / JMK → **AetherOS**, renamed per principal. In this wave, existing repo and file names remain untouched; only *new* artifacts carry AetherOS naming (SPEC.md, header note). The microkernel path `nnos/neurodios/jasterish-microkernel` stays as-is.

---

## 5. Session Protocol

### 5.1 Recon-before-build

5.1.1 Every session opens with recon against the deterministic source of truth: read the actual committed state of the target repositories before writing anything.

5.1.2 Chat memory is never equated with committed state. The prior session for this wave maxed out its context; per plan.md Stage 0, nothing it described was assumed committed until verified by GitHub recon (2026-09-07T19:2xZ), which located: `engine` (public, default `master`), `temporal-knowledge-graph` (private, default `main`), `morehlex-deterministic` (created 2026-09-07), and `hubserve` (deprecated).

5.1.3 Recon output is itself an action event with `layer: "recon"`, recording method and timestamp so its freshness is later auditable.

### 5.2 Drift check — before and after

5.2.1 The engine repo carries `nnos/scripts/binary_drift_check.sh`. Where a drift check exists, it is run (or inspected, if execution is infeasible in the working environment) **before** modifications and **after** modifications.

5.2.2 The before-run establishes the baseline the session inherited; the after-run proves the session changed only what its action catalog claims. Any unexplained delta is a doctrine violation and halts integration.

5.2.3 Runtime drift between checks is additionally surfaced by Layer 6 of the telemetry plane (Wazuh FIM + osquery), which emits integrity alerts as `SEB_ALERT` events into the same pipeline — the doctrine's drift rule and the plane's integrity layer are the same idea at two timescales.

### 5.3 Stage gates

5.3.1 Work proceeds through gates; a gate does not open until the prior gate's evidence exists.

| Gate | Entry condition | Exit evidence |
|------|-----------------|---------------|
| G0 Recon | Session start | Deterministic source of truth enumerated with timestamps (5.1) |
| G1 Doctrine | G0 passed | This document ratified |
| G2 Spec | G1 passed | `SPEC.md` ratified; interface contracts fixed |
| G3 Build | G2 passed | Artifacts per deliverable map; zero-warning C compile; YAML safe_load pass; `test_seb` pass; unittest pass for notebook compiler |
| G4 Provenance | G3 passed | `telemetry/HASHES.sha256` committed; action catalog JSONL complete for the wave |
| G5 Integration | G4 passed | Drift check after-run clean or explained; branch pushed; notebooks compiled from the wave's own action catalog (self-hosting the procedure) |

5.3.2 This wave's ordering — doctrine, then telemetry plane, then knowledge graph, then integration, and only afterward resumption of compiler/tokenization work — is the stage-gate rule applied to the roadmap in plan.md.

---

## 6. Compliance Table — Doctrine Clause ↔ Wave Artifact

| Doctrine clause | Requirement | Satisfying artifact(s) this wave | Evidence location |
|-----------------|-------------|----------------------------------|-------------------|
| 2.1 Identify problem | Solution-free problem statement | §2.1.3 (this document); plan.md Mission Context | This file |
| 2.2 Decompose to primitives | Quarks/sparks table | §2.2.2; realized as the 7-layer plane | This file; `TELEMETRY_PLANE.md` §1 |
| 2.3 Challenge assumptions | Written physical-vs-convention classification | §2.3.3 (Docker, ring buffer, systemd, YAML, centralized vs. guest-native) | This file; summarized in `TELEMETRY_PLANE.md` §5.3 |
| 2.4 Rebuild from verified truth | Design traces to recon-verified repos | SPEC.md §"Deterministic source of truth" (GitHub recon 2026-09-07T19:2xZ) | `SPEC.md` |
| 3 Core goals | Measurable interpretation per goal | §3.0.1 table; layer↔failure-class mapping | This file; `TELEMETRY_PLANE.md` §6 |
| 4.1 Spec-first | SPEC before implementation | `SPEC.md` (2026-09-07) ratified ahead of build; fixed Layer 1 API | `SPEC.md` §"Layer 1 API (FIXED)" |
| 4.2 Deterministic builds | Pinned versions, committed configs, reproducible bring-up, localhost-only runtime | `telemetry/VERSIONS.md`; committed YAML configs; `telemetry/BRINGUP.md` with run order Loki→Jaeger→Prometheus→otelcol→Wazuh→Grafana | engine repo `telemetry/` |
| 4.3 Hash-everything | Per-file sha256 + category manifest | `telemetry/HASHES.sha256` (Verification gate 5) | engine repo `telemetry/` |
| 4.4 Action cataloging | Schema-conformant JSONL, replication-sufficient | `engine/knowledge/actions/2026-09-07-telemetry-plane.jsonl`; schema in `temporal-knowledge-graph/schemas/action_event.schema.json`; notebooks via `temporal_kg notebook` | engine `knowledge/`; TKG repo |
| 4.5 Consensus before destructive | Recorded, approved, superseded-not-erased | hubserve deprecation recorded in SPEC.md; non-destructive `.observability/agent.yml` extension mandated ("EXTEND … non-destructive") | `SPEC.md` Deliverable Map |
| 4.6 Honest uncertainty | Probability language only | §4.6; Open Items in `TELEMETRY_PLANE.md` §2.2 and §5.4 | Both documents |
| 4.7 No unilateral renames | Lineage recorded; existing names untouched | SPEC.md header note; §4.7.2 | `SPEC.md`; this file |
| 5.1 Recon-before-build | Verified repo truth precedes writes | plan.md Stage 0 executed; four repos enumerated with default branches | `SPEC.md`; plan.md |
| 5.2 Drift check before/after | Baseline and post-run | `nnos/scripts/binary_drift_check.sh` inspected/run per Stage 0 and Stage 4 | engine repo `nnos/scripts/` |
| 5.3 Stage gates | Ordered gates with evidence | §5.3.1; wave ordering doctrine→plane→KG→integration | This file; plan.md Stages 1–4 |

---

## 7. Amendment

7.1 Amendments to this doctrine are destructive-class actions under 4.5: proposal, consensus, `decided` event, supersession — never deletion. The doctrine's history lives in the knowledge graph, not in anyone's memory.
