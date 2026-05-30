# Requirements Specification: NNOS Daemon Constellation + Jasterish Validation/Morph Port

**Document ID:** NNOS-REQ-001  
**Version:** 1.0.0  
**Date:** 2026-05-28  
**Highest-Value Projects:** engine/nnos (primary), apps/ (Jasterish compiler), pitchfork (consumer via agent layer)

---

## 1. Purpose

NNOS (Neurodivergent Neural-Link Operating System) is a constellation of deterministic, compiled daemons that monitor physiology, prevent burnout/masking, enforce context-switch budgets, and synchronize state across devices with end-to-end encryption.

This Requirements Specification defines the mandatory properties of the baseline 6-daemon constellation and the Jasterish porting path for the validation and morphogenetic layers.

---

## 2. Baseline Daemon Constellation (Mandatory)

The system shall consist of at minimum the following daemons (or their Jasterish-port equivalents):

1. **lsa_boot_dcn** (or equivalent) — Device Coordination Node bootstrap / shared-state daemon.
2. **lsa_boot_hcn** — Human Coordination Node (physiology + task limit enforcement).
3. **lsa_boot_epn** — External Perception Node (sensory load, environmental context).
4. **SystemIntegrityDaemon** — Integrity monitoring and drift detection.
5. **ThreatIntelligenceManager** — Local threat / masking detection.
6. **MorphogeneticMaintainer** — Self-healing / optimization cycle (daily scheduled).

Additional profile daemons (morph engine, threat scanner, neuro analyzer) are explicitly optional extensions, not baseline.

---

## 3. Functional Requirements

FR-001: All daemons must be compiled native binaries (C++ baseline; Jasterish port for validation + morph layers). No interpreted code in the runtime path.

FR-002: Physiology monitoring (heart rate, sensory load, etc.) must feed a deterministic burnout/masking risk model with bounded context-switch budgets.

FR-003: Task limits and masking warnings must be enforceable at the OS level (systemd units, capability restrictions, or Jasterish runtime guards).

FR-004: Device synchronization must occur over encrypted Ethernet (or WiFi 6) with origin-vault provenance.

FR-005: The Jasterish validation-layer port must act as a runtime guard on emitted code / shared-state transitions, not as a compiler pass that validates every IR instruction.

FR-006: The morphogenetic engine port must implement glial-style repair: detect damage (drift), remove, restore connectivity, optimize, adapt topology.

---

## 4. Determinism and Verification Requirements

- Fixed-input computations must produce identical outputs (measured via `scripts/benchmark_determinism.sh`).
- Jasterish self-host ladder must be verified with `apps/scripts/jstar_bootstrap_check.sh` before any promotion.
- 180 Neurodivergent Patterns Library (NDPL-REQ-003) must be versioned and used as the canonical rule source for masking / burnout detection.

---

## 5. Deployment and Bootstrap Requirements

- Must support native Linux (Ubuntu 22.04+/Jetson), Docker, Podman rootless, and VM-hosted runs.
- Missing `CMakeLists.txt` and the three `lsa_boot_*` targets are required before the constellation is considered buildable.
- systemd units, quadlets, or supervisord configs must be materialized from the specs.

---

## 6. Integration with Highest-Value Ecosystem

- **apps/**: Jasterish compiler is the target language for the validation and morph ports.
- **pitchfork/agent/**: The deterministic agent can consume NNOS traces for proof-validated suggestions in the Forgejo overlay.
- Shared governance frameworks (BBB security, procedural registry, PQC) from infra/ must be applied to the daemon constellation.

---

**End of Requirements Specification**

Design and Technical Specifications for the daemon interfaces, shared-state schema, Jasterish port contracts, and bootstrap registry follow in the mandated triad. This document was created to close the formal specification gap for one of the four highest-value projects.