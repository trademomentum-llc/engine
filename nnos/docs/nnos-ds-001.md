```markdown
# NNOS -- Design Specification
## Neurodivergent Neural-Link Operating System Architecture

| Field | Value |
|---|---|
| Document ID | NNOS-DS-001 |
| Version | 2.0.0 |
| Date | 2026-02-23 |
| Status | APPROVED |
| Related Spec | NNOS-SRS-001 |
| Encoding | UTF-8 without BOM, ASCII-safe punctuation |

---

## 1. Architectural Overview

### 1.1 Process Topology

NNOS is implemented as a constellation of cooperating C++ daemons on a single Linux host.

Processes:

- `nnos_boot_daemon`
- `nnos_state_monitor`
- `nnos_task_manager`
- `nnos_context_gate`
- `nnos_comm_bridge`
- `nnos_profile_refiner`
- `nnos_ethernet_sync`

All daemons:

- Attach to a single POSIX shared memory object (`/dev/shm/nnos_neural_link`).
- Operate on a shared `SharedState` struct using atomic fields.
- Communicate via SharedState and structured JSON logs, not sockets or pipes. [web:210]

Design goals:

- Deterministic behavior under load.
- Zero dynamic allocation in hot loops.
- Lock-free cross-process communication.
- Clear separation of concerns per daemon.

### 1.2 High-Level Data Flow

1. `nnos_boot_daemon`:
   - Creates and initializes SharedState.
   - Runs profile matching.
   - Spawns and supervises child daemons.

2. `nnos_state_monitor`:
   - Reads sensors and self-report inputs.
   - Computes `sensory_load`, `emotional_load`, `total_load`.
   - Sets `intervention_tier` and updates timestamps in SharedState.

3. `nnos_task_manager`:
   - Receives task intake events (via local IPC or test harness).
   - Reads current tier, task counts, slots, and profile thresholds.
   - Accepts or defers tasks and updates `active_task_count`.

4. `nnos_context_gate`:
   - Tracks context switches and hyperfocus state.
   - Applies budget and hyperfocus rules.
   - Logs protect/break decisions.

5. `nnos_comm_bridge`:
   - Reads SharedState, plus incoming request descriptors.
   - Computes capacity and demand scores.
   - Sets `masking_alert` and emits communication decisions.

6. `nnos_profile_refiner`:
   - Reads logs from `/var/log/nnos/events.jsonl`.
   - Computes behavior statistics and adjusts traits.
   - Writes refined profile and updates `last_profile_update`.

7. `nnos_ethernet_sync`:
   - Periodically serializes SharedState summary.
   - Encrypts and multicasts summary to peers.
   - Receives, authenticates, and merges remote state.
   - Serves a TCP polling endpoint for mobile clients.

---

## 2. SharedState Design

### 2.1 Layout

`SharedState` is a POD struct with explicit field ordering and padding to avoid false sharing. Alignment is 64 bytes. [web:216]

Key design properties:

- Fixed-size (no variable-length fields).
- All cross-process mutable fields are `std::atomic<T>`.
- Header with magic and version for validation.
- Reserved region for future fields.

Simplified C++ definition:

```cpp
namespace nnos {

struct alignas(64) SharedState {
    // Header
    uint64_t magic;      // 0x4E4E4F534C494E4B ("NNOSLINK")
    uint32_t version;    // 1
    uint32_t reserved0;

    // Signals
    std::atomic<float> noise_level;
    std::atomic<float> light_level;
    std::atomic<uint16_t> notifications_count;
    std::atomic<float> heart_rate_variability;
    std::atomic<uint8_t> self_report_overwhelm;
    uint8_t padding0[1];

    // Profile identity
    std::atomic<uint8_t> active_profile_id;
    std::atomic<float> profile_confidence;
    uint8_t reserved_profile[1];

    // Traits
    std::atomic<uint8_t> need_for_structure;
    std::atomic<uint8_t> novelty_seeking;
    std::atomic<uint8_t> hyperfocus_inclination;
    std::atomic<uint8_t> sensory_sensitivity;
    std::atomic<uint8_t> social_energy_capacity;
    std::atomic<uint8_t> exec_function_difficulty;
    uint8_t padding1[2];

