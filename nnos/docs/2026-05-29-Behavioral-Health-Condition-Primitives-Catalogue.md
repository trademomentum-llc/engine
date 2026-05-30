# Catalogue of Behavioral Health Condition Primitives

**Document ID:** NEURODIOS-BH-PRIMITIVES-CAT-001  
**Version:** 0.9.0 (Foundation Draft)  
**Date:** 2026-05-29  
**Project:** NeuroDiOS (Neurodivergent Neural-Link Operating System)  
**Status:** Foundational Primitives — Starting Point for All Adaptive Logic

---

## 1. Purpose

This catalogue defines the **minimal set of first-class behavioral health condition primitives** that NeuroDiOS must be able to recognize, model, and act upon.

These primitives are the "very beginning" — the atomic, deterministic building blocks from which all higher-level adaptation, risk scoring, context budgeting, masking prevention, and morphogenetic repair logic will be constructed.

Each primitive is specified with:
- Observable signals (physiological + behavioral)
- Deterministic recognition model
- Unhealthy fluctuation signatures
- System adaptation/offset primitives the kernel and daemons can invoke

The goal is to give the system the ability to detect when the human operator is entering states that will produce unhealthy variance in performance, decision quality, or long-term neurological health, and to automatically apply bounded compensatory actions.

---

## 2. Design Principles for These Primitives

1. **Deterministic Recognizability** — Must be detectable from measurable signals with bounded false-positive rates.
2. **Fluctuation Awareness** — Primitives track not just static state but rate, duration, and acceleration of change.
3. **Adaptation Offsettable** — Every recognized unhealthy fluctuation must have at least one defined system action that can reduce its impact.
4. **Neurodivergent-Native** — Prioritizes conditions especially costly for neurodivergent nervous systems (masking load, sensory processing differences, executive function variance).
5. **Composable** — Primitives can combine into higher-order states (e.g., "Masked Sensory Overload + Decision Fatigue").

---

## 3. Core Primitives Catalogue

### BH-01: Executive Depletion (Burnout Precursor)

**Recognition Signals**
- Sustained elevation in task initiation latency
- Increase in context switch frequency without productivity gain
- Drop in heart rate variability (HRV) below personal baseline
- Self-reported or inferred "mental fog" markers (increased typing errors, longer pauses in speech)

**Deterministic Model**
- State machine: Normal → Elevated Load → Depletion Threshold Crossed
- Primary metric: `ExecutiveBudgetRemaining = f(HRV_trend, context_switches_last_90min, task_completion_quality)`
- Unhealthy fluctuation: `d(ExecutiveBudgetRemaining)/dt < -threshold` for duration > 25 minutes

**Unhealthy Variants**
- Rapid depletion (acute)
- Slow chronic drain (no recovery overnight)
- Masked depletion (outward performance maintained while internal cost is high)

**System Adaptation Primitives**
- Automatic tightening of context-switch budget
- Forced micro-recovery window (minimum 8 minutes, no new task intake)
- Reduction of sensory input channels (e.g., suppress non-critical notifications)
- Elevation of "masking risk" score passed to ThreatIntelligenceManager

---

### BH-02: Masking Load Accumulation

**Recognition Signals**
- Sustained high outward performance while internal physiology shows stress (HRV suppression + elevated heart rate)
- Increased social/communication output without corresponding recovery behavior
- Post-interaction exhaustion spikes (detectable via subsequent task performance collapse)
- Self-report or behavioral markers of "performing normality"

**Deterministic Model**
- `MaskingLoad = (SocialDemand + CognitiveMaskingEffort) - RecoveryCredits`
- Detection when `MaskingLoad` exceeds personal baseline + acceleration > threshold

**Unhealthy Variants**
- High-intensity short masking (social marathon)
- Low-grade continuous masking (workplace "professional" mode all day)
- Masking + sensory load compound

**System Adaptation Primitives**
- Preemptive reduction of scheduled social/cognitive demand in the next time block
- Insertion of "decompression" context (low-demand, low-sensory period)
- Warning to user + optional auto-decline of non-critical meetings
- Increase in MorphogeneticMaintainer priority for recovery scheduling

---

### BH-03: Sensory Overload / Processing Capacity Exhaustion

**Recognition Signals**
- Increased error rate on fine-motor or detail-oriented tasks
- Physiological arousal markers (heart rate elevation, pupil dilation proxies if available)
- Behavioral withdrawal or stimming increase
- Reduced tolerance for concurrent auditory + visual input

