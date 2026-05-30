# NNOS Validation Layer Port to Jasterish Technical Specification

| Field | Value |
|---|---|
| Document ID | VL-JSTAR-PORT-TECH-001 |
| Version | 1.0.0 |
| Date | 2026-03-20 |
| Status | DRAFT |
| Related Specs | VL-JSTAR-PORT-DS-001 |
| Encoding | UTF-8 without BOM |

## 1. Core Guard Function

```text
struct SharedState {
    intervention_tier: u8,
    hyperfocus_minutes: u32,
    masking_alert: u8,
    threat_anomaly_score: f32,
    ctx_switches_this_hour: u32,
    max_ctx_switches_per_hour: u32,
    inferred_use_case_primary: u8,
    load_deviation: f32,
    neuro_insight_score: f32,
    cluster_density: f32,
    qmorph_energy_min: f32,
    last_state_update: [u8; 32],
}

proc validate_action(action: str, state: SharedState, daemon: str) -> bool:
    if not verify_hmac(state.last_state_update, action):
        escalate_tier(state, 3)
        log_event("AUTH_FAIL_SIGNATURE", action)
        return false

    capability = registry_get(daemon, state.inferred_use_case_primary)
    if capability < 0.65:
        escalate_tier(state, 2)
        defer_action()
        return false

    if state.hyperfocus_minutes > 90 and state.intervention_tier >= 2:
        if action == "morph_evolve":
            return false

    if state.masking_alert == 1 and state.threat_anomaly_score > 0.4:
        if action != "recovery_prompt":
            escalate_tier(state, 3)
            return false

    if state.ctx_switches_this_hour >= state.max_ctx_switches_per_hour:
        return false

    energy = compute_morph_energy(state)
    threshold = use_case_threshold(state.inferred_use_case_primary)
    if energy > threshold:
        escalate_tier(state, 2)
        log_event("QMG_BLOCKED", energy, threshold)
        return false

    density = get_7d_density(state)
    if density < 0.75:
        defer_action()
        return false

    append_ledger(action, state)
    return true
```

## 2. Energy Gate

```text
proc compute_morph_energy(state: SharedState) -> f32:
    e = 0.0
    e += 0.35 * state.load_deviation
    e += 0.25 * state.threat_anomaly_score
    e += 0.20 * (1.0 - state.neuro_insight_score)
    e += 0.20 * (1.0 - state.cluster_density)
    return clamp(e, 0.0, 1.0)

proc use_case_threshold(use_case: u8) -> f32:
    match use_case:
        HYPERFOCUS_DOMINANT => 0.65
        SENSORY_OVERLOAD => 0.45
        _ => 0.70
```

## 3. Bootstrap-Friendly Registry

```text
const REGISTRY_ENTRIES: [(str, u8, f32)] = [
    ("nnos_morph_engine", HYPERFOCUS_DOMINANT, 0.82),
]

proc registry_get(daemon: str, use_case: u8) -> f32:
    for entry in REGISTRY_ENTRIES:
        if entry.0 == daemon and entry.1 == use_case:
            return entry.2
    return 0.0
```

## 4. Integration Notes

- The first implementation target should be a Jasterish runtime or support module used by ported NNOS daemons.
- Compiler integration should emit calls to `validate_action` only around guarded runtime actions, not around every compiler instruction.
- Ledger append should hash the action identifier plus a canonical `SharedState` snapshot.
- PostgreSQL and pgvector lookups should stay stubbed during bootstrap until self-hosting is stable on Linux.
