# NeuroDiOS Efficiency & Minimal Footprint Layer

**Document ID:** NEURODIOS-EFF-001  
**Version:** 1.0  
**Date:** 2026-05-30

---

## Objective

Every action, primitive evaluation, mechanism, and context load must use the absolute minimum computational and token resources required for correctness and determinism.

### Two Dimensions of Optimization

1. **Runtime / Compute Footprint**
   - Default to INT8 (0-255) or INT16.
   - Use FP32 only when the underlying physics or required precision genuinely demands it.
   - Track "Compute Footprint" as an explicit sub-resource inside the Budget/Resource Accounting denominator.

2. **Token / Context Footprint**
   - Provide "Minimal Context" variants of all critical binding documents.
   - The Kimi agent (and any other agent) can load the compressed form when working under tight token budgets.

---

## Implementation Artifacts

- `engine/nnos/neurobalance/minimal_types.py` — canonical minimal integer and fixed-point types + footprint calculator.
- `neurobalance_coordinator.py` — updated to use minimal representations in hot paths and to enforce the new Compute Footprint sub-budget.
- `Minimal_Context_Kimi_Binding.md` files in both `engine/nnos/lsa/synthesized/` and `apps/context/`.

---

## Governance Rule

When designing or reviewing any new action, offset, or mechanism:

> "What is the smallest safe integer type (or fixed-point representation) that can express this behavior deterministically? Use that — never more."

The NeuroBalance Engine will eventually penalize (via reduced Budget) any emitted behavior that wastes compute on oversized types.

---

**End of Efficiency Layer Specification**