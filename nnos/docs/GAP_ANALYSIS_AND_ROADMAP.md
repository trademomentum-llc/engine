# NNOS Gap Analysis & Implementation Roadmap

**Date:** 2026-05-30
**Scope:** Synthesis of `nnos-lsa` → `engine/nnos` + gap analysis between specs and implementation

---

## 1. File Synthesis Complete

Artifacts from `~/Projects/nnos-lsa` have been organized into `~/Projects/engine/nnos`:

| Source | Destination | Notes |
|--------|-------------|-------|
| `implementation.md` | `docs/security/LSA-SPEC-002-AMD-001-gap-remediation.md` | Security remediation spec |
| `socketmon.md` | `docs/daemons/socketmon-design.md` | Socket monitoring daemon design |
| `neurobalance-engine.py` | `neurobalance/neurobalance_engine.py` | Original 1,101-line engine |
| `Qwen_*.rs.txt` | `stubs/lsa_state_monitor.rs.stub` | Rust daemon stub (52 lines) |
| `Qwen_*.toml` | `stubs/Cargo.toml.stub` | Cargo manifest stub |
| `Qwen_*.ini` | `stubs/lsa_state_monitor.systemd.stub` | systemd unit stub |
| `Qwen_*.xml` | `stubs/lsa_state_monitor.launchd.stub` | launchd plist stub |
| `Qwen_*.json` | `stubs/primitive_map.json.stub` | J→atomic dependency manifest |
| `Qwen_*.sh` | `stubs/*.sh.stub` | Build/install script stubs |
| PDFs/XLSX/DOCX | `references/` | Reference documents |
| `NeuroDiv.md` | **DELETED** (duplicate of `docs/NeuroDiv.md`) | Identical file |

---

## 2. What EXISTS (Implementation Inventory)

### 2.1 C++ Header-Only Demo (863 lines, not daemons)

**Status:** Single-process demo (`boot.cpp` + `main_engine.cpp`). Not compiled daemons. No `CMakeLists.txt`.

| File | Lines | What It Does | Gap |
|------|-------|--------------|-----|
| `neuro_profile.hpp` | 273 | 4 compile-time neurodivergent profile templates (Systems Hyperfocus, Divergent Creative, Sensory-Social Fragile, Intense Mood Variance) with trait levels, strength/vulnerability bitflags, thresholds, time slots | Not a daemon; no runtime calibration |
| `profile_matcher.hpp` | 100 | Weighted similarity matching (Jaccard + trait distance) against templates | Not a daemon; no live data ingestion |
| `task_manager.hpp` | 146 | Task intake with strength/risk matching, capacity checks, slot scheduling (deep/light/supported) | Not a daemon; no persistence, no IPC |
| `state_monitor.hpp` | 98 | Physiological state → sensory/emotional load → intervention tier (NONE/ADJUST/GUIDED/EMERGENCY) | Not a daemon; `apply_tier1/2/3` are empty stubs |
| `context_gating.hpp` | 104 | Hyperfocus protection, context switch budget enforcement, handoff note generation | Not a daemon; `current_switch_count_` not persisted |
| `communication_bridge.hpp` | 142 | Request evaluation (ACCEPT/NEGOTIATE_SCOPE/NEGOTIATE_DEADLINE/DECLINE), anti-masking detection | Not a daemon; no actual network I/O |

**Total:** ~863 lines of header-only C++ that compiles to a demo, not a daemon constellation.

### 2.2 Python Implementations

| File | Lines | What It Does | Gap |
|------|-------|--------------|-----|
| `neurobalance/neurobalance_engine.py` | 1,101 | Original state assessment engine: primitive extraction, mechanism derivation, state scoring, gap identification, action recommendations from signal events | Not integrated with C++ daemons; no IPC |
| `neurobalance/neurobalance_coordinator.py` | 278 | Coordinator layer on top of engine: 5 Validated Denominators, Roller Coaster phase awareness, offset engines, uplift opportunity generation | Not integrated with C++ daemons; no IPC |

### 2.3 Jasterish Microkernel (~12,700 lines)

