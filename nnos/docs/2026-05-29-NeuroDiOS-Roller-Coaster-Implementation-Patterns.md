# Implementation Patterns: Roller Coaster Framework + Relational Primitives

**Document ID:** NEURODIOS-RCF-IMPL-001  
**Date:** 2026-05-29

---

## 1. Primitive Clustering (Deterministic)

```cpp
enum class LoadCluster { ExecutiveDepletion, SensoryOverload, ContextFragmentation, SleepDebt };
enum class VolatilityCluster { MaskingLoad, RSDEmotionalSpike };

struct ClusterState {
    float aggregate_intensity;
    float aggregate_velocity;
};
```

---

## 2. Relational Mode Engine (Core Logic)

```cpp
RelationalMode compute_relation(const PrimitiveState& a, const PrimitiveState& b) {
    float corr = correlation(a.intensity_history, b.intensity_history);
    float vel_sign = sign(a.velocity * b.velocity);

    if (corr > 0.7 && vel_sign > 0) return RelationalMode::Mirror;
    if (corr < -0.5) return RelationalMode::Counter;
    if (abs(corr) < 0.2) return RelationalMode::Orthogonal;

    if (a.state == State::EmulatedAnchor || b.state == State::EmulatedAnchor)
        return RelationalMode::MirrorAnchored;

    // Synth detection: emergent third metric
    if (has_emergent_third_metric(a, b)) return RelationalMode::Synth;

    return RelationalMode::Observer; // default conservative
}
```

---

## 3. Anchoring by Emulation (Key Mechanism)

```cpp
void engage_anchor_emulation(PrimitiveID anchor_prime, PrimitiveID target_to_stabilize) {
    // Temporarily inject a low-intensity, highly controlled version of anchor_prime
    // into the user's state representation.
    // This creates psychological predictability during volatile transitions.
    
    current_cycle.active_anchor = anchor_prime;
    apply_bounded_emulation(anchor_prime, intensity = 0.25, duration_limit);
}
```

**Jasterish-style expression (target long-term):**

```
emulate mild executive depletion as anchor
while masking load is rising rapidly
to create known ground during the drop
```

---

## 4. Basic Roller Coaster Cycle Controller

```cpp
void run_cycle(const CyclePlan& plan) {
    for (auto& segment : plan.segments) {
        transition_to(segment.primitive_or_cluster);
        apply_relational_aware_offsets(segment.dominant_relation);
        
        if (should_engage_anchor(segment)) {
            engage_anchor_emulation(segment.anchor_choice, segment.target);
        }
        
        wait_for_phase_exit_conditions();
        capture_mechanisms_in_between();  // record transitions + uplift signals
    }
}
```

---

## 5. Uplift Capture During Coast Phase

During the "Coast / Crystallization" phase, the system actively runs lightweight synthesis processes:
- Surface relevant historical primitive sequences
- Highlight contrasts between ascent and descent states
- Protect a minimum "insight window" with extremely low external demand

---

This gives the concrete, buildable patterns requested.

The primitives have been clustered, their relationships formalized through the requested modes (with compounds), anchoring by emulation is defined, and the mechanisms between states are explicitly captured in the cycle log.