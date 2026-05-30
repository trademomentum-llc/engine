# Jason's Living System Architecture
# Tri-Plane Heterogeneous Compute Fabric (TP-HCF)
# Software Requirements + Design Specification

| Field | Value |
|---|---|
| Document ID | LSA-SPEC-002 |
| Version | 2.0.0 |
| Date | 2026-02-22 |
| Status | APPROVED |
| Classification | CONFIDENTIAL |
| Encoding | UTF-8 without BOM |

---

## 1. Purpose

This specification defines a compiled, deterministic system that formalizes
Jason's empirically validated decision architecture across three domains
(forex execution, neurodivergent support, context infrastructure) deployed
across a three-node heterogeneous compute fabric.

Core Design Law (Jason's):

    Most distributed systems fail because people try to make every node
    do everything. GPU nodes do math. CPU nodes do orchestration.
    Edge nodes do latency-sensitive tasks.

    Modern AI calls it compute. Old timers called it automated workflow.

This spec routes by workload topology, not by language.

---

## 2. System Classification

### 2.1 Fabric Type

Heterogeneous Distributed Compute Fabric (HDCF-3)

System Name: Tri-Plane Heterogeneous Compute Fabric (TP-HCF)

### 2.2 Execution Classes

| Class | Profile | Characteristics |
|-------|---------|-----------------|
| E1 | Deterministic Control | Branch-heavy, I/O bound, transactional, low parallelism |
| E2 | Parallel Numerical | Tensor ops, vector similarity, batch inference, embeddings |
| E3 | Real-Time Reactive | Sensor input, vision pipelines, edge inference, latency loops |

### 2.3 Execution Routing Law

    If workload in E1         -> Route to NUC (DCN)
    If workload in E2, small  -> Route to M1  (HCN)
    If workload in E2, large  -> Route to Orin (EPN) if CUDA-optimized
    If workload in E3         -> Route to Orin (EPN)

Threshold for E2 routing determined by: tensor size, batch size,
latency tolerance, memory footprint.

### 2.4 Control Law

All governance and persistence remain on the deterministic plane (NUC).

### 2.5 Anti-Patterns (DO NOT)

- Host primary DB on Orin
- Run heavy CUDA jobs on M1 expecting parity
- Treat NUC as ML node
- Move large tensors unnecessarily across network
- Make every node run every daemon

---

## 3. Node Capability Matrix

### 3.1 Jetson Orin Nano -- Edge Parallel Node (EPN)

| Property | Value |
|----------|-------|
| Architecture | ARM64 + Ampere CUDA GPU |
| Memory Model | Shared SoC (GPU + CPU) |
| OS | JetPack 6.x / Ubuntu 22.04 |
| Compiler | GCC 11+ (aarch64) |
| GPU Path | Python -> PyTorch CUDA -> cuDNN/TensorRT -> GPU SMs |
| Execution Classes | E2 (Parallel), E3 (Real-Time) |
| Optimized For | ONNX/TensorRT models, vision inference, sensor pipelines |
| NOT For | DB hosting, heavy transactional control |
| Constraint | Thermal envelope + memory ceiling |

Daemons deployed: lsa_boot, lsa_state_monitor, lsa_ethernet_sync

### 3.2 MacBook Pro M1 -- Hybrid Cognitive Node (HCN)

| Property | Value |
|----------|-------|
| Architecture | ARM64 + Apple GPU (Metal) |
| Memory Model | Unified Memory Architecture (no PCIe penalty) |
| OS | macOS 14+ |
| Compiler | Apple Clang 15+ (aarch64) |
| GPU Path | Python -> MLX/PyTorch MPS -> Metal -> Apple GPU |
| Execution Classes | E2 (Medium), Orchestration |
| Optimized For | Embeddings, agent orchestration, model experiments, drift scoring |
| NOT For | CUDA-dependent workloads |
| Advantage | No PCIe memory copy penalty. High bandwidth. Dev ergonomics. |

Daemons deployed: lsa_boot, lsa_convergence_bond, lsa_profile_refiner,
lsa_comm_bridge, lsa_ethernet_sync

### 3.3 ASUS NUC 15 Pro -- Deterministic Control Node (DCN)

| Property | Value |
|----------|-------|
| Architecture | x86_64 (Intel), CPU only |
| Memory Model | Standard DDR5 |
| OS | Ubuntu 24.04 |
| Compiler | GCC 13+ (x86_64) |
| GPU Path | None |
| Execution Classes | E1 (Deterministic Control) |
| Optimized For | PostgreSQL, Vector DB, API gateway, scheduler, auth, logging |
| NOT For | ML inference, tensor operations |
| Role | Source of truth. Governance. Persistence. |

Daemons deployed: lsa_boot (master), lsa_task_manager, lsa_context_gate,
lsa_drift_detector, lsa_ethernet_sync

---

## 4. Data Flow Model

    NUC (source of truth: task state, profile config, Origin Vault)
      |
      v
    M1 (precompute: embeddings, convergence, MirrorLock, profiles)
      |
      v
    Orin (real-time: sensor fusion, intervention tier, edge inference)
      |
      v
    Results back to NUC (persist, drift analysis, governance)

    Pixel 9 Pro -> TCP poll to NUC port 20047 -> SharedState snapshot

### 4.1 State Ownership

| State Type | Owner | Policy |
|------------|-------|--------|
| Persistent State | NUC | Source of truth. Never overwritten by remote. |
| Ephemeral Compute Buffers | Local node | Discarded after use. |
| Model Artifacts | Replicated M1 + Orin | Versioned. NUC stores registry. |
| Edge Cache | Orin local | Sensor readings. Newest timestamp wins on sync. |
| Profile Data | M1 primary | Newest profile_update timestamp wins. |

### 4.2 Merge Strategy (Ethernet Sync)

- Sensor fields (HRV, noise, light, intervention_tier): newest timestamp wins
- Task fields (active_tasks, context_switches, breathing_room): NEVER overwrite local
- Profile fields (profile_id, confidence, thresholds): newest profile_update wins
- Governance fields (Origin Vault, drift status): NUC authoritative

---

## 5. Core Architectural Principle: Breathing Room

Jason's universal constant:

    Never fill to capacity. Always preserve buffer for shock absorption.

| Scale | Application | Expression |
|-------|-------------|------------|
| Node | 3 nodes, 3 roles. Not 3 nodes doing everything. | Workload routing |
| Position | 4 pennies not 50 at 200:1 margin | 92% capacity reserved |
| Task | max_concurrent_tasks capped at profile limit | Executive function protection |
| GPU | Run 4 CUDA models on Orin, not 50 | Thermal + memory headroom |
| Memory | M1 unified memory: experiment without PCIe tax | No copy penalty |
| Persistence | NUC: source of truth, never overloaded with inference | Deterministic anchor |

Mathematical anchor:

    phi_inverse = 0.618033988749895
    complementarity_zone = 1.0 - phi_inverse = 0.382
    breathing_room_threshold = complementarity_zone

---

## 6. Functional Requirements

### 6.1 Origin Vault (GAP-001) -- Runs on NUC

FR-6.1.1: The system SHALL store decision rules with entry/exit conditions,
proxy tier (COLD/WARM/HOT/ORIGIN), and confidence score on the NUC.

FR-6.1.2: The system SHALL compute calibration omega targeting phi_inverse.

FR-6.1.3: The system SHALL support break-glass reconstitution returning
rules sorted by tier then confidence descending.

FR-6.1.4: Origin Vault data SHALL be persisted to PostgreSQL on NUC.

### 6.2 MirrorLock Consistency (GAP-002) -- Runs on M1

FR-6.2.1: The system SHALL log trade records with stated (symbolic) and
actual (subsymbolic) values for entry MA, stochastic, and trend.

FR-6.2.2: The system SHALL classify profitable mismatches as
IMPLICIT KNOWLEDGE (signal, not error).

FR-6.2.3: MirrorLock runs on M1 (pattern analysis = E2 workload).
Results sync to NUC for persistence.

### 6.3 Convergence Bond (GAP-003) -- Runs on M1

FR-6.3.1: The system SHALL compute domain overlap using cosine similarity.

FR-6.3.2: OPTIMAL complementarity when overlap < 0.382.

FR-6.3.3: Leverage multiplier: 2193x when overlap < 0.382.

FR-6.3.4: Bridging principle identification via combined weight analysis.

### 6.4 Drift Detection (GAP-004) -- Runs on NUC

FR-6.4.1: The system SHALL maintain rolling window of recent returns.

FR-6.4.2: Alert when drift > 0.618 (phi_inverse threshold).

FR-6.4.3: Drift analysis queries PostgreSQL on NUC. DB local to compute.

### 6.5 State Monitoring -- Runs on Orin

FR-6.5.1: The system SHALL sample physiological signals at 4 Hz on Orin.

FR-6.5.2: Sensory load computation:
sensory_load = (noise * sensitivity * 0.4) + (light * sensitivity * 0.3)
             + (notifications * 0.3)

FR-6.5.3: Intervention tier published within 50ms of threshold crossing.

FR-6.5.4: Future: CUDA vision pipeline for environment sensing on Orin.

### 6.6 Task Management -- Runs on NUC

FR-6.6.1: Max concurrent tasks enforced per profile (range 1-4).

FR-6.6.2: Breathing room computed: 1.0 - (margin_used / capital).

FR-6.6.3: Task acceptance/deferral is governance logic = E1 = NUC.

### 6.7 Context Gate -- Runs on NUC

FR-6.7.1: Hourly context switch limits enforced per profile (range 2-5).

FR-6.7.2: Hyperfocus protection/break decisions.

FR-6.7.3: Manual execution gate: AI never executes trades.

FR-6.7.4: Control-plane logic. Deterministic. Runs on NUC.

### 6.8 Profile Refinement -- Runs on M1

FR-6.8.1: Daily 03:00 batch inference on 30-day logs.

FR-6.8.2: MLX/Metal acceleration optional for embedding computation.

FR-6.8.3: Profile updates sync to NUC + Orin via ethernet_sync.

### 6.9 Anti-Masking Detection -- Runs on M1

FR-6.9.1: Flags masking when capacity < 0.4 AND demand > 0.7.

FR-6.9.2: Market tier classification: UNDERLEVERAGED/MATCHED/MASKING.

FR-6.9.3: Pattern analysis workload = E2 = M1.

### 6.10 Ethernet Synchronization -- Runs on ALL

FR-6.10.1: UDP multicast 239.73.78.69:20046 every 100ms.

FR-6.10.2: AES-256-GCM encryption with pre-shared key.

FR-6.10.3: Each node broadcasts only its own SHM fields.

FR-6.10.4: Each node merges remote fields per merge strategy (Section 4.2).

FR-6.10.5: TCP polling on NUC port 20047 for mobile device queries.

### 6.11 Daemon Supervision -- Per-Node

FR-6.11.1: Each node runs its own boot_daemon that forks only local children.

FR-6.11.2: NUC boot_daemon is master control (additional health monitoring).

FR-6.11.3: boot_daemon on each node SHALL restart any crashed child within 2 seconds.

FR-6.11.4: boot_daemon SHALL log all child exits with PID, exit code, and restart timestamp.

FR-6.11.5: On SIGTERM, boot_daemon SHALL:
- Send SIGTERM to all children on that node.
- Wait up to 10 seconds for clean shutdown.
- Force-kill remaining processes if needed.
- Clean up shared memory segments and exit with code 0.

---

## 7. Non-Functional Requirements

### 7.1 Performance

NFR-7.1.1: Origin Vault queries on NUC SHALL return in < 50 ms for 95% of reads under normal load.

NFR-7.1.2: MirrorLock and Convergence Bond batch runs on M1 SHALL complete within their configured batch window (default 15 minutes).

NFR-7.1.3: Drift detection queries on NUC SHALL complete in < 250 ms for typical window sizes.

NFR-7.1.4: Orin state_monitor SHALL maintain 4 Hz sampling and publish intervention tier updates within 50 ms of threshold crossing.

NFR-7.1.5: Ethernet sync end-to-end propagation latency across nodes SHALL be < 150 ms on 1 Gbps LAN for 99% of packets.

### 7.2 Reliability

NFR-7.2.1: Each node SHALL achieve 99.5% process uptime for critical daemons over a rolling 30-day window.

NFR-7.2.2: A single daemon crash SHALL NOT corrupt shared memory; magic and version SHALL be validated on every open.

NFR-7.2.3: Network packet loss ≤ 10% SHALL NOT result in state divergence greater than 500 ms.

NFR-7.2.4: System SHALL survive cold boot with corrupt log files by regenerating them or skipping over bad segments.

### 7.3 Security

NFR-7.3.1: All inter-node traffic (multicast and TCP polling) SHALL use AES-256-GCM with a pre-shared key stored only on disk in `/etc/lsa/sync.key` (0600 permissions).

NFR-7.3.2: NUC SHALL run a local-only PostgreSQL instance; no direct Internet access to DB.

NFR-7.3.3: Model artifact distribution SHALL be signed (future) or checksum-verified; NUC stores checksums.

NFR-7.3.4: No private keys or exchange API secrets SHALL be stored on Orin; NUC holds all secrets.

### 7.4 Maintainability

NFR-7.4.1: All C++ source SHALL compile warning-free at `-Wall -Wextra -Wpedantic` on NUC and Orin.

NFR-7.4.2: Platform-specific code SHALL be isolated behind small adapter layers (for example CUDA vs Metal vs CPU-only).

NFR-7.4.3: Configuration files SHALL be simple text formats (INI, TOML, or YAML) and version-controlled.

NFR-7.4.4: All daemons SHALL log structured JSON events to node-local logs (for example `/var/log/lsa/*.jsonl`).

### 7.5 Portability

NFR-7.5.1: Core control-plane code (Origin Vault, Drift Detection, Task Manager, Context Gate) SHALL be portable between x86_64 and ARM64.

NFR-7.5.2: M1-specific acceleration (MLX, Metal) SHALL be optional; CPU-only fallbacks SHALL exist for development.

NFR-7.5.3: Orin-specific CUDA paths SHALL be guarded by runtime checks; system degrades gracefully to CPU-only if CUDA unavailable.

---

## 8. Daemon Inventory by Node

### 8.1 Node: Jetson Orin Nano (EPN)

Daemons:
- `lsa_boot_epn`
- `lsa_state_monitor`
- `lsa_ethernet_sync_epn`

Responsibilities:
- Real-time physiological and environment sensing.
- Local computation of sensory_load and intervention tiers.
- Multicast of edge state to NUC and M1.

### 8.2 Node: MacBook Pro M1 (HCN)

Daemons:
- `lsa_boot_hcn`
- `lsa_convergence_bond`
- `lsa_profile_refiner`
- `lsa_comm_bridge`
- `lsa_ethernet_sync_hcn`

Responsibilities:
- Embeddings and pattern analysis (MirrorLock, Convergence Bond).
- Profile refinement ML runs.
- Communication decisions and anti-masking logic.
- Relaying refined profile state to NUC and Orin.

### 8.3 Node: ASUS NUC 15 Pro (DCN)

Daemons:
- `lsa_boot_dcn`
- `lsa_task_manager`
- `lsa_context_gate`
- `lsa_drift_detector`
- `lsa_ethernet_sync_dcn`

Responsibilities:
- Governance and persistence (Origin Vault, Drift).
- Task intake and scheduling.
- Context gating and execution guardrails.
- Authoritative view of system health and drift.

---

## 9. Shared State and Sync Mapping

### 9.1 Shared State Fields (Conceptual)

Core fields shared across nodes (conceptual, not exact struct):

- Position and performance:
  - `recent_returns[]`
  - `drift_score`
- Neuro/system state:
  - `sensory_load`
  - `intervention_tier`
  - `masking_flag`
- Profile state:
  - `active_profile_id`
  - `profile_confidence`
  - `traits[...]`
- Governance:
  - `origin_vault_version`
  - `drift_alert_active`

### 9.2 Which Node Owns Which Fields

- NUC:
  - Owns `origin_vault_version`, `drift_score`, governance flags.
- M1:
  - Owns `active_profile_id`, `profile_confidence`, `traits[...]`.
- Orin:
  - Owns `sensory_load`, `intervention_tier`, edge metrics.

Merge strategy:
- If field is owned by node X:
  - Node X writes it.
  - Other nodes only overwrite local copies when receiving from X.

---

## 10. Operational Scenarios

### 10.1 Normal Trading Day

1. NUC boots and starts:
   - Origin Vault (DB ready).
   - Drift Detector.
   - Task Manager and Context Gate.
2. M1 boots:
   - Connects to NUC.
   - Runs Convergence Bond and MirrorLock analyses in background.
   - Refines profiles nightly at 03:00.
3. Orin boots:
   - Starts State Monitor and begins sampling at 4 Hz.
   - Computes intervention_tier and multicasts to NUC and M1.
4. NUC Task Manager:
   - Accepts tasks if:
     - Margin within breathing_room threshold.
     - Context Gate allows.
   - Defers tasks when overload or masking detected.
5. At end of day:
   - Drift Detector computes drift over rolling window.
   - Alerts if exceed thresholds.
   - Origin Vault updated with new rules and confidence scores.

### 10.2 Market Shock Scenario

1. Sudden volatility spike:
   - Recent returns window deviates beyond drift threshold.
   - Drift Detector on NUC raises alert.
2. NUC Context Gate:
   - Tightens acceptance rules (for example no new high-risk positions).
   - Enforces manual-only execution path.
3. M1:
   - Recomputes Convergence Bond to re-evaluate domain overlap.
4. Orin:
   - Continues monitoring Jason’s state.
   - If overload + masking risk detected:
     - Raises intervention tier.
     - NUC defers new tasks until state recovers.

### 10.3 Neuro Overload Scenario (No Market Shock)

1. Jason’s sensory_load rises on Orin (no market drift).
2. Orin State Monitor:
   - Escalates intervention_tier.
   - Multicasts to NUC.
3. NUC Task Manager and Context Gate:
   - Defer non-critical tasks.
   - Enforce breaks.
4. M1 Comm Bridge:
   - Recommends NEGOTIATE_SCOPE or DECLINE for new requests.
5. Once Jason’s state falls below threshold:
   - Intervention tier returns to baseline.
   - Task acceptance gradually resumes.

---

## 11. Traceability (High-Level)

| Gap ID | Node | Section | Requirement IDs |
|--------|------|---------|-----------------|
| GAP-001 Origin Vault | NUC | 6.1 | FR-6.1.1, FR-6.1.2, FR-6.1.3, FR-6.1.4 |
| GAP-002 MirrorLock | M1 | 6.2 | FR-6.2.1, FR-6.2.2, FR-6.2.3 |
| GAP-003 Convergence Bond | M1 | 6.3 | FR-6.3.1, FR-6.3.2, FR-6.3.3, FR-6.3.4 |
| GAP-004 Drift Detection | NUC | 6.4 | FR-6.4.1, FR-6.4.2, FR-6.4.3 |
| State Monitoring | Orin | 6.5 | FR-6.5.1, FR-6.5.2, FR-6.5.3, FR-6.5.4 |
| Task Management | NUC | 6.6 | FR-6.6.1, FR-6.6.2, FR-6.6.3 |
| Context Gate | NUC | 6.7 | FR-6.7.1, FR-6.7.2, FR-6.7.3, FR-6.7.4 |
| Profile Refinement | M1 | 6.8 | FR-6.8.1, FR-6.8.2, FR-6.8.3 |
| Anti-Masking | M1 | 6.9 | FR-6.9.1, FR-6.9.2, FR-6.9.3 |
| Ethernet Sync | All | 6.10 | FR-6.10.1, FR-6.10.2, FR-6.10.3, FR-6.10.4, FR-6.10.5 |
| Daemon Supervision | All | 6.11 | FR-6.11.1, FR-6.11.2, FR-6.11.3, FR-6.11.4, FR-6.11.5 |