| Module | Status |
|--------|--------|
| `boot.jstr` | ✅ Multiboot2 entry, GDT, long mode transition, stack setup |
| `kernel.jstr` | ✅ Main kernel loop, panic handler, timer IRQ |
| `memory.jstr` | ✅ PMM bitmap, VMM 4-level paging, COW fork, kernel heap |
| `idt.jstr` | ✅ 256-entry IDT, exception handlers, IRQ dispatcher, COW page fault |
| `process.jstr` | ✅ PCB arrays, scheduler, `process_create_user`, `init_user_process` |
| `syscall.jstr` | ✅ Expanded syscall table, shell commands |
| `elf.jstr` | ✅ ELF loader, `sys_exec` |
| `vfs.jstr` | ✅ Simple VFS/RAMFS |
| `disk.jstr` | ✅ ATA PIO disk driver with persistence |
| `drivers.jstr` | ✅ UART, keyboard, timer |
| `ipc.jstr` | ✅ Inter-process communication primitives |

**Status:** Structurally complete but not yet booting in QEMU (per TODO.md).

### 2.4 Build & Tooling

| File | Status |
|------|--------|
| `scripts/bootstrap_encoding.sh` | ✅ UTF-8 normalization |
| `scripts/propagate_nnos.sh` | ✅ Environment validation (native/VM/container detection) |
| `scripts/benchmark_determinism.sh` | ✅ Determinism measurement harness |
| `neurodios/jasterish-microkernel/Makefile` | ✅ Kernel build system |
| `CMakeLists.txt` | ❌ **MISSING** |
| `systemd/` | ❌ **EMPTY** |
| `firmware/src/` | ❌ **EMPTY** |
| `bootloader/` | ❌ **EMPTY** |

---

## 3. What DOES NOT EXIST (The Gaps)

### 3.1 Baseline Daemon Constellation (6 Daemons)

From `NNOS-REQ-001` / `NNOS-DS-001` / `NNOS-TECH-001`:

| Daemon | Spec Node | Status | Blocker |
|--------|-----------|--------|---------|
| `lsa_boot_dcn` | NUC | ❌ **Zero code** | Needs CMake, systemd, shared-state IPC |
| `lsa_boot_hcn` | M1 | ❌ **Zero code** | Needs launchd, shared-state IPC |
| `lsa_boot_epn` | Orin | ❌ **Zero code** | Needs systemd, sensor I/O, shared-state IPC |
| `SystemIntegrityDaemon` | NUC | ❌ **Zero code** | Needs PQC signatures, file attestation |
| `ThreatIntelligenceManager` | NUC/M1 | ❌ **Zero code** | Needs 180 NDPL rule engine, anomaly detection |
| `MorphogeneticMaintainer` | NUC | ❌ **Zero code** | Needs Jasterish port, repair cycle scheduler |

### 3.2 LSA-SPEC-002 Daemons (9 Daemons)

From `NeuroDiv.md` (LSA-SPEC-002 v2.0.0):

| Daemon | Node | Spec Status | Impl Status | Notes |
|--------|------|-------------|-------------|-------|
| `lsa_boot` (all variants) | All | ✅ Spec'd | ❌ No code | Master supervision daemon |
| `lsa_task_manager` | NUC | ✅ Spec'd | ⚠️ Partial | C++ `TaskManager` header exists (146 lines) but not a daemon |
| `lsa_context_gate` | NUC | ✅ Spec'd | ⚠️ Partial | C++ `ContextGating` header exists (104 lines) but not a daemon |
| `lsa_drift_detector` | NUC | ✅ Spec'd | ❌ No code | Rolling window drift analysis |
| `lsa_state_monitor` | Orin | ✅ Spec'd | ⚠️ Partial | C++ `StateMonitor` header + Python engine exist |
| `lsa_convergence_bond` | M1 | ✅ Spec'd | ❌ No code | Cosine similarity domain overlap |
| `lsa_profile_refiner` | M1 | ✅ Spec'd | ⚠️ Partial | C++ `ProfileMatcher` exists (100 lines) but not a daemon |
| `lsa_comm_bridge` | M1 | ✅ Spec'd | ⚠️ Partial | C++ `CommunicationBridge` header exists (142 lines) but not a daemon |
| `lsa_ethernet_sync` | All | ✅ Spec'd | ❌ No code | UDP multicast 239.73.78.69:20046 + AES-256-GCM |

