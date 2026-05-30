# Technical Specification: Behavioral Health Condition Primitives Catalogue

**Document ID:** NEURODIOS-BH-PRIMITIVES-TS-001  
**Version:** 1.0.0  
**Date:** 2026-05-29  
**Predecessor:** NEURODIOS-BH-PRIMITIVES-DS-001

---

## 1. Purpose

This document provides the concrete technical definition of the primitives, including data models, state machine specifications, and example implementation patterns suitable for both C++ daemons and future Jasterish ports.

---

## 2. Data Model

All primitives share a common structure:

```cpp
struct BehavioralHealthPrimitive {
    PrimitiveID id;
    Timestamp last_updated;
    float intensity;           // 0.0 - 1.0 normalized
    float velocity;            // rate of change
    float acceleration;        // second derivative
    PrimitiveState state;
    vector<AdaptationAction> active_offsets;
};
```

---

## 3. Concrete Primitive Specifications (Selected Examples)

### BH-01: Executive Depletion

**State Machine**
- `NORMAL`
- `ELEVATED_LOAD`
- `DEPLETION_WARNING`
- `DEPLETION_CRITICAL`

**Transition Rules (Deterministic)**
```
if (hrv_trend < -8% per 15min AND context_switches > personal_baseline * 1.6)
    → DEPLETION_WARNING

if (state == DEPLETION_WARNING AND duration > 20min)
    → DEPLETION_CRITICAL
```

**Adaptation Actions**
- `TIGHTEN_CONTEXT_BUDGET(delta)`
- `FORCE_MICRO_RECOVERY(duration_minutes)`
- `SUPPRESS_NONCRITICAL_NOTIFICATIONS()`

---

### BH-02: Masking Load Accumulation

**Key Metric**
```math
MaskingLoad = (SocialDemand + CognitiveMaskingEffort) - (RecentRestorativeCredits * 0.7)
```

**Unhealthy Fluctuation Detection**
- If `MaskingLoad > 0.75` AND `d(MaskingLoad)/dt > 0.04` per minute for 12+ minutes → trigger adaptation.

**Adaptation Actions**
- `INSERT_DECOMPRESSION_BLOCK()`
- `ELEVATE_MASKING_RISK_SCORE()`
- `REDUCE_SCHEDULED_DEMAND(next_window)`

---

## 4. Implementation Patterns

### Pattern A: Daemon-Side Evaluator (C++)

```cpp
class PrimitiveEvaluator {
public:
    void update(const PhysiologySample& sample, const BehavioralTelemetry& telemetry);
    PrimitiveState get_state(PrimitiveID id) const;
    vector<AdaptationAction> get_recommended_actions(PrimitiveID id) const;
};
```

### Pattern B: Kernel-Level Primitive Hooks (Future Jasterish)

The Jasterish Micro-Kernel should eventually expose minimal syscalls such as:
- `sys_query_context_budget()`
- `sys_report_physiological_marker(type, value)`
- `sys_request_adaptive_throttling(reason_primitive)`

These are referenced in the existing NeuroDiOS Jasterish Micro-Kernel Technical Specification as "NeuroDiOS Extensions."

---

## 5. Versioning and Governance

- The catalogue lives at `engine/nnos/docs/2026-05-29-Behavioral-Health-Condition-Primitives-Catalogue.md`
- All changes require corresponding updates to the three specification documents in this triad.
- The 180 NDPL rules shall be expressed as compositions and parameterizations of these primitives.

---

**End of Technical Specification**