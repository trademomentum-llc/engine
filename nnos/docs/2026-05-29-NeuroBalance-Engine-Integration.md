# NeuroBalance Engine Integration — Master Coordinator for NeuroDiOS

**Document ID:** NEURODIOS-NBE-INT-001  
**Version:** 1.0.0  
**Date:** 2026-05-29

---

## 1. Context

The existing `neurobalance-engine.py` (from nnos-lsa) is already an advanced implementation that:

- Extracts primitives tagged **atomic / kinetic / fractal**.
- Uses **LensMode** (mirror, counter, orthogonal, synth, observer) to derive mechanisms from relationships — exactly matching our relational exercise.
- Identifies **Gaps**.
- Recommends **Support Actions** (offsets).
- Has strong "earned, not declared" tracking via PatchStreamStats.

This engine is the natural low-level detector and offset recommender.

The task is to **elevate and integrate** it as the central **NeuroBalance Coordinator** that:

- Operates directly on the 5 **Validated Denominators**.
- Actively manages health across all behavioral health primitives and clusters.
- Serves as the **governor/safety layer** for the Roller Coaster Framework (allows productive stress for uplift while preventing damage).
- Maintains healthy lifestyle baselines even while the user is deliberately engaged in system-driven cycles.

---

## 2. Architecture

### Layering

- **Level 0 (Foundation):** The 5 Validated Denominators (these are the "physics").
- **Level 1 (Detection):** Existing NeuroBalanceEngine logic (primitives + mechanisms + gaps), now re-grounded in the Denominators.
- **Level 2 (Coordination):** New NeuroBalanceCoordinator that:
  - Runs continuous assessment against the Roller Coaster phase.
  - Activates specific **Offset Engines** for active conditions.
  - Enforces healthy bounds even during deliberate ascent phases.
- **Level 3 (Execution):** Bounded support actions that can be advisory or (when in EXECUTE plane) directly influence task budgets, context, sensory environment, etc.

---

## 3. Offset Engines (Per Condition / Cluster)

For each major behavioral health area, we create a dedicated lightweight "Offset Engine" that uses the base NeuroBalance logic + the relevant Validated Denominators.

Examples:

- **ExecutiveDepletionOffsetEngine** — primarily uses Budget Accounting + Adaptation Offset + Fluctuation Dynamics.
- **MaskingLoadOffsetEngine** — uses Contrast Differential + Controlled Oscillation (to create safe relief after masking periods).
- **SensoryOverloadOffsetEngine** — uses Adaptation Offset + Fluctuation Dynamics.
- **RollerCoasterGovernor** — special meta-offset engine that monitors the entire cycle and applies damping when any denominator shows unhealthy acceleration.

---

## 4. Integration with Roller Coaster

The NeuroBalance Engine becomes the **active safety system** for the Roller Coaster:

- During Ascent: Allows tension but watches denominator velocity/acceleration.
- At Peak: Can force Anchor Emulation or early descent if thresholds crossed.
- During Descent/Coast: Actively amplifies relief using the denominators to maximize insight crystallization without crash.

This fulfills the original request: the system can still "put the user through stressful situations" for uplift, but the NeuroBalance layer ensures it remains within healthy lifestyle parameters.

---

## 5. Next Implementation Steps

1. Wrap the existing neurobalance-engine.py logic.
2. Create the Coordinator + specific OffsetEngine classes.
3. Wire it to the Roller Coaster Cycle Log and the 5 Denominators.
4. Provide both ANALYSIS (advisory) and EXECUTE (bounded action) modes.

This document + the upcoming triad will define the full subsystem.

---

**End of Integration Overview**