### 3.3 Shared Infrastructure

| Component | Status | Priority |
|-----------|--------|----------|
| **Shared-state TLV log + snapshot** | ❌ Not implemented | P0 — blocks all daemons |
| **Ethernet sync (UDP multicast + AES-256-GCM)** | ❌ Not implemented | P0 — blocks multi-node |
| **Jasterish validation runtime guard** | ❌ Design artifact only | P1 — blocks Jasterish port |
| **Jasterish morphogenetic repair engine** | ❌ Design artifact only | P1 — blocks self-healing |
| **180 NDPL pattern library** | ❌ Not materialized | P1 — blocks masking detection |
| **Bootstrap registry** | ❌ Stub only | P2 — blocks rule loading |
| **CMakeLists.txt** | ❌ Missing | P0 — blocks compilation |
| **systemd units** | ❌ Empty directory | P1 — blocks deployment |
| **Dockerfiles / Podman quadlets** | ❌ Missing | P2 — blocks containerization |
| **QEMU bring-up for microkernel** | ❌ Not working | P0 — blocks kernel validation |

---

## 4. Implementation Roadmap

### Phase 0: Foundation (Unblocks Everything)

**Goal:** Get the build system working and the microkernel booting.

| Task | Effort | Owner | Deliverable |
|------|--------|-------|-------------|
| P0.1 Write `CMakeLists.txt` with all 6 baseline targets | 2-3h | Dev | Buildable daemon constellation |
| P0.2 Get Jasterish microkernel booting in QEMU via Makefile | 4-6h | Dev | `make qemu` succeeds |
| P0.3 Integrate compiler fixes from `apps/` into `engine/nnos` | 1-2h | Dev | `compiler.jstr` builds clean |
| P0.4 Populate `systemd/` with real units from stubs | 2h | Dev | 6 `.service` files |

### Phase 1: Shared State & Sync (Unblocks Multi-Node)

**Goal:** Daemons can communicate.

| Task | Effort | Owner | Deliverable |
|------|--------|-------|-------------|
| P1.1 Implement shared-state TLV log + snapshot (C++) | 6-8h | Dev | `libnnos_state.so` |
| P1.2 Implement UDP multicast sync (239.73.78.69:20046) | 4-6h | Dev | `lsa_ethernet_sync` binary |
| P1.3 Wire AES-256-GCM encryption around sync payload | 4h | Dev | Encrypted multicast |
| P1.4 Implement Unix domain socket IPC for local daemons | 3h | Dev | Capability-token IPC |

### Phase 2: Boot Daemon & Supervision (The Root Process)

**Goal:** One daemon starts all others and restarts crashes.

| Task | Effort | Owner | Deliverable |
|------|--------|-------|-------------|
| P2.1 Implement `lsa_boot_dcn` (NUC master) | 4-6h | Dev | Forks children, watches PIDs, restarts |
| P2.2 Implement `lsa_boot_hcn` (M1) | 3-4h | Dev | launchd-compatible or standalone |
| P2.3 Implement `lsa_boot_epn` (Orin) | 3-4h | Dev | Sensor ingestion startup |
| P2.4 SIGTERM cascading shutdown | 2h | Dev | Clean shutdown within 10s |

### Phase 3: Core Daemons (The Baseline 6)

**Goal:** All spec'd daemons are running.

| Task | Effort | Owner | Deliverable |
|------|--------|-------|-------------|
| P3.1 Port C++ `TaskManager` → `lsa_task_manager` daemon | 3-4h | Dev | systemd service, shared-state I/O |
| P3.2 Port C++ `ContextGating` → `lsa_context_gate` daemon | 3-4h | Dev | systemd service, shared-state I/O |
| P3.3 Port C++ `StateMonitor` + Python engine → `lsa_state_monitor` | 6-8h | Dev | Real sensor I/O, 4 Hz sampling |
| P3.4 Port C++ `CommunicationBridge` → `lsa_comm_bridge` | 3-4h | Dev | Network request evaluation |
| P3.5 Port C++ `ProfileMatcher` → `lsa_profile_refiner` | 4-6h | Dev | Daily 03:00 batch, MLX optional |
| P3.6 Implement `lsa_drift_detector` | 4-6h | Dev | PostgreSQL queries, rolling window |
| P3.7 Implement `lsa_convergence_bond` | 4-6h | Dev | Cosine similarity, domain overlap |