**Deterministic Model**
- `SensoryLoadIndex = weighted_sum(normalized inputs from auditory, visual, proprioceptive, interoceptive channels)`
- Overload when `SensoryLoadIndex > personal_capacity * 0.85` for > 12 minutes or rate of increase > threshold

**Unhealthy Variants**
- Acute sensory meltdown trajectory
- Chronic low-level sensory exhaustion (common in open offices)
- Interoceptive overload (internal body signals becoming overwhelming)

**System Adaptation Primitives**
- Dynamic reduction of notification density and UI complexity
- Environmental control suggestions (lighting, sound) where hardware permits
- Enforcement of "sensory buffer" periods between high-input tasks
- Elevation of priority for noise-cancelling or visual filtering modes

---

### BH-04: Context Switching Fragmentation

**Recognition Signals**
- Rapid increase in number of active contexts (open applications, documents, conversations)
- Drop in depth-of-work metrics (time spent in single context < threshold)
- Elevated error rate when returning to previous context
- Physiological signature of chronic low-grade stress

**Deterministic Model**
- `ContextFragmentationScore = (unique_contexts_last_45min / average_dwell_time)`
- Unhealthy when score exceeds personal optimum + negative trend in output quality

**Unhealthy Variants**
- Hyper-switching (ADHD-style)
- Anxious context proliferation (anxiety-driven checking)
- Forced switching due to external interruptions

**System Adaptation Primitives**
- Hard enforcement of context-switch budget (kernel or daemon level)
- "Context lock" mode that defers non-critical interrupts
- Automatic grouping and deferral of related low-priority tasks
- Post-switch recovery buffer (minimum dwell time enforcement)

---

### BH-05: Rejection Sensitive Dysphoria (RSD) / Emotional Dysregulation Spike

**Recognition Signals**
- Sudden drop in task engagement following perceived negative feedback or ambiguity
- Physiological arousal disproportionate to current task demand
- Behavioral markers: increased self-deprecation language, task avoidance, or compensatory overwork
- Rapid shift from high agency to helplessness language

**Deterministic Model**
- Short-term change detection on emotional valence markers + task abandonment rate
- Spike declared when deviation from personal baseline exceeds 2.5σ within 15-minute window

**Unhealthy Variants**
- Acute RSD crash (hours of lost productivity)
- Chronic low-grade RSD sensitivity (constant background emotional taxation)
- RSD-triggered masking escalation

**System Adaptation Primitives**
- Temporary increase in task acceptance thresholds (protect against overcompensation)
- Insertion of low-stakes, high-mastery micro-tasks to rebuild agency
- Suppression of non-essential feedback channels during detected spike
- Morphogenetic repair cycle focused on emotional state stabilization

---

### BH-06: Sleep Debt / Circadian Misalignment Impact

**Recognition Signals**
- Elevated reaction time variance
- Increased reliance on external structure (more frequent checking of lists/calendars)
- Reduced tolerance for ambiguity or novelty
- Physiological markers (if wearable data available): poor HRV during "awake" hours, atypical temperature curves

**Deterministic Model**
- `CognitiveCapacityModifier = f(sleep_debt_hours, time_since_last_quality_rest, circadian_phase_offset)`
- Unhealthy when modifier drops below 0.75 of personal baseline

**Unhealthy Variants**
- Acute sleep debt (one bad night)
- Chronic misalignment (shift work, delayed sleep phase)
- "Revenge bedtime procrastination" pattern

**System Adaptation Primitives**
- Aggressive reduction of total cognitive load for the day
- Banning of high-stakes or novel tasks during low-capacity windows
- Stronger enforcement of wind-down protocols in evening
- Automatic rescheduling of non-critical work

---

## 4. Cross-Cutting Primitives

These are not standalone conditions but measurable properties that modulate all of the above:

- **Fluctuation Velocity** — Rate of change in any primary metric
- **Recovery Debt** — Cumulative deficit in restorative activities
- **Masking Efficiency** — Ratio of outward performance to internal cost
- **Context Budget Remaining** (core primitive already referenced across specs)

---

## 5. Relationship to Existing NeuroDiOS Concepts

These primitives directly feed:
- The 180 Neurodivergent Patterns Library (NDPL)
- Burnout and masking risk models in `lsa_boot_hcn`
- Context budget enforcement logic
- Morphogenetic repair prioritization
- Future kernel-level primitives (context switch guards, physiology feedback channels)

---

**End of Catalogue (Foundation Version)**

This document defines the primitive language the entire adaptive NeuroDiOS system will speak. All subsequent daemon logic, risk models, adaptation rules, and kernel extensions must be expressed in terms of these (or composable extensions of these) primitives.