    // Thresholds / limits
    std::atomic<uint8_t> max_concurrent_tasks;
    std::atomic<uint8_t> max_ctx_switches_per_hour;
    std::atomic<uint16_t> deep_work_block_min;
    std::atomic<uint16_t> light_work_block_min;
    std::atomic<uint16_t> min_recovery_block_min;
    std::atomic<uint16_t> max_social_min_per_day;
    std::atomic<float> sensory_alert_threshold;

    // Runtime state
    std::atomic<uint8_t> active_task_count;
    std::atomic<uint8_t> ctx_switches_this_hour;
    std::atomic<uint16_t> social_minutes_today;
    std::atomic<uint8_t> intervention_tier;
    std::atomic<uint8_t> hyperfocus_active;
    std::atomic<uint16_t> hyperfocus_minutes;
    std::atomic<uint8_t> masking_alert;
    uint8_t padding2[1];

    // Timestamps (seconds)
    std::atomic<uint32_t> last_state_update;
    std::atomic<uint32_t> last_profile_update;
    std::atomic<uint32_t> last_task_event;
    std::atomic<uint32_t> boot_timestamp;

    // Reserved
    uint8_t reserved_region;
};

} // namespace nnos
```

### 2.2 Initialization

Performed only by `nnos_boot_daemon`:

- Create shared memory object via `shm_open`.
- `ftruncate` to sizeof(SharedState).
- `mmap` with `PROT_READ | PROT_WRITE`, `MAP_SHARED`.
- `std::memset` to zero.
- Write:
  - `magic`.
  - `version`.
  - `boot_timestamp`.

Child daemons:

- Use `shm_open` + `mmap`.
- Validate:
  - `magic == 0x4E4E4F534C494E4B`.
  - `version` in [1, MAX_SUPPORTED_VERSION].
- If validation fails:
  - Log error and exit.

### 2.3 Atomic and Memory Ordering Rules

- Writers that publish coherent state changes:

  - Use `store(value, std::memory_order_release)` for:
    - `intervention_tier`
    - `masking_alert`
    - `active_profile_id`
    - `profile_confidence`
    - Timestamps

- Readers that depend on consistency:

  - Use `load(std::memory_order_acquire)` when reading the above.

- Counters where exact interleaving is not critical (e.g., `notifications_count`):

  - Use `fetch_add` with `memory_order_relaxed`.

No mutexes are used around SharedState. Synchronization is entirely via atomics and clearly defined ordering.

### 2.4 Validation and Recovery

- On startup, each daemon:

  - Validates header.
  - If invalid:
    - Writes a `CRITICAL` log entry.
    - Exits with non-zero code.

- `nnos_boot_daemon`:

  - If its own mapping fails validation, it recreates shared memory from scratch.

---

## 3. Component Design (Daemons)

### 3.1 nnos_boot_daemon

Responsibilities:

- SharedState lifecycle.
- Profile matching.
- Child process supervision.

Key steps:

1. Load configuration:
   - `/etc/nnos/config.ini` (paths, network, log location).
   - `/etc/nnos/observed_traits.ini` (optional).
2. Create and initialize SharedState.
3. Run profile matching:
   - Load archetype tables from `neuro_profile.hpp`.
   - Use `profile_matcher.hpp` to choose archetype.
   - Write profile fields to SharedState.
4. Spawn child daemons via `fork` + `execl` or `posix_spawn`.
5. Monitor children:
   - `waitpid` loop.
   - Restart crashed children.
   - Apply circuit breaker (stop restarting if too frequent).
6. Handle SIGTERM:
   - Propagate SIGTERM.
   - Cleanup shared memory.
   - Exit.

Internal modules used:

- `common.hpp` (constants, enums).
- `shared_memory.hpp` (creation/attachment helpers).
- `neuro_profile.hpp` / `profile_matcher.hpp`.

### 3.2 nnos_state_monitor

Responsibilities:

- 4 Hz sampling and tier computation.

Design:

- Single-threaded loop:

  - Read raw inputs (initial version uses simulated sensors and self-report injection).
  - Compute:
    - `sensory_load`.
    - `emotional_load`.
    - `total_load`.
  - Compute new `intervention_tier`.
  - If tier changed:
    - `store` new tier (release).
    - Update `last_state_update`.
    - Log TIER_CHANGE event.

- Timing:

  - Use `clock_nanosleep` or similar for 250 ms intervals.
  - Set SCHED_FIFO priority higher than other daemons (e.g., 40).

Inputs (initial):

- Configured baselines.
- Self-report values via a simple local API or test harness.

Outputs:

- Updated SharedState fields as per SRS.
- Structured logs.

### 3.3 nnos_task_manager

Responsibilities:

- Enforce `max_concurrent_tasks`.
- Accept/Defer decisions.

Design:

- Local IPC interface:

  - For v1: simple stdin/CLI or Unix-domain socket with JSON messages.
  - Future: HTTP/REST or gRPC.

- Decision algorithm:

  - Load from SharedState (acquire):
    - `active_task_count`.
    - `intervention_tier`.
    - Thresholds.
  - Compute breathing room from task and margin config.
  - Decide ACCEPT or DEFER.
  - On ACCEPT:
    - `active_task_count.fetch_add(1, release)`.
    - Update `last_task_event`.
  - On completion (external signal):
    - `fetch_sub(1, release)`.

### 3.4 nnos_context_gate

Responsibilities:

- Track context switches.
- Protect/break hyperfocus.

Design:

- Periodic loop (e.g., 1 Hz):

  - Track current hour window.
  - Reset `ctx_switches_this_hour` at hour boundary.
  - For each incoming interrupt:
    - Apply budget rules.
    - Decide allow/defer.
  - Manage hyperfocus state:
    - Update `hyperfocus_minutes`.
    - Apply protect/break rules.

Integration:

- Interrupts can be modeled as events in a queue (initially, a simple test harness or CLI invocation).

### 3.5 nnos_comm_bridge

Responsibilities:

- Capacity/demand computation.
- Masking detection.
- Communication decisions.

Design:

- Expose a local API:

  - For v1: HTTP or Unix-socket JSON interface:
    - Input: request with effort_min, importance, social_intensity.
    - Output: decision, capacity, demand, masking_flag.

- Steps:

  - Read SharedState:
    - sensory_load, social_minutes_today, active_task_count, tier.
  - Incorporate self-report (energy, social_battery).
  - Compute capacity and demand.
  - Detect masking.
  - Update `masking_alert` if needed.
  - Log decision.

### 3.6 nnos_profile_refiner

Responsibilities:

- Daily analysis and trait adjustment.

Design:

- Scheduled by internal timer:

  - Uses local time to trigger around 03:00.
  - Alternatively, systemd timer could be used to run this daemon once per day.

- Steps:

  - Read logs from `/var/log/nnos/events.jsonl`.
  - Aggregate metrics over last 30 days.
  - Compare to archetype expectations.
  - Adjust traits and thresholds (bounded ±1 per run).
  - Re-match archetype and possibly switch.
  - Respect overrides in `/etc/nnos/overrides.ini`.
  - Persist updated profile and write into SharedState.

### 3.7 nnos_ethernet_sync

Responsibilities:

- Periodic broadcast.
- Receive and merge remote state.
- TCP polling endpoint.

Design:

- UDP multicast socket:
  - Join group 239.73.78.69 on configured interface.
- AES-256-GCM:
  - Load PSK from `/etc/nnos/sync.key`.
  - Derive nonce from device_id and sequence.
- Transmit:
  - Every 100 ms, read SharedState snapshot (with acquire loads).
  - Build header + payload.
  - Encrypt and send.
- Receive:
  - Loop on `recvfrom`.
  - Decrypt.
  - Validate magic, version, tag.
  - Check anti-replay via sequence map.
  - Merge fields according to SRS rules.
- TCP polling:
  - Listen on port 20047 (on chosen node).
  - On new connection:
    - Read nothing (simple handshake).
    - Serialize SharedState to JSON.
    - Optionally encrypt again at application level or rely on LAN.

---

## 4. Repo Layout and File Roles

### 4.1 Directory Structure

```text
nnos/
├── docs/
│   ├── NNOS-SRS-001.md
│   └── NNOS-DS-001.md
├── include/nnos/
│   ├── common.hpp
│   ├── config.hpp
│   ├── logging.hpp
│   ├── shared_state.hpp
│   ├── neuro_profile.hpp
│   ├── profile_matcher.hpp
│   ├── state_monitor.hpp
│   ├── task_manager.hpp
│   ├── context_gate.hpp
│   ├── comm_bridge.hpp
│   ├── profile_refiner.hpp
│   └── ethernet_sync.hpp
├── src/
│   ├── boot_daemon.cpp
│   ├── state_monitor.cpp
│   ├── task_manager.cpp
│   ├── context_gate.cpp
│   ├── comm_bridge.cpp
│   ├── profile_refiner.cpp
│   └── ethernet_sync.cpp
├── systemd/
│   └── nnos-boot.service
├── scripts/
│   └── bootstrap_encoding.sh
└── tests/
    ├── test_shared_state.cpp
    ├── test_state_monitor.cpp
    ├── test_task_manager.cpp
    ├── test_context_gate.cpp
    └── test_ethernet_sync.cpp
