# NeuroDiOS Non-Invasive Neural Link (NINL) Subsystem — Requirements Specification

**Document ID:** NEURODIOS-NINL-REQ-001  
**Version:** 1.0.0  
**Date:** 2026-05-30  
**Status:** Authoritative Baseline  
**Maintainer:** Grok (System Architect / Live Context)  
**Project:** NeuroDiOS (Neurodivergent Neural-Link Operating System)  

---

## 1. Purpose

The Non-Invasive Neural Link (NINL) Subsystem is the hardware-software interface layer that realizes the original core vision of NeuroDiOS: a direct, bidirectional neural connection between the user and the system that does not require surgical implantation.

It accepts raw or preprocessed signals from non-invasive electromagnetic wave resonance sensors (or equivalent safe modalities such as high-density dry-electrode EEG, fNIRS, or hybrid EM-optical approaches) and transforms them into deterministic, interpretable command and prompt streams that the rest of the NeuroDiOS stack can act upon.

The subsystem must operate under the strict constraints of the 8 Validated Denominators, the Efficiency Mandate, the NeuroBalance Coordinator, and the Roller Coaster Framework so that the act of using the neural link itself does not create or amplify unhealthy behavioral or neurological fluctuations.

This is the first authoritative triad for the hardware-facing neural link layer. All prior references to "nnos_neural_link" (shared memory IPC) are internal software mechanisms and are separate from this subsystem.

---

## 2. Functional Requirements

FR-1. **Non-Invasive Signal Acquisition**  
The subsystem SHALL support only non-invasive sensor modalities. Primary target modality: electromagnetic wave resonance (near-field or resonant inductive coupling tuned to neurophysiologically relevant frequency bands). Secondary supported modalities: high-density dry EEG arrays and fNIRS. No requirement or support for invasive (implanted) electrodes.

