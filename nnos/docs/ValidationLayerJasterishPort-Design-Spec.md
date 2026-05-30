# NNOS Validation Layer Port to Jasterish Design Specification

| Field | Value |
|---|---|
| Document ID | VL-JSTAR-PORT-DS-001 |
| Version | 1.0.0 |
| Date | 2026-03-20 |
| Status | DRAFT |
| Related Specs | VL-JSTAR-PORT-REQ-001 |
| Encoding | UTF-8 without BOM |

## 1. Architecture

The Jasterish port keeps the NNOS validation layer as a single deterministic guard function:

- Function: `validate_action(action, state, daemon) -> bool`
- Input: action identifier, `SharedState` snapshot, daemon or agent name
- Output: allow or deny, plus side effects such as tier escalation, deferral, and ledger append

The correct integration boundary is runtime action gating, not compiler-internal code generation. In practice:

- Ported NNOS daemons call `validate_action` before morph, threat, neuro-analysis, and quantum-morph actions.
- A future Jasterish runtime may expose `validate_action` as a standard guard primitive.
- The compiler may emit calls to this guard for annotated runtime actions, but it should not invoke the guard before every IR instruction emission.

Determinism requirements:

- Every decision uses compile-time thresholds plus the current `SharedState`.
- No network lookups, clocks, or non-seeded randomness in the hot path.
- Any ambiguous state defaults to deny plus escalation or deferral.

## 2. Decision Flow

```text
proc validate_action(action: str, state: SharedState, daemon: str) -> bool:
    if not verify_hmac(state.last_state_update, action, previous_hash):
        escalate_tier(state, 3)
        log("AUTH_FAIL_SIGNATURE")
        return false

    capability = registry_lookup(daemon, state.inferred_use_case_primary)
    if capability < 0.65:
        escalate_tier(state, 2)
        defer()
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
    threshold = get_use_case_threshold(state.inferred_use_case_primary)
    if energy > threshold:
        escalate_tier(state, 2)
        log("QMG_BLOCKED")
        return false

    density = query_7d_density(state)
    if density < 0.75:
        defer()
        return false

    append_to_ledger(action, state)
    return true
```

## 3. Bootstrap Strategy

For early Jasterish self-hosting, the validation layer should use:

- An in-memory registry table instead of PostgreSQL
- Fixed threshold tables instead of dynamic rule evolution
- Deterministic HMAC or hash chaining with local state only

That keeps the port small enough for bootstrap while preserving the intended NNOS governance model.

## 4. Exploration

- Possibility 1: Inline registry as a static array for the bootstrap compiler and early daemon ports
- Possibility 2: Replace the static registry with PostgreSQL or pgvector-backed lookups after the runtime stabilizes
- Possibility 3: Let morph logic tune guard thresholds only after deterministic replay and ledger validation are proven