```

### 4.2 Header Responsibilities

- `common.hpp`:
  - Constants (magic, version, ports, multicast group).
  - Enum types.
- `config.hpp`:
  - `struct Config`.
  - `Config load_config(const std::string& path);`
- `logging.hpp`:
  - JSON line logging helper.
- `shared_state.hpp`:
  - `struct SharedState`.
  - Helper functions for init/validate.
- `neuro_profile.hpp`:
  - Archetype definitions.
- `profile_matcher.hpp`:
  - Matching algorithm interface.
- `*_monitor.hpp`, `task_manager.hpp`, etc.:
  - Function prototypes for their daemons.

---

## 5. Scheduling, Systemd, and Testing

### 5.1 Real-Time Scheduling

- `nnos_state_monitor`:
  - SCHED_FIFO, priority 40.
- `nnos_context_gate`:
  - SCHED_FIFO, priority 30.
- Others:
  - SCHED_OTHER or lower real-time priority.

Implementation:

- Use `pthread_setschedparam` in each daemon.
- If real-time scheduling not permitted, log a warning and continue with normal scheduling.

### 5.2 Systemd Unit

`systemd/nnos-boot.service` is the only unit; children are spawned by the boot daemon.

Key directives:

- `After=network.target`
- `User=nnos`
- `Group=nnos`
- Hardening:
  - `NoNewPrivileges=true`
  - `PrivateTmp=true`
  - `ProtectSystem=full`
  - `ProtectHome=true`

### 5.3 Test Hooks

- `--simulate` flag for daemons:
  - `nnos_state_monitor --simulate` uses synthetic signals.
  - `nnos_task_manager --simulate` reads tasks from stdin or a test file.
- `tests/*.cpp`:
  - Link against shared_state and common code.
  - Use in-memory SharedState instances (not shm) for unit tests.

---

## 6. Design Constraints Recap

- C++17.
- -fno-exceptions, -fno-rtti.
- No heap allocations in main loops.
- SharedState is single source of truth for runtime state.
- No direct external dependencies in business logic beyond:
  - POSIX,
  - OpenSSL,
  - Standard C++.

---

## 7. Document Status

This Design Specification (NNOS-DS-001) is paired with NNOS-SRS-001 and provides:

- Explicit SharedState layout.
- Detailed daemon responsibilities and interactions.
- Ethernet sync structure.
- Scheduling, systemd, and testing approach.

It is sufficient for an engineer or AI coder to design and implement NNOS daemons in the repo layout defined in Section 4.
```
