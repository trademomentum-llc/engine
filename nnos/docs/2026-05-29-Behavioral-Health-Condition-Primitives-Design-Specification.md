# Design Specification: Behavioral Health Condition Primitives Catalogue

**Document ID:** NEURODIOS-BH-PRIMITIVES-DS-001  
**Version:** 1.0.0  
**Date:** 2026-05-29  
**Predecessor:** NEURODIOS-BH-PRIMITIVES-REQ-001

---

## 1. Design Philosophy

The primitives are deliberately defined at a level of abstraction that is:
- Low enough to be implemented from real physiological and behavioral signals
- High enough to be meaningful for adaptation logic
- Deterministic enough to be auditable and safe for an operating system to act upon

This catalogue is the neurodivergent equivalent of the "condition codes" or "status registers" in a traditional CPU — observable internal states that higher layers of the system can read and respond to.

---

## 2. Structural Design

Each primitive is defined with four mandatory sections:

1. **Recognition Layer** — How the system detects the condition from raw signals.
2. **Fluctuation Model** — How unhealthy variance is quantified (not just presence/absence).
3. **Adaptation Offset Interface** — The set of actions the rest of NeuroDiOS can invoke.
4. **Composition Rules** — How this primitive interacts with others.

---

## 3. Integration with NeuroDiOS Architecture

These primitives are intended to be consumed by:

- **lsa_boot_hcn** (Human Coordination Node) — Primary consumer for burnout/masking logic
- **lsa_boot_epn** (External Perception Node) — Supplies sensory load signals
- **SystemIntegrityDaemon / ThreatIntelligenceManager** — Uses masking and emotional dysregulation primitives for risk scoring
- **MorphogeneticMaintainer** — Uses recovery debt and fluctuation velocity to prioritize repair cycles
- Future kernel extensions — Context budget enforcement, scheduling hints, and physiology feedback channels (see NeuroDiOS Jasterish Micro-Kernel specs)

---

## 4. Key Design Decisions

**Decision: Rule + Threshold First, ML Later**
All initial recognition models are specified as deterministic state machines or trend detectors. This satisfies the core NeuroDiOS requirement for verifiable, auditable behavior.

**Decision: Fluctuation-Centric Rather Than State-Centric**
The catalogue emphasizes detection of *unhealthy change* (velocity, acceleration, duration outside bounds) rather than static "you are in burnout." This aligns with the project's focus on preventing damage rather than only reacting to it.

**Decision: Explicit Adaptation Contracts**
Every primitive must declare the adaptation actions it can trigger. This prevents the common failure mode where a monitoring system detects problems but has no defined levers to pull.

---

## 5. Evolution Path

This catalogue is versioned. New primitives may be added when:
- A new measurable signal becomes reliably available
- A compound pattern is shown to have distinct adaptation requirements not covered by composition of existing primitives
- Clinical or empirical evidence from the 180 NDPL work demonstrates a high-value gap

---

**End of Design Specification**