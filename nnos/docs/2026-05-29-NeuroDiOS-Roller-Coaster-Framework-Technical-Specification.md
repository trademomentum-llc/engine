# Technical Specification: NeuroDiOS Roller Coaster Framework

**Document ID:** NEURODIOS-RCF-TS-001  
**Version:** 1.0.0  
**Date:** 2026-05-29

---

## 1. Core Data Models

```cpp
enum class RelationalMode {
    Mirror,
    Counter,
    Orthogonal,
    Synth,
    Observer,
    MirrorCounter,
    MirrorAnchored,
    // ... other compounds
};

struct PrimitiveCluster {
    string name;
    vector<PrimitiveID> members;
};

struct CycleSegment {
    PrimitiveID or ClusterID active;
    RelationalMode dominant_relation;
    Optional<PrimitiveID> anchor_emulated;
    float intensity_curve; // 0.0 to 1.0 over time
    vector<AdaptationAction> active_offsets;
};

struct RollerCoasterCycle {
    vector<CycleSegment> segments;
    Phase current_phase;
    vector<UpliftEvent> captured_insights;
    // ...
};
```

---

## 2. Relational Mode Computation (Simplified Deterministic Rules)

- **Mirror**: High positive correlation in intensity + similar fluctuation velocity sign.
- **Counter**: High negative correlation or one rising while the other is being actively offset.
- **Synth**: Sudden jump in a third derived metric when both are elevated together.
- **Observer**: One primitive's state changes reliably precede changes in another with high consistency.
- **Anchored**: One primitive is being artificially emulated (low intensity, high control) to stabilize another.

---

## 3. Example Cycle (Conceptual)

**Goal**: Generate insight on a stuck creative problem while user has moderate Masking + rising Executive Depletion.

1. **Coast** → Baseline
2. **Ascent** → Controlled increase in Context Switching (BH-04) + mild Sensory Load (BH-03)  [Load Cluster]
3. **Peak** → Add Masking Load (BH-02) under observation  [Volatility Cluster]
4. **Anchor** → Emulate mild Executive Depletion (BH-01) as ballast
5. **Descent** → Rapid drop of all load + protected low-demand period
6. **Coast + Capture** → System surfaces relevant past states + protects space for insight

Relational modes observed: Mirror (between 03 and 04), Counter (anchored 01 vs 02), Synth at peak.

---

## 4. Integration Requirements

The Roller Coaster Framework must be able to:
- Read current primitive states and relational graph from shared state.
- Propose or accept cycle structures.
- Command adaptation actions through the existing daemon interfaces.
- Write detailed Cycle Logs back into shared state for Morphogenetic analysis.

---

**End of Technical Specification**