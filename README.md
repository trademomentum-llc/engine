# NNOS — Neurodivergent Neural-Link Operating System

**Deterministic, physiology-aware daemon constellation for neurodivergent operators.**

NNOS is a compiled C++ daemon suite that monitors physiological state, enforces task and context-switch budgets, detects burnout risk and social masking, and synchronizes encrypted state across heterogeneous nodes (NUC / Apple Silicon / Jetson Orin).

All runtime code is compiled. No interpreted Python or shell scripts are loaded in the critical path.

---

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Node Topologies                           │
├─────────────────┬─────────────────┬─────────────────────────┤
│   DCN (NUC)     │   HCN (M1 Mac)  │   EPN (Jetson Orin)    │
│                 │                 │                         │
│  lsa_boot_dcn   │  lsa_boot_hcn   │  lsa_boot_epn          │
│  ├─ task_mgr    │  ├─ comm_bridge │  ├─ state_monitor      │
│  ├─ context_gate│  ├─ profile_ref │  └─ ethernet_sync      │
│  └─ integrity   │  └─ convergence │                         │
│                 │                 │                         │
│  threat_intel   │  (global daemons│  drift_detector         │
│  morph_maintainer│  run on all)   │                         │
└─────────────────┴─────────────────┴─────────────────────────┘
                              │
                    UDP multicast 239.73.78.69:20046
                    AES-256-GCM + BLAKE3-32 MAC
                              │
                    ┌─────────────────────────┐
                    │   Shared-State TLV Log   │
                    │   (append-only, committed)│
                    └─────────────────────────┘
