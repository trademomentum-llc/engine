# NNOS Full Specification Chain
# Consolidated from Jason's session - March 20, 2026
# Covers: VM, Docker, Podman, Benchmarks, Daemons, Behavioral Health,
#         Dimensional Intelligence, Authority/Validation, Quantum Morph,
#         Jasterish Compiler, 180 Neurodivergent Patterns, Morphlex AST,
#         Codegen, Morph Engine Port

## Document Registry

| ID | Title | Version |
|----|-------|---------|
| VM-REQ-001 | VM Module Requirements | 1.0.0 |
| VM-DS-001 | VM Design Spec | 1.0.0 |
| VM-TECH-001 | VM Technical Spec | 1.0.0 |
| DOCKER-REQ-001 | Docker Requirements | 1.0.0 |
| DOCKER-DS-001 | Docker Design Spec | 1.0.0 |
| DOCKER-TECH-001 | Docker Technical Spec | 1.0.0 |
| PODMAN-REQ-001 | Podman Rootless Requirements | 1.0.0 |
| PODMAN-DS-001 | Podman Design Spec | 1.0.0 |
| PODMAN-TECH-001 | Podman Technical Spec | 1.0.0 |
| BENCH-REQ-001 | Determinism Benchmark Requirements | 1.2.0 |
| BENCH-DS-001 | Benchmark Design Spec | 1.2.0 |
| BENCH-TECH-001 | Benchmark Technical Spec | 1.2.0 |
| MORPH-REQ-001 | Morphogenetic Daemon Requirements | 1.0.0 |
| THREAT-REQ-001 | Threat Intelligence Daemon Requirements | 1.0.0 |
| NEURO-REQ-001 | Neuro-Analysis Daemon Requirements | 1.0.0 |
| QMORPH-REQ-001 | Quantum Morphogenesis Requirements | 1.0.0 |
| TEST-REQ-001 | Testing Module Requirements | 1.0.0 |
| ADV-REQ-001 | Adversarial Testing Requirements | 1.0.0 |
| INT-REQ-001 | Inter-Daemon Integration Requirements | 1.0.0 |
| BEHAV-REQ-001 | Behavioral Health Intelligence Requirements | 1.0.0 |
| DIM-REQ-001 | Dimensional Intelligence (Obnoxious Data Table) Requirements | 1.0.0 |
| VDB-REQ-001 | VectorDB Selection Requirements | 1.0.0 |
| AUTH-REQ-001 | Authority Validation Registry Requirements | 1.0.0 |
| AUTH-DS-002 | Validation Algorithms Design | 1.1.0 |
| AUTH-TECH-002 | Validation Algorithms Implementation | 1.1.0 |
| QMG-REQ-001 | Quantum Morph Energy Gates Requirements | 1.0.0 |
| JSTAR-DIAG-001 | Jasterish Bootstrap Diagnosis | 1.0.0 |
| JSTAR-EXT-REQ-001 | Jasterish Primitive Extraction Requirements | 1.0.0 |
| NDPL-REQ-003 | 180 Neurodivergent Patterns Library | 3.0.0 |
| JSTAR-EXT-ENG-REQ-001 | Jasterish Extraction Engine Requirements | 1.0.0 |
| AST-REQ-001 | Morphlex AST Builder Requirements | 1.0.0 |
| JSTAR-CG-DIAG-REQ-001 | Jasterish Codegen Diagnosis | 1.0.0 |
| VL-JSTAR-PORT-REQ-001 | Validation Layer Jasterish Port | 1.0.0 |
| MORPH-JSTAR-PORT-REQ-001 | Morph Engine Jasterish Port | 1.0.0 |

## Key Architecture Decisions

- Hypervisor: KVM/QEMU (no Docker)
- Container: Podman rootless preferred
- VectorDB: pgvector + pgvectorscale on NUC PostgreSQL
- Pattern Library: 180 neurodivergent patterns (9 categories x 20)
- 7D Clustering: sensory, executive, hyperfocus, masking, autonomic, social, morph
- Language Target: Jasterish (deterministic single-binary machine language)
- Bootstrap: jstarN == jstarN+1 stability required before NNOS port

## Daemon Constellation (Expanded)

| Daemon | Role | Node |
|--------|------|------|
| nnos_boot_daemon | Supervisor | All |
| nnos_state_monitor | Physiological sampling | Orin/EPN |
| nnos_task_manager | Task intake/scheduling | NUC/DCN |
| nnos_context_gate | Context switch protection | NUC/DCN |
| nnos_comm_bridge | Capacity/demand scoring | M1/HCN |
| nnos_profile_refiner | Daily trait adjustment | M1/HCN |
| nnos_ethernet_sync | Multi-device state sync | All |
| nnos_morph_engine | Adaptive trait evolution | M1/HCN |
| nnos_threat_scanner | Security monitoring | NUC/DCN |
| nnos_neuro_analyzer | ML-driven neuro insights | M1/HCN |
| nnos_quantum_morph | Quantum-inspired evolution | M1/HCN + Orin |

## SharedState Extensions

```cpp
struct alignas(64) SharedState {
    // Core fields (existing)
    // ...
    // New integration fields
    std::atomic<float> neuro_insight_score;
    std::atomic<float> threat_anomaly_score;
    std::atomic<uint8_t> morph_trigger;
    std::atomic<float> qmorph_energy_min;
    std::atomic<uint8_t> inferred_use_case_primary;
    std::atomic<float> use_case_confidence;
    std::atomic<float> load_deviation;
    std::atomic<float> cluster_density;
};
```

## Validation Layer Decision Tree

1. Root of Authority signature verification (HMAC-SHA256)
2. Conditions Registry capability lookup (>= 0.65)
3. Neuro-specific behavioral guards (hyperfocus/masking/tier)
4. Quantum morph energy gate (energy < threshold)
5. 7D pattern density approval (density >= 0.75)
6. Final: allow / escalate + defer + log

## Energy Gate Formula

```
morph_energy = 0.35 * load_deviation
             + 0.25 * threat_anomaly_score
             + 0.20 * (1.0 - neuro_insight_score)
             + 0.20 * (1.0 - cluster_density)
```

Thresholds by use case:
- HYPERFOCUS_DOMINANT: 0.65
- SENSORY_OVERLOAD: 0.45
- Default: 0.70