### Phase 4: Security & Intelligence (The Extended 3)

**Goal:** Integrity, threat detection, self-healing.

| Task | Effort | Owner | Deliverable |
|------|--------|-------|-------------|
| P4.1 Implement `SystemIntegrityDaemon` | 4-6h | Dev | File attestation, PQC signatures |
| P4.2 Implement `ThreatIntelligenceManager` | 6-8h | Dev | 180 NDPL rule engine, masking detection |
| P4.3 Implement `MorphogeneticMaintainer` | 8-12h | Dev | Daily repair cycle, glial metaphor |

### Phase 5: Jasterish Port (Sovereign Path)

**Goal:** Critical layers run on the custom kernel.

| Task | Effort | Owner | Deliverable |
|------|--------|-------|-------------|
| P5.1 Port validation runtime to JStar | 8-12h | Dev | `nnos_validate_action` as runtime guard |
| P5.2 Port morphogenetic engine to JStar | 12-16h | Dev | JStar-executable repair plans |
| P5.3 Self-host ladder: compiler.jstr → jstar1 → jstar2 | 4-6h | Dev | Verified on NUC Linux |
| P5.4 Link validation runtime against JStar-emitted code | 6-8h | Dev | Runtime guard integrated |

### Phase 6: Deployment Hardening

**Goal:** Production-ready packaging.

| Task | Effort | Owner | Deliverable |
|------|--------|-------|-------------|
| P6.1 Docker multi-stage builds | 3-4h | Dev | `Dockerfile` per daemon |
| P6.2 Podman rootless quadlets | 3-4h | Dev | `.container` + `.service` quadlets |
| P6.3 Smoke tests on NUC (Ubuntu 24.04) | 4h | QA | Verified boot + sync |
| P6.4 Smoke tests on Orin (JetPack 6.x) | 4h | QA | Verified sensor pipeline |
| P6.5 Determinism benchmarks wired | 2h | Dev | `benchmark_determinism.sh` canonical |

---

## 5. Immediate Next Steps (What to Do Today)

The highest-leverage tasks right now:

1. **Write `CMakeLists.txt`** — Unblocks compilation of all 6 baseline daemons from existing headers
2. **Promote C++ headers to daemon targets** — Wrap `TaskManager`, `ContextGating`, `StateMonitor`, `CommunicationBridge`, `ProfileMatcher` into `main()` loops with shared-state I/O
3. **Implement shared-state TLV** — This is the single dependency blocking all daemon IPC
4. **Get microkernel QEMU boot working** — Unblocks the sovereign Jasterish path

**Recommended order:** P0.1 → P0.4 → P1.1 → P2.1 → P3.1/P3.2 (parallel)

---

## 6. Cross-Reference Map

| Spec Document | Impl Location | Gap |
|---------------|---------------|-----|
| `docs/2026-05-28-NNOS-Daemon-Constellation-Requirements.md` | C++ headers (demo) | Not compiled daemons |
| `docs/2026-05-28-NNOS-Daemon-Constellation-Design-Specification.md` | C++ headers (demo) | No IPC, no persistence |
| `docs/2026-05-28-NNOS-Daemon-Constellation-Technical-Specification.md` | Stubs only | No CMake, no systemd |
| `docs/NeuroDiv.md` (LSA-SPEC-002) | C++ headers + Python | No daemon processes |
| `docs/security/LSA-SPEC-002-AMD-001-gap-remediation.md` | Stubs only | No hardening implemented |
| `docs/daemons/socketmon-design.md` | Zero code | Not started |
| `neurodios/docs/2026-05-28-NeuroDiOS-Jasterish-Micro-Kernel-*.md` | `jasterish-microkernel/` | Not booting in QEMU yet |

---

*End of Gap Analysis & Roadmap*
