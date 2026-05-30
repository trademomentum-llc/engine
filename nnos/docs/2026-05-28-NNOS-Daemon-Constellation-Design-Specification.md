# Design Specification: NNOS Daemon Constellation + Jasterish Validation/Morph Port

**Document ID:** NNOS-DS-001  
**Version:** 1.0.0  
**Date:** 2026-05-28  
**Predecessor:** NNOS-REQ-001

---

## 1. Architectural Style

NNOS is a **constellation** of small, compiled daemons coordinated through encrypted shared-state (not a single monolithic process). This mirrors neuroanatomical specialization (different brain regions for different functions) while maintaining global coherence via the morphogenetic layer.

Core principles:
- Deterministic execution (fixed input → identical output).
- Explicit budgets (task limits, context switches, sensory load).
- Selective permeability (BBB-style access control between daemons and external world).
- Self-healing (morphogenetic repair cycles).

---

## 2. Baseline Daemon Responsibilities (Design Level)

**lsa_boot_dcn / Device Coordination Node**
- Manages Ethernet/WiFi 6 sync of encrypted state blobs.
- Origin-vault provenance tracking.
- Conflict resolution for concurrent device updates.

**lsa_boot_hcn / Human Coordination Node**
- Real-time physiology ingestion (heart rate, HRV, etc.).
- Burnout risk model + masking detector (using 180 NDPL rules).
- Enforces task limits and context-switch budgets.

**lsa_boot_epn / External Perception Node**
- Sensory/environmental load measurement.
- Feeds into shared-state for HCN consumption.

**SystemIntegrityDaemon**
- File integrity, drift detection, binary attestation.
- Ties into PQC signatures where applicable.

**ThreatIntelligenceManager**
- Local anomaly detection (masking, overcommitment patterns).
- Raises events to MorphogeneticMaintainer.

**MorphogeneticMaintainer**
- Scheduled (daily 4 AM baseline) repair/optimization cycle.
- Detect → Remove damaged → Restore connectivity → Optimize → Adapt topology.
- Jasterish port target for core logic.

---

## 3. Jasterish Port Design (Validation + Morph Layers)

Validation layer:
- Acts as runtime guard: before any shared-state transition or daemon action, validate against capability rules + NDPL patterns.
- Not a compiler pass (avoids validating every IR instruction).

Morphogenetic layer:
- Implements glial repair metaphor in code.
- Uses deterministic pattern matching over state history.
- Emits repair plans that are themselves JStar-executable where possible.

Integration: Both layers link against JStar runtime emitted from the apps/ compiler (see JSTAR triad).

---

## 4. Shared-State Schema (High-Level Design)

Versioned, append-only log + current snapshot.
Fields include (minimal baseline):
- physiology_snapshot (timestamped)
- task_budget_remaining
- context_switch_count (windowed)
- threat_score
- morph_cycle_id
- origin_vault_ref

Schema evolution must be additive until v2.0; breaking changes require migration plan in specs.

---

## 5. Communication & Isolation

- Encrypted Ethernet (or WiFi 6) between devices.
- Local daemons communicate via Unix domain sockets or shared memory with capability tokens.
- Solutions-plane style read-only observation for higher-level monitoring (when integrated with pitchfork-style systems).

---

## 6. Verification & Test Strategy

- Determinism benchmarks on fixed fixtures (scripts/benchmark_determinism.sh).
- Jasterish self-host preflight before any ported daemon is promoted.
- Chaos harnesses for device sync and budget enforcement.
- 180 NDPL patterns as the canonical test oracle for masking/burnout detection.

---

**End of Design Specification**

Technical Specification (exact shared-state wire format, Jasterish guard API, bootstrap registry, systemd/quadlet manifests, CMake structure) follows.