FR-2. **Safe Exposure Envelope**  
The subsystem SHALL enforce hard limits on transmitted EM power, duty cycle, and session duration. These limits are first-class Budget (#2) resources and are dynamically adjusted by the NeuroBalance Coordinator based on the user's current state (sensory load, emotional load, recent fluctuation velocity).

FR-3. **Deterministic Signal Chain**  
All signal processing from raw acquisition through feature extraction to command generation SHALL be expressible with deterministic, reproducible mathematics. Where floating point is required for physics, it SHALL be isolated, versioned, and paired with fixed-point (FixedQ7_9 or better) or integer equivalents for the hot command-generation path.

FR-4. **Command & Prompt Generation**  
The subsystem SHALL produce two classes of output:
- Discrete commands (high-confidence, low-entropy patterns mapped to discrete actions).
- Natural language or structured prompts (lower-confidence, higher-entropy patterns that are passed to the prompt interpretation layer or directly to Kimi-style agents).

Every output item SHALL carry a confidence scalar (Intensity8), a provenance hash, and a reference to the exact model/pipeline version that produced it.

FR-5. **Integration with Existing Sensory & Health Systems**  
NINL outputs SHALL be treated as an additional high-bandwidth sensory channel. They SHALL feed:
- The SensoryLoadIndex calculation.
- The NeuroBalance Coordinator (as both a potential source of load and a candidate for Adaptation Offset actions).
- The Roller Coaster governor (to enforce safe high-bandwidth / coasting cycles).

FR-6. **Calibration & Personalization**  
The subsystem SHALL support per-user calibration that produces a minimal, versioned, auditable profile. Calibration data is stored under Origin Vault (#7) rules and is subject to Drift Detection (#8).

FR-7. **Fail-Safe & Graceful Degradation**  
If signal quality drops below threshold, or if NeuroBalance declares the link unsafe, the subsystem SHALL immediately fall back to zero-input (no commands generated) and notify the rest of the system. Hard physical or firmware kill-switch capability is required.

FR-8. **Traceability & Audit**  
Every command or prompt generated from neural signals SHALL be traceable to:
- The exact time window of raw signal.
- The processing steps and model version applied.
- The user state (NeuroBalance reading) at the moment of generation.

This traceability is mandatory for Primitive Traceability (#6) and Origin Vault (#7).

---

## 3. Non-Functional Requirements

NFR-1. **Behavioral Health Primacy**  
No feature of the Neural Link may be activated or remain active if it would violate any of the validated Behavioral Health Condition Primitives or the Roller Coaster safety cycles. The link is a tool for the user; the user’s long-term neurological and behavioral health is the higher-order objective.

NFR-2. **Minimal Computational & Energetic Footprint (Efficiency Mandate)**  
On-device or near-device processing must default to the smallest safe integer and fixed-point types. FP32/64 is permitted only in well-isolated calibration and physics-modeling sections and must be explicitly justified. Power draw of the sensor head and any local compute is a tracked Budget resource.

NFR-3. **Determinism & Reproducibility**  
Given the same raw signal window, the same calibration profile, and the same pipeline version, the subsystem SHALL produce bit-identical command/prompt outputs (modulo explicitly documented non-deterministic elements such as true hardware entropy sources, which must be separately logged).

NFR-4. **Safety Under Fluctuation**  
The subsystem must be robust to the high Fluctuation Dynamics (#1) inherent in brain signals. It must not amplify rapid state changes into unsafe command rates. Rate limiting and temporal smoothing are mandatory and themselves governed by the Roller Coaster framework.

NFR-5. **Auditability & Origin Vault**  
All calibration data, model weights (if any), firmware versions, and session logs are immutable artifacts under the Origin Vault. Any change to the signal interpretation pipeline constitutes a versioned, traceable event.

NFR-6. **Regenerability of Interpretation History**  
It must be possible to replay any past session’s raw (or sufficiently high-fidelity) neural data through the exact pipeline version that was active at the time and obtain the same commands/prompts. This is the hardware analogue of Invariant L in the Kimi Execution Log system.

---

## 4. Constraints & Invariants

C-1. **Non-Invasiveness is Non-Negotiable**  
Any design or implementation that requires or encourages surgical implantation is out of scope and forbidden.

C-2. **NeuroBalance Veto**  
The NeuroBalance Coordinator has absolute veto power over link activation, session duration, bandwidth, and command emission rate. This veto is not overrideable by user intent in the moment if the coordinator assesses elevated risk.

C-3. **No Bypass of Behavioral Health Primitives**  
The Neural Link may not be used to circumvent the behavioral health condition primitives or the Roller Coaster stress/coast discipline. Direct brain input is treated as an extremely high-potency sensory and cognitive channel and is therefore subject to the strictest controls.

C-4. **Minimal Trusted Computing Base for Signal Path**  
The chain from sensor to first deterministic command token must be kept as small and auditable as possible. Complex learned models, if used, must be versioned, hashed, and accompanied by a simpler deterministic fallback path.

C-5. **Human User Boundary**  
Per the 2026-05-30 clarification to the Kimi Execution Log system: the Neural Link layer observes and acts on the user’s brain signals. It does not observe or log the content of human conversations with this development environment or any external chat interfaces.

---

## 5. Mathematical & Deterministic Grounding

The following invariants must hold for any compliant implementation:

**Invariant S (Signal Safety):**  
For any time window W, if NeuroBalance.assess_and_offset() returns an intervention tier ≥ 3 for the user, then the Neural Link SHALL emit zero commands during W + cooldown.

**Invariant D (Deterministic Interpretation):**  
Let P_v be pipeline version v.  
Let S be a fixed raw signal window.  
Let C be the user calibration at time t.  
Then P_v(S, C) produces the identical command token sequence on every execution (modulo explicitly declared entropy sources that are separately journaled under Origin Vault).

**Invariant B (Budget Enforcement):**  
EM energy delivered to the user during any rolling 24-hour window is a first-class Budget resource. Exceeding the current NeuroBalance-approved allocation forces immediate link suspension.

These invariants are directly derivable from the 8 Validated Denominators and the existing NeuroBalance + Roller Coaster machinery.

---

## 6. Relationship to Existing Work

- Builds upon the SensoryLoadIndex and multi-channel signal model already present in the Behavioral Health Primitives.
- Consumes and is governed by the NeuroBalance Coordinator (the live health governor).
- Must be compatible with the minimal integer type discipline (Intensity8, Delta8, FixedQ7_9) already proven in the daemon constellation and minimal_types.py.
- Will eventually be driven from (or drive) user-space services running on the Jasterish Micro-Kernel.
- Its session history and command provenance must be expressible inside the Kimi Execution Log when the autonomous agents are developing or validating the signal processing components.

---

**End of Requirements Specification**

This document, together with the forthcoming Design Specification and Technical Specification, constitutes the complete authoritative baseline for the NeuroDiOS Non-Invasive Neural Link Subsystem. No implementation work on the hardware interface layer may proceed until the full triad exists and is cross-referenced from the Chained Source of Truth.