```

### Daemon Responsibilities

| Daemon | Node | Responsibility |
|--------|------|----------------|
| `lsa_boot_dcn` | NUC | Forks + supervises NUC-local children; restarts crashes within 2 s |
| `lsa_boot_hcn` | M1 | Forks + supervises M1-local children; cascading shutdown |
| `lsa_boot_epn` | Orin | Forks + supervises Orin-local children |
| `lsa_task_manager` | NUC | Task intake, scheduling, budget enforcement, breathing-room check |
| `lsa_context_gate` | NUC | Context-switch gating, hyperfocus protection, manual-execution policy |
| `lsa_state_monitor` | Orin | 4 Hz physiology ingestion → burnout risk → `InterventionTier` |
| `lsa_comm_bridge` | M1 | Request evaluation, anti-masking heuristic, negotiate/decline |
| `lsa_profile_refiner` | M1 | Daily 03:00 batch inference on 30-day logs → profile match |
| `lsa_drift_detector` | Global | Monitors all-node state for NDPL-rule divergence |
| `lsa_convergence_bond` | Global | Periodic determinism hash verification across nodes |
| `lsa_ethernet_sync` | Global | UDP multicast sync of TLV state segments (stubbed) |
| `SystemIntegrityDaemon` | Global | Node health monitoring, corruption/failure alerts |
| `ThreatIntelligenceManager` | Global | Threat indicator ingestion, NDPL correlation, risk scoring |
| `MorphogeneticMaintainer` | Global | Daily 04:00 repair cycle (detect→remove→restore→optimize→adapt) |

---

## Build

### Prerequisites

- Ubuntu 22.04+ / Jetson Linux 36.4+ / macOS 14+
- GCC 11+ or Clang 14+
- CMake 3.18+
- OpenSSL 3.0+ (for future crypto layer)
- pthreads

```bash
sudo apt update
sudo apt install -y build-essential cmake git libssl-dev
```

### Compile

```bash
cd nnos
cmake -B build -S .
cmake --build build -j$(nproc)
```

All 15 targets compile to `nnos/build/`:
- 3 boot daemons (`lsa_boot_*`)
- 5 functional daemons (`lsa_task_manager`, `lsa_context_gate`, `lsa_state_monitor`, `lsa_comm_bridge`, `lsa_profile_refiner`)
- 3 global daemons (`lsa_drift_detector`, `lsa_convergence_bond`, `lsa_ethernet_sync`)
- 3 security/morph daemons (`SystemIntegrityDaemon`, `ThreatIntelligenceManager`, `MorphogeneticMaintainer`)
- `libnnos_common.a` — shared state, IPC, logging, signal handling

### Smoke Test

```bash
export NNOS_LOG_DIR=/tmp/lsa_logs
mkdir -p $NNOS_LOG_DIR
./build/lsa_drift_detector
# ^C after a few seconds
cat /tmp/lsa_logs/lsa_drift_detector.jsonl
```

---

## Project Structure

```
engine/
├── nnos/
│   ├── CMakeLists.txt              # 15-target CMake build
│   ├── include/nnos/               # Core domain headers
│   │   ├── neuro_profile.hpp       # Packed NeuroProfile (< 256 B)
│   │   ├── task_manager.hpp        # Intake + scheduling logic
│   │   ├── state_monitor.hpp       # Physiology → InterventionTier
│   │   ├── context_gating.hpp      # Hyperfocus protection
│   │   ├── communication_bridge.hpp # Capacity/demand scoring
│   │   └── profile_matcher.hpp     # Weighted trait similarity
│   ├── src/common/                 # Shared library
│   │   ├── shared_state.hpp/cpp    # TLV wire format, append-only log
│   │   ├── ipc.hpp                 # Unix sockets, SHM, capability tokens
│   │   ├── daemon_base.hpp         # Signal handling, child supervision
│   │   ├── signal_handler.hpp/cpp  # SIGTERM/SIGINT/SIGHUP hooks
│   │   └── logger.hpp/cpp          # Structured JSONL logging
│   ├── src/daemons/                # Per-daemon entry points
│   ├── neurodios/                  # Sovereign platform kernel (Jasterish)
│   │   └── jasterish-microkernel/  # ~12.7k lines, 11 modules
│   ├── docs/                       # Full spec triads (REQ/DS/TECH)
│   ├── scripts/                    # Bootstrap, benchmark, propagate
│   └── recipes/                    # C validation recipes (20 files)
├── src/                            # Legacy LST engine (C)
├── recipes/                        # Denominator validation recipes
└── store/                          # Runtime artifact store (*.lst)
```

---

## Specifications

Full Requirements + Design + Technical Specification triads govern every subsystem:

| Subsystem | REQ | DS | TECH |
|-----------|-----|-----|------|
| Daemon Constellation | [NNOS-REQ-001](nnos/docs/2026-05-28-NNOS-Daemon-Constellation-Requirements.md) | [NNOS-DS-001](nnos/docs/2026-05-28-NNOS-Daemon-Constellation-Design-Specification.md) | [NNOS-TECH-001](nnos/docs/2026-05-28-NNOS-Daemon-Constellation-Technical-Specification.md) |
| Jasterish Micro-Kernel | [JMK-REQ](nnos/neurodios/docs/2026-05-28-NeuroDiOS-Jasterish-Micro-Kernel-Requirements.md) | [JMK-DS](nnos/neurodios/docs/2026-05-28-NeuroDiOS-Jasterish-Micro-Kernel-Design-Specification.md) | [JMK-TECH](nnos/neurodios/docs/2026-05-28-NeuroDiOS-Jasterish-Micro-Kernel-Technical-Specification.md) |
| Behavioral Health Primitives | [BHP-REQ](nnos/docs/2026-05-29-Behavioral-Health-Condition-Primitives-Requirements.md) | [BHP-DS](nnos/docs/2026-05-29-Behavioral-Health-Condition-Primitives-Design-Specification.md) | [BHP-TECH](nnos/docs/2026-05-29-Behavioral-Health-Condition-Primitives-Technical-Specification.md) |
| Roller Coaster Framework | [RCF-REQ](nnos/neurodios/docs/2026-05-29-NeuroDiOS-Roller-Coaster-Framework-Requirements.md) | [RCF-DS](nnos/neurodios/docs/2026-05-29-NeuroDiOS-Roller-Coaster-Framework-Design-Specification.md) | [RCF-TECH](nnos/neurodios/docs/2026-05-29-NeuroDiOS-Roller-Coaster-Framework-Technical-Specification.md) |

See [`nnos/docs/GAP_ANALYSIS_AND_ROADMAP.md`](nnos/docs/GAP_ANALYSIS_AND_ROADMAP.md) for prioritized next steps (P0–P6).

---

## Release Artifacts

Compiled binaries, reference PDFs, spreadsheets, and firmware images are **not tracked in git**. They are published as GitHub Release assets:

- Daemon ELF binaries per platform (`x86_64-linux`, `aarch64-linux`, `darwin-arm64`)
- LSA reference PDFs (`LSA-Spec-v2.0.0.pdf`, `NeuroDiv` series, etc.)
- Primitive mapping spreadsheets
- Android factory images (caiman ZIP)
- `store/*.lst` runtime artifacts

---

## Determinism

All shared-state transitions must be deterministic given identical inputs + versioned NDPL rules. The benchmark script verifies this:

```bash
./nnos/scripts/benchmark_determinism.sh <command> <fixture>
```

---

## Roadmap

1. **Wire event loops** — Replace stub loops with real shared-state I/O
2. **Ethernet sync** — Implement UDP multicast `239.73.78.69:20046` + AES-256-GCM
3. **systemd/quadlet** — Generate per-node service definitions from CMake
4. **Jasterish port** — Link validation and morph layers against JStar runtime
5. **QEMU boot** — Bring up the ~12.7k-line microkernel in emulation
6. **Self-host fixpoint** — Verify `jstar1 → jstar2 → jstar3` T-diagram on NUC

---

*NNOS is research software under active development. See [`nnos/PROJECT_SUMMARY.md`](nnos/PROJECT_SUMMARY.md) for strategic context and [`nnos/START_HERE.md`](nnos/START_HERE.md) for first-time implementation guidance.*
