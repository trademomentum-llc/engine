# NeuroDiOS Non-Invasive Neural Link (NINL) Subsystem — Design Specification

**Document ID:** NEURODIOS-NINL-DES-001  
**Version:** 1.0.0  
**Date:** 2026-05-30  
**Status:** Authoritative Baseline  
**Related:** NEURODIOS-NINL-REQ-001 (Requirements)

---

## 1. Architectural Overview

The Non-Invasive Neural Link is designed as a layered, safety-first pipeline whose primary objective is to convert high-dimensional, noisy, non-stationary brain signals into low-entropy, high-confidence commands and prompts while never violating the user’s behavioral health invariants.

It follows a strict “sense → guard → interpret → act → recover” rhythm that maps directly onto the Roller Coaster Framework (high-bandwidth neural engagement periods followed by enforced coasting/recovery periods).

The architecture deliberately separates:
- The physical sensor head and its immediate analog/digital front-end (minimal trusted hardware).
- The deterministic digital signal processing and interpretation pipeline (mostly integer/fixed-point).
- The governance layer (NeuroBalance + Roller Coaster) that has veto power over the entire link.

This separation ensures that even if the signal interpretation layer contains sophisticated models, the safety and health machinery remains simple, auditable, and independent.

---

## 2. Major Components

### 2.1 Sensor Head & Acquisition Front-End

- Primary modality: Electromagnetic wave resonance (resonant inductive or capacitive coupling at frequencies chosen for penetration with minimal power and maximal neurophysiological contrast).
- Fallback/hybrid modalities: Dry high-density EEG + fNIRS.
- Local capabilities: Analog filtering, amplification, high-resolution ADC (at least 24-bit where physics demands), basic on-sensor decimation.
- Hard constraints: Maximum average power, peak instantaneous power, and duty cycle are physically limited in hardware and further software-governed.

The sensor head is treated as a peripheral. It does not make command decisions.

### 2.2 Deterministic Signal Processing Pipeline

Stages (all versioned and provenance-tagged):

1. **Raw Windowing & Denoising** — Fixed-length overlapping windows. Deterministic bandpass / notch filtering expressed in fixed-point or integer arithmetic where possible.
2. **Feature Extraction** — Time-domain statistics, frequency-domain energy in canonical bands (delta/theta/alpha/beta/gamma + subject-specific resonant bands), phase-amplitude coupling measures, all reduced to Intensity8 / Delta8 or FixedQ7_9 representations.
3. **Pattern Matching / Intent Classification** — Two parallel paths:
   - High-confidence discrete command path (template matching or small deterministic classifier → discrete token).
   - Higher-entropy prompt path (more contextual, lower confidence → structured prompt for downstream agents).
4. **Confidence & Provenance Tagging** — Every output token carries an Intensity8 confidence, a pipeline version hash, and a rolling signal-quality metric.

All hot-path arithmetic after the initial physics modeling is required to be integer or fixed-point.

### 2.3 Safety & Governance Layer (NeuroBalance Integration)

The Neural Link is a first-class client of the NeuroBalance Coordinator:

- Before any acquisition session, it queries current user state.
- During the session, it streams a continuous “Neural Load” signal (derived from signal complexity, rate of change, and user historical response) into the Coordinator.
- The Coordinator can command immediate suspension, rate reduction, or forced coasting period.
- The Roller Coaster governor uses the link’s own bandwidth as one of the controllable oscillation dimensions: high-engagement neural input periods are deliberately followed by enforced low-input or zero-input recovery windows.

This integration makes prolonged unsafe use structurally difficult.

### 2.4 Command & Prompt Egress

Outputs flow into the existing NeuroDiOS shared state mechanisms (the historical `/dev/shm/nnos_neural_link` concept is the natural target, now extended with a dedicated high-rate neural ring buffer).

Discrete commands are injected as high-priority events.
Natural language prompts are injected as first-class prompt sources for any agent (including future Kimi-style autonomous components) running on the system.

### 2.5 Calibration & Model Management

Calibration produces a small, versioned, auditable artifact (user-specific resonant frequency response, baseline noise profile, personal command templates).

Learned models, if used at all, are treated as versioned black boxes with:
- A deterministic fallback path.
- A hash of the exact weights + training metadata under Origin Vault.
- Continuous performance monitoring with automatic drift flagging.

---

## 3. Safety Interlocks (Mandatory)

1. **NeuroBalance Veto** — Absolute. No bypass.
2. **Hardware Power Envelope** — Physical current limiting + software watchdog that can cut power to the emitter.
3. **Session Budget** — Rolling energy and time budgets that the user cannot override in the moment.
4. **Signal Quality Guard** — If signal-to-noise or stability falls below threshold for a configurable duration, automatic graceful shutdown.
5. **User-Observable Kill Switch** — Simple, reliable physical or very low-cognitive-load software mechanism that instantly disables the link.
6. **Post-Session Coasting Enforcement** — The Roller Coaster governor will not allow another high-bandwidth neural session until a minimum recovery window has elapsed.

---

## 4. Data Flow & Integration Points

- Sensor Head → Local Front-End (raw samples)
- Local Front-End → Host (via USB, Bluetooth LE, or custom low-power link) → Deterministic Pipeline
- Pipeline → Neural Command/Prompt Tokens → NeuroDiOS Shared State + Event Bus
- NeuroBalance Coordinator ← bidirectional → Neural Link Governor (load feedback + control commands)
- Roller Coaster Governor ← Neural Link bandwidth usage as one oscillation axis

The link is deliberately not allowed to become a direct privileged channel that bypasses the rest of the health and context machinery.

---

## 5. Relationship to the 8 Validated Denominators

The entire design is derived from them:

- **Fluctuation Dynamics (#1)**: Brain signals are the highest-velocity signals the system will ever see. The architecture is built to measure and bound their rate of change.
- **Budget (#2)**: EM energy, compute cycles on the host, and cognitive load on the user are all first-class budgeted resources.
- **Contrast (#3)**: Distinguishing intentional patterns from ongoing neural “noise” is the central technical problem.
- **Controlled Oscillation (#4)**: The Roller Coaster framework is extended so that the neural link itself becomes one of the controllable high/low phases.
- **Adaptation Offset (#5)**: NeuroBalance can actively schedule recovery or counter-stimuli after heavy neural link use.
- **Primitive Traceability (#6)**: Every command is traceable to signal features + exact pipeline version.
- **Origin Vault (#7)**: All calibration, models, firmware, and session logs are immutable and auditable.
- **Drift Detection (#8)**: Continuous monitoring of signal quality, user response, and model performance.

---

## 6. Evolutionary Path

Phase 1 (current): Pure receive-only link (brain → system commands/prompts). No output stimulation.

Phase 2: Carefully controlled, low-power, low-duty-cycle feedback or entrainment signals (still under extreme safety constraints).

Phase 3: Tighter integration with the sovereign Jasterish Micro-Kernel (user-space neural driver + kernel-level rate limiting and safety hooks).

The Requirements and this Design treat Phase 1 as the mandatory first delivery. Later phases must re-prove all safety invariants.

---

**End of Design Specification**

This document, together with the Requirements and the forthcoming Technical Specification, forms the complete authoritative baseline for the Non-Invasive Neural Link Subsystem. Implementation of any part of the signal chain or hardware interface may begin only after the full triad is complete and referenced from the Chained Source of Truth.