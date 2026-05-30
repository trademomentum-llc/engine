# Technical Specification: NeuroBalance Engine — Master Health Coordinator

**Document ID:** NEURODIOS-NBE-TS-001  
**Version:** 1.0.0  
**Date:** 2026-05-29

---

## 1. Core Runtime Model

The coordinator runs a continuous loop:

1. Ingest signals → Update denominator readings (velocity, acceleration, value).
2. Receive current Roller Coaster phase from the framework.
3. Run assessment using base NeuroBalance primitive extraction + relationship derivation (lens modes).
4. Activate relevant Offset Engines for active conditions/clusters.
5. Output bounded OffsetActions that respect the current phase and denominator safety margins.

---

## 2. Offset Engine Interface (Example)

```python
class OffsetEngine:
    def recommend(
        self,
        readings: Dict[ValidatedDenominator, DenominatorReading],
        current_phase: RollerCoasterPhase,
    ) -> List[OffsetAction]:
        ...
```

Each concrete engine (ExecutiveDepletionOffsetEngine, MaskingLoadOffsetEngine, etc.) implements this.

---

## 3. Roller Coaster Governance Rules (Technical)

- During ASCENT: Budget and Fluctuation acceleration have hard upper bounds. If crossed, force early DESCENT.
- During CRYSTALLIZATION: Strongly protect low-variance, high-contrast-differential conditions.
- Adaptation Offset is the universal "always-on" safety net.

---

## 4. Relationship to Original neurobalance-engine.py

The original engine provides the battle-tested low-level detection (primitives tagged atomic/kinetic/fractal, mechanism derivation via lens modes, gap identification, earned tracking).

The new coordinator wraps it, re-grounds its outputs in the 5 Validated Denominators, and adds explicit Roller Coaster phase awareness + per-condition Offset Engines.

---

**End of Technical Specification**