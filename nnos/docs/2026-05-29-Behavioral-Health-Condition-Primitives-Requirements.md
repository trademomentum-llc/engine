# Requirements Specification: Behavioral Health Condition Primitives Catalogue

**Document ID:** NEURODIOS-BH-PRIMITIVES-REQ-001  
**Version:** 1.0.0  
**Date:** 2026-05-29  
**Predecessor:** None (Foundational)

---

## 1. Purpose

This document defines the requirements for a formal, versioned catalogue of behavioral health condition primitives. These primitives constitute the lowest-level language through which NeuroDiOS will perceive, model, and adapt to the human operator's internal state.

Without these primitives, higher-level daemons (physiology monitoring, burnout prevention, masking detection, context budgeting) lack a deterministic, composable foundation.

---

## 2. Scope

### In Scope
- Definition of the minimal viable set of recognizable behavioral health condition primitives
- Specification of observable signals, recognition models, unhealthy fluctuation signatures, and system adaptation offsets for each primitive
- Requirements for deterministic modeling (state machines, trend detection, bounded metrics)
- Requirements for composability and integration with existing NeuroDiOS concepts (180 NDPL, context budgets, morphogenetic repair)

### Out of Scope (Future)
- Full implementation of all sensor ingestion pipelines
- Machine learning models for signal classification (initial versions must be rule + threshold based for determinism)
- Kernel-level enforcement mechanisms (these are derived requirements)

---

## 3. Functional Requirements

**FR-BH-01:** The system shall maintain a versioned catalogue of behavioral health condition primitives.

**FR-BH-02:** Each primitive shall define:
- A finite set of observable input signals
- A deterministic recognition model (state machine or threshold + derivative rules)
- At least one measurable unhealthy fluctuation pattern
- At least two explicit system adaptation/offset actions

**FR-BH-03:** Recognition models must be executable with fixed computational cost and produce deterministic outputs for the same input history.

**FR-BH-04:** The catalogue must explicitly cover conditions that are particularly high-cost for neurodivergent nervous systems, including but not limited to: masking load, sensory overload, executive depletion, and context fragmentation.

**FR-BH-05:** Primitives shall be composable. The system must be able to detect compound states (e.g., Masking Load + Sensory Overload).

**FR-BH-06:** Every primitive must feed directly into existing NeuroDiOS mechanisms: context budget enforcement, masking risk scoring, and morphogenetic repair prioritization.

---

## 4. Non-Functional Requirements

- **Determinism:** All recognition logic must be reproducible given the same signal history.
- **Bounded Latency:** Primitive evaluation must complete within strict time bounds suitable for real-time daemon operation.
- **Extensibility:** The catalogue structure must allow addition of new primitives without invalidating existing ones.
- **Auditability:** Every state transition and adaptation decision must be traceable to the specific primitive(s) that triggered it.

---

## 5. Success Criteria

1. The catalogue contains a coherent, minimal set of primitives that together cover the primary failure modes described in the NNOS Daemon Constellation requirements (burnout, masking, sensory overload, context fragmentation).
2. Each primitive has at least one defined adaptation action that can be implemented by the current or near-future daemon set.
3. The catalogue is adopted as the canonical source for all future NDPL rule development and kernel neurodivergent-aware primitives.

---

**End of Requirements Specification**