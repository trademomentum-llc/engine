# NNOS -- Design Specification
## Neurodivergent Neural-Link Operating System Architecture

| Field | Value |
|---|---|
| Document ID | NNOS-DS-001 |
| Version | 1.0.0 |
| Date | 2026-02-22 |
| Status | APPROVED |

---

## 1. System Architecture

NNOS is implemented as a constellation of cooperating C++ daemons running on a single Linux host, communicating through a shared memory segment and, across devices, through an encrypted multicast protocol.

The design goals are:
- Deterministic behavior under load.
- Zero-heap, low-jitter execution in hot paths.
- Clear separation between sensing, decision-making, and actuation.
- Easy observability for debugging overload and masking behavior.

### 1.1 High-Level Component Diagram

Textual view of the architecture:

- boot_daemon
  - Creates and initializes shared memory.
  - Spawns and supervises child daemons.
  - Handles clean shutdown on SIGTERM.
- nnos_state_monitor
  - Reads raw signals (HRV proxy, sensory metrics, self-report).
  - Computes sensory_load, emotional_load, and intervention tier.
  - Publishes updates into shared memory.
- nnos_task_manager
  - Manages task intake, scheduling, and handoff notes.
  - Enforces max concurrent tasks and slot compatibility.
- nnos_context_gate
  - Enforces context switch budgets.
  - Protects or breaks hyperfocus based on thresholds and state.
- nnos_comm_bridge
  - Evaluates capacity vs demand.
  - Produces communication decisions (ACCEPT, NEGOTIATE_SCOPE, NEGOTIATE_DEADLINE, DECLINE).
- nnos_profile_refiner
  - Runs daily to adjust trait levels and re-match profiles.
  - Writes updated profile parameters back into shared memory and disk.
- nnos_ethernet_sync
  - Serializes the SharedState struct.
  - Encrypts and multicasts state to other devices.
  - Receives, authenticates, and merges remote state.

All daemons attach to the same POSIX shared memory object and operate on a single SharedState struct using atomic fields and explicit memory ordering.

### 1.2 Process Model

- All daemons are simple, single-threaded processes.
- Each daemon has:
  - A main event loop.
  - A fixed-period tick (for example, 250 ms for nnos_state_monitor).
  - No dynamic memory allocation after initialization.
- Inter-process communication on the same host uses:
  - POSIX shared memory (shm_open + mmap).
  - Atomic operations for synchronization.
  - No mutexes in hot paths.

### 1.3 Real-Time and Determinism Strategy

To preserve determinism:

- State monitor and context gate run with SCHED_FIFO priorities.
- All daemons:
  - Use fixed-size buffers only.
  - Avoid exceptions and RTTI.
  - Avoid blocking I/O in hot paths.
- Network handling is:
  - Single-threaded.
  - Non-blocking sockets with timeouts.
  - Bounded serialization and encryption work per tick.

The design trades throughput for predictability: it is more important that decisions happen within known timing bounds than that the system saturate the CPU.

---

## 2. Daemon Responsibilities

This section defines each daemon’s contract in more detail: inputs, outputs, and invariants.

### 2.1 boot_daemon

**Purpose**

- Owns process lifetime.
- Ensures shared memory is created, initialized, and cleaned up.
- Supervises all other daemons and restarts them on failure.

**Inputs**

- Command line arguments and environment.
- systemd service configuration.
- SIGTERM and SIGINT signals from the OS.

**Outputs**

- Creates `/dev/shm/nnos_neural_link` shared memory and initializes the SharedState struct.
- Spawns child processes:
  - nnos_state_monitor
  - nnos_task_manager
  - nnos_context_gate
  - nnos_comm_bridge
  - nnos_profile_refiner
  - nnos_ethernet_sync
- Writes structured logs for:
  - Startup and shutdown.
  - Child exits and restarts.
  - Shared memory creation and cleanup.

**Invariants**

- Shared memory magic value is valid before any child daemon is spawned.
- On SIGTERM:
  - Sends SIGTERM to all children.
  - Waits for them to exit.
  - Unlinks shared memory and exits with code 0.

---

### 2.2 nnos_state_monitor

**Purpose**

- Sample physiological and contextual signals.
- Compute sensory_load, emotional_load, and intervention tier.
- Publish state updates into shared memory at a fixed rate.

**Inputs**

- Sensor readings (initially simulated values from configuration or simple probes).
- Self-report values (overwhelm, energy) from a local IPC mechanism or stub.
- Profile-specific thresholds from SharedState.

**Outputs**

- Updated fields in SharedState:
  - heart_rate_variability
  - noise_level
  - light_level
  - notifications_count
  - self_report_overwhelm
  - intervention_tier
  - last_state_update
- Logs of tier changes, including:
  - Old tier, new tier.
  - Current sensory_load and emotional_load.

**Invariants**

- Publishes an updated intervention_tier within 50 ms of detecting a threshold crossing.
- Never blocks for more than its tick interval (for example 250 ms).
- Uses atomic stores with memory_order_release when updating tier and timestamps.

---

### 2.3 nnos_task_manager

**Purpose**

- Accept or defer tasks based on profile constraints and current tier.
- Track active tasks, generate handoff notes, and respect slot scheduling.

**Inputs**

- Task intake events from a queue or IPC interface (task id, type, demand classification).
- Profile parameters from SharedState:
  - max_concurrent_tasks
  - deep_work_block_min
  - light_work_block_min
- Current intervention_tier and active_task_count from SharedState.
- Current time slot classification (deep work, light work, rest).

**Outputs**

- Updated SharedState fields:
  - active_task_count
  - last_task_event
- Handoff notes written to a log or lightweight file (task id, last index, next index, timestamp).
- Accept/defer decisions returned immediately to the caller via IPC.

**Invariants**

- Never exceeds max_concurrent_tasks.
- Refuses new tasks automatically when intervention_tier >= 2.
- Returns accept/defer decisions within 10 ms.

---

### 2.4 nnos_context_gate

**Purpose**

- Manage context switches and hyperfocus protection.
- Decide whether to allow or defer interrupts.

**Inputs**

- Incoming interrupt events (notifications, meeting start, etc.).
- Current:
  - ctx_switches_this_hour
  - hyperfocus_active
  - hyperfocus_minutes
  - intervention_tier
  - profile thresholds (max_ctx_switches_per_hour, hyperfocus duration limits).

**Outputs**

- Updated SharedState fields:
  - ctx_switches_this_hour
  - hyperfocus_active
  - hyperfocus_minutes
- Logs of:
  - Allowed vs deferred context switches.
  - Hyperfocus protect vs break decisions.

**Invariants**

- Enforces the per-hour context switch budget for each profile.
- Defers interrupts when:
  - Budget is exhausted and
  - Incoming priority < active priority + 2.
- Breaks hyperfocus when:
  - intervention_tier >= 2, or
  - hyperfocus_minutes exceed the configured max.

---

### 2.5 nnos_comm_bridge

**Purpose**

- Detect masking by comparing capacity and demand.
- Produce a recommended communication stance.

**Inputs**

- SharedState fields:
  - sensory_load, social_minutes_today, active_task_count.
  - Profile capacity-related parameters.
- Structured representation of an incoming request:
  - effort_min
  - importance
  - social_intensity
  - deadline pressure (optional extension).

**Outputs**

- Computed capacity and demand scores (logged or exposed for UI).
- A decision enumeration:
  - ACCEPT
  - NEGOTIATE_SCOPE
  - NEGOTIATE_DEADLINE
  - DECLINE
- Updates masking_alert flag in SharedState when masking is detected.

**Invariants**

- Uses deterministic formulas from the SRS for capacity and demand.
- Never suppresses masking_alert once set; only a separate clear event may reset it.
- Returns a decision within a fixed time bound (for example 10 ms).

---

### 2.6 nnos_profile_refiner

**Purpose**

- Adjust profile traits over time based on observed behavior.
- Switch to a better-fitting archetype when evidence is strong.

**Inputs**

- Read-only access to:
  - Historical logs (last 30 days) from `/var/log/nnos/events.jsonl`.
  - Current archetype parameters in SharedState.
- Schedule:
  - Triggered once per day at 03:00 local time.

**Outputs**

- Updated trait levels written to SharedState and persisted in `/etc/nnos/active_profile.bin`.
- Updated active_profile_id when a new archetype is selected.
- Logs summarizing changes:
  - Which traits changed.
  - Old vs new values.
  - Confidence of new match.

**Invariants**

- Trait adjustments are bounded to small steps (for example +/- 1) per refinement run.
- Manual overrides from `/etc/nnos/overrides.ini` are never overwritten.
- Only switches archetypes when new confidence exceeds current by at least the configured margin.

---

### 2.7 nnos_ethernet_sync

**Purpose**

- Keep SharedState synchronized across devices.
- Ensure confidentiality and integrity of state over the network.

**Inputs**

- Local SharedState snapshot.
- Sync key loaded from `/etc/nnos/sync.key`.
- Incoming multicast packets on `239.73.78.69:20046`.
- Incoming TCP polling connections on port 20047.

**Outputs**

- Periodic multicast packets containing encrypted state.
- Updated local SharedState fields from remote devices, merged according to:
  - Timestamp for sensors.
  - Local-override semantics for tasks.
  - Profile timestamp for profile fields.
- Encrypted TCP responses with a current SharedState snapshot.

**Invariants**

- Drops packets that:
  - Have an invalid magic number.
  - Fail AES-GCM authentication.
  - Have sequence numbers not strictly greater than the last seen for that device.
- Respects merge rules defined in the SRS.
- Responds to TCP polling requests within the configured latency bound.

---

## 3. Shared Memory Design

### 3.1 SharedState Struct Layout

The SharedState struct is the single source of truth for all daemons on a host. It is allocated in a POSIX shared memory object and mapped read-write into each process.

Key design constraints:
- Fixed size (no variable-length members).
- Cache-friendly layout (group related fields, respect alignment).
- All cross-process fields are atomic.
- Magic and version fields allow validation and migration.

Conceptual layout (simplified):

```cpp
struct alignas(64) SharedState {
    // Header
    uint64_t magic;            // 0x4E4E4F534C494E4B ("NNOSLINK")
    uint32_t version;          // Struct version
    uint32_t reserved0;

    // Physiological signals
    std::atomic<float> heart_rate_variability;
    std::atomic<float> noise_level;
    std::atomic<float> light_level;
    std::atomic<uint16_t> notifications_count;
    std::atomic<uint8_t> self_report_overwhelm;
    uint8_t padding0;

    // Profile identity
    std::atomic<uint8_t> active_profile_id;
    std::atomic<float> profile_confidence;
    uint8_t reserved_profile;[1]

    // Trait dimensions (0-10)
    std::atomic<uint8_t> need_for_structure;
    std::atomic<uint8_t> novelty_seeking;
    std::atomic<uint8_t> hyperfocus_inclination;
    std::atomic<uint8_t> sensory_sensitivity;
    std::atomic<uint8_t> social_energy_capacity;
    std::atomic<uint8_t> exec_function_difficulty;
    uint8_t padding1;[2]

    // Thresholds and limits
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
    uint8_t padding2;[1]

    // Timestamps (seconds since epoch or monotonic)
    std::atomic<uint32_t> last_state_update;
    std::atomic<uint32_t> last_profile_update;
    std::atomic<uint32_t> last_task_event;
    std::atomic<uint32_t> boot_timestamp;

    // Reserved for future fields
    uint8_t reserved_region;
};
3.2 Initialization Rules
	•	boot_daemon:
	•	Creates the shared memory object with a fixed size.
	•	Zeroes the entire region.
	•	Writes:
	•	magic
	•	version
	•	boot_timestamp
	•	Sets safe defaults for:
	•	active_profile_id
	•	thresholds and limits.
	•	Every child daemon:
	•	Maps the same object read-write.
	•	Validates:
	•	magic matches expected constant.
	•	version is supported.
	•	If validation fails, daemon logs an error and exits.
3.3 Atomic Access and Memory Ordering
Rules:
	•	Writers use:
	•	 memory_order_release  when publishing state that others will read as a coherent snapshot (for example intervention_tier, timestamps).
	•	Readers use:
	•	 memory_order_acquire  when consuming those fields to ensure they see up-to-date data.
	•	Fields that are monotonic counters (for example notifications_count, ctx_switches_this_hour) can use:
	•	 fetch_add  with  memory_order_relaxed  when the exact interleaving does not matter, but readers should still use acquire when correlating with other fields.
No daemon uses  std::mutex  around SharedState. All synchronization is via atomics and careful ordering.
3.4 Validation and Corruption Handling
Each daemon, on startup:
	•	Checks:
	•	 magic == 0x4E4E4F534C494E4B 
	•	 version  in a known supported range.
	•	If validation fails:
	•	Logs an error including the invalid values.
	•	Exits with a non-zero code.
	•	boot_daemon is responsible for:
	•	Recreating shared memory if it detects corruption at boot time.
	•	Never attempting to reuse a region with wrong magic.
4. Ethernet Sync Protocol Design
4.1 Goals
	•	Keep SharedState synchronized across multiple devices on a LAN.
	•	Ensure confidentiality and integrity of state.
	•	Avoid replay and stale-state issues.
	•	Preserve host-local fields (for example tasks) while sharing cross-device fields (for example tiers, sensory metrics).
4.2 Packet Structure
Each multicast packet has the following conceptual structure (before encryption):
	•	Header:
	•	magic (4 bytes) = 0x4E4E5359 (“NNsy”)
	•	version (1 byte)
	•	flags (1 byte)
	•	reserved (2 bytes)
	•	device_id (8 bytes, stable per device)
	•	sequence (4 bytes, monotonic per device)
	•	timestamp (4 bytes, sender time)
	•	Payload:
	•	Compact encoding of SharedState fields that are cross-device relevant:
	•	intervention_tier
	•	sensory and emotional load metrics
	•	masking_alert
	•	timestamps
	•	profile identity and confidence.
	•	Authentication:
	•	AES GCM nonce (12 bytes)
	•	Ciphertext (header + payload)
	•	GCM tag (16 bytes)
The full UDP payload stays within standard MTU limits.
4.3 Encryption and Key Management
	•	Algorithm: AES-256-GCM.
	•	Key:
	•	Read from  /etc/nnos/sync.key  (32 bytes).
	•	File permissions: 0600.
	•	Nonce:
	•	12 bytes per packet.
	•	Constructed from:
	•	device_id (high bits)
	•	sequence (low bits)
	•	Ensures uniqueness per key.
On send:
	•	Serialize header + payload.
	•	Generate nonce from device_id and sequence.
	•	Encrypt with AES-256-GCM to produce ciphertext + tag.
On receive:
	•	Derive nonce using device_id and sequence in the same way.
	•	Attempt AES-256-GCM decryption.
	•	If decryption or tag check fails:
	•	Drop packet and log.
4.4 Anti-Replay
Each nnos_ethernet_sync instance maintains an in-memory map:
	•	Key: device_id.
	•	Value: last_seen_sequence.
On receiving a valid, authenticated packet:
	•	If  sequence <= last_seen_sequencedevice_id :
	•	Drop as replay or out-of-order.
	•	Else:
	•	Accept and update  last_seen_sequencedevice_id .
This ensures only forward-progressing streams are used to update state.
4.5 Merge Strategy
When applying remote state:
	•	Sensor fields:
	•	Compare remote  last_state_update  to local.
	•	If remote is newer:
	•	Copy remote sensor-related fields into local SharedState.
	•	Task fields:
	•	Never modify host-local task state from remote packets.
	•	Profile fields:
	•	Compare remote  last_profile_update  to local.
	•	If remote is newer:
	•	Update profile identity and trait thresholds.
All updates use atomics with release semantics; readers on other daemons use acquire reads.
5. Logging and Observability
5.1 Logging Strategy
All daemons log to structured JSON lines:
	•	Path:  /var/log/nnos/events.jsonl 
	•	One JSON object per line, including:
	•	timestamp
	•	daemon name
	•	level (info, warn, error)
	•	event type (for example TIER_CHANGE, TASK_ACCEPTED, MASKING_DETECTED)
	•	payload object with event-specific fields.
Example:

{
  "ts": "2026-02-22T10:45:12Z",
  "daemon": "nnos_state_monitor",
  "level": "info",
  "event": "TIER_CHANGE",
  "old_tier": 1,
  "new_tier": 2,
  "sensory_load": 0.72,
  "emotional_load": 0.65
}

5.2 Key Metrics
The system is designed to expose, via logs or future metrics endpoints:
	•	Current intervention_tier and frequency of tier changes.
	•	Average sensory_load and emotional_load over time windows.
	•	Context switch counts per hour and number of deferrals.
	•	Task acceptance vs deferral rates by task type.
	•	Masking detection events and their contexts.
These metrics support:
	•	Evaluating whether thresholds are tuned correctly.
	•	Debugging overload scenarios.
	•	Measuring the impact of NNOS on the user’s workload and burnout risk.
5.3 Failure Modes and Recovery
Expected failure modes:
	•	A daemon crashes:
	•	boot_daemon detects child exit.
	•	Logs exit code and timestamp.
	•	Attempts restart within 2 seconds.
	•	Shared memory corruption at boot:
	•	boot_daemon detects invalid magic or version.
	•	Recreates shared memory region.
	•	Network partition:
	•	nnos_ethernet_sync continues broadcasting local state.
	•	Remote devices hold last known good state.
	•	On reconnection, merge rules resolve discrepancies.
The design assumes:
	•	Local protection behavior (tiers, masking alerts) continues to function even if cross-device sync is temporarily unavailable.


---

## 12. Implementation Plan

This section defines how to turn this specification into a concrete, compiled codebase using an AI code generator (Claude Code, Qwen2.5 Coder, DeepSeek, etc.).

### 12.1 Repository Structure

Target repo layout:

lsa/ 
|– CMakeLists.txt 
|– include/lsa/ 
|   |– common.hpp 
|   |– shared_state.hpp 
|   |– node_roles.hpp 
|   |– origin_vault.hpp 
|   |– mirrorlock.hpp 
|   |– convergence_bond.hpp 
|   |– drift_detector.hpp 
|   |– task_manager.hpp 
|   |– context_gate.hpp 
|   |– comm_bridge.hpp 
|   |– profile_refiner.hpp 
|    -- ethernet_sync.hpp |-- src/ |   |-- boot_dcn.cpp |   |-- boot_hcn.cpp |   |-- boot_epn.cpp |   |-- state_monitor_epn.cpp |   |-- origin_vault_dcn.cpp |   |-- mirrorlock_hcn.cpp |   |-- convergence_bond_hcn.cpp |   |-- drift_detector_dcn.cpp |   |-- task_manager_dcn.cpp |   |-- context_gate_dcn.cpp |   |-- comm_bridge_hcn.cpp |   |-- profile_refiner_hcn.cpp |   |-- ethernet_sync_dcn.cpp |   |-- ethernet_sync_hcn.cpp |    – ethernet_sync_epn.cpp 
|– systemd/ 
|   |– lsa-boot-dcn.service 
|   |– lsa-boot-hcn.service 
|    -- lsa-boot-epn.service |-- scripts/ |   |-- bootstrap_encoding.sh |   |-- install_dcn.sh |   |-- install_hcn.sh |    – install_epn.sh  -- tests/ |-- test_shared_state.cpp |-- test_ethernet_sync.cpp |-- test_task_manager.cpp |-- test_context_gate.cpp |-- test_drift_detector.cpp  – test_origin_vault.cpp


### 12.2 Implementation Phases

#### Phase 1: Core Types and Shared State

1. `include/lsa/common.hpp`
   - Basic typedefs, constants (magic numbers, ports, multicast address).
   - NodeRole enum (DCN, HCN, EPN).
   - Logging helpers (JSON line formatter).

2. `include/lsa/shared_state.hpp`
   - SharedState struct (control-plane fields as in Section 9.1).
   - Functions for:
     - Initialization.
     - Validation (magic, version).
     - Atomic load/store helpers.

3. `include/lsa/node_roles.hpp`
   - Compile-time configuration for each node type:
     - Which daemons run.
     - Which fields are “owned” by the node.
     - Merge policies.

Ask the coder model:

> Implement `include/lsa/common.hpp`, `include/lsa/shared_state.hpp`, and `include/lsa/node_roles.hpp` according to Sections 3 and 9 of the spec.

#### Phase 2: Node Boot Daemons

Files:
- `src/boot_dcn.cpp`
- `src/boot_hcn.cpp`
- `src/boot_epn.cpp`

Responsibilities:
- Create and initialize shared memory (if DCN, or per node as decided).
- Spawn the appropriate child daemons for that node.
- Supervise children (restart on crash, handle SIGTERM).
- Log lifecycle events.

Prompts to coder model:

> Using the spec’s Daemon Supervision section (6.11) and the daemon inventory (Section 8), implement `boot_dcn.cpp` as a simple supervisor that creates shared memory, validates it, forks the DCN daemons, and restarts them on crash.

Repeat similarly for HCN and EPN boot processes.

#### Phase 3: Per-Node Daemons

Implement daemons in this order (easier dependency graph):

1. DCN:
   - `origin_vault_dcn.cpp`
   - `drift_detector_dcn.cpp`
   - `task_manager_dcn.cpp`
   - `context_gate_dcn.cpp`

2. HCN:
   - `mirrorlock_hcn.cpp`
   - `convergence_bond_hcn.cpp`
   - `profile_refiner_hcn.cpp`
   - `comm_bridge_hcn.cpp`

3. EPN:
   - `state_monitor_epn.cpp`

Each daemon:
- Attaches to shared memory.
- Validates SharedState.
- Enters an event loop with a fixed tick.
- Implements its Functional Requirements from Section 6.

Example prompt:

> Implement `src/task_manager_dcn.cpp` using FR-6.6 from the spec. It should:
> - Read profile limits and breathing room thresholds from SharedState.
> - Accept/deny tasks based on capacity and tier.
> - Update active_task_count and last_task_event atomically.
> - Log decisions as JSON lines.

#### Phase 4: Ethernet Sync

Files:
- `include/lsa/ethernet_sync.hpp`
- `src/ethernet_sync_dcn.cpp`
- `src/ethernet_sync_hcn.cpp`
- `src/ethernet_sync_epn.cpp`

Implement:
- Packet structure (Section 4.2).
- AES-256-GCM encryption/decryption.
- Anti-replay map keyed by device_id.
- Merge policies per Section 4.2 and 9.2.
- TCP polling endpoint on DCN (NUC).

Prompt:

> Implement the Ethernet sync module according to Sections 4 and 6.10. Each node only broadcasts fields it owns and merges remote fields per the merge strategy.

#### Phase 5: Build and Deploy

1. `CMakeLists.txt`
   - Targets:
     - `lsa_boot_dcn`, `lsa_boot_hcn`, `lsa_boot_epn`.
     - All daemons as separate executables or part of boot-controlled tower.
   - Flags:
     - `-O2 -Wall -Wextra -Wpedantic -fno-exceptions -fno-rtti -std=c++17`
   - Link:
     - `-lpthread -lssl -lcrypto -lrt` as needed.

2. `systemd/*.service`
   - One service per boot daemon.
   - WantedBy=multi-user.target.
   - Restart=always.

3. `scripts/install_*.sh`
   - Install binaries to `/usr/local/bin`.
   - Install systemd units.
   - Enable and start services.

### 12.3 Testing Strategy

Tests (Section 11 mapping + this plan):

- `test_shared_state.cpp`
  - Validate magic, version, atomic behavior.
- `test_ethernet_sync.cpp`
  - Simulate send/receive across two “nodes”.
  - Verify encryption, anti-replay, merge rules.
- `test_task_manager.cpp`
  - Feed synthetic tasks and verify acceptance rules.
- `test_context_gate.cpp`
  - Simulate context switches and hyperfocus states.
- `test_drift_detector.cpp`
  - Feed synthetic returns and verify drift alerts.
- `test_origin_vault.cpp`
  - CRUD operations against test DB; ensure break-glass behavior.

---

## 13. AI Implementation Instructions

When using an AI coder (Claude, Qwen2.5, DeepSeek, etc.):

1. Provide this spec file `LSA-SPEC-002` in full.
2. Ask it to:
   - “Summarize the node roles and daemons first, to prove understanding.”
   - Then: “Implement Phase 1 header files exactly as described in Section 12.2.”
3. Work file-by-file:
   - Never let it redesign the architecture.
   - Always refer it back to:
     - Node roles (Section 3).
     - Functional Requirements (Section 6).
     - Daemon inventory (Section 8).
     - Merge rules (Section 4 and 9.2).
4. After each file compiles, move to the next per the Phase ordering.

The goal is a deterministic, three-node fabric that encodes Jason’s decisions and breathing room law into compiled behavior, not another vague “AI agent” stack.

---

## 14. Glossary and Definitions

**Origin Vault**  
Authoritative store on NUC of decision rules, entry/exit conditions, tiers (COLD/WARM/HOT/ORIGIN), and confidence scores.

**MirrorLock**  
Module on M1 that compares stated (symbolic) and actual (subsymbolic) indicators to identify implicit knowledge (profitable mismatches).

**Convergence Bond**  
Module on M1 that measures domain overlap (cosine similarity) between Jason’s knowledge domains and external systems; optimal when overlap < 0.382.

**Drift**  
Deviation of recent returns from expected performance over a rolling window; used to detect when system behavior diverges from design assumptions.

**Breathing Room**  
Intentional under-utilization of capacity to preserve buffer for shocks. Mathematically anchored at complementarity_zone = 1.0 - phi_inverse ≈ 0.382.

**Execution Classes (E1/E2/E3)**  
Classification of workloads:
- E1: Deterministic control (NUC).
- E2: Parallel numerical (M1, Orin).
- E3: Real-time reactive (Orin).

**TP-HCF**  
Tri-Plane Heterogeneous Compute Fabric: Jason’s three-node architecture (NUC, M1, Orin) with differentiated roles.

**Masking (System Context)**  
Operating with apparent capacity while internal risk/overload is high; for trading and neuro context, continuing to take risk or tasks when capacity is low.

**Capacity Score**  
Computed metric combining energy, sensory load, social battery, commitments (0–1) indicating how much load Jason/system can safely handle.

**Demand Score**  
Computed metric combining effort, importance, social intensity (0–1) indicating how heavy a request or trade is.

---

## 15. Database Schemas (NUC)

### 15.1 Origin Vault Schema

Table: `origin_rules`

| Column            | Type        | Description                                  |
|-------------------|------------|----------------------------------------------|
| id                | UUID       | Primary key                                  |
| name              | TEXT       | Human-readable rule name                     |
| description       | TEXT       | Rule description                             |
| tier              | TEXT       | 'COLD'/'WARM'/'HOT'/'ORIGIN'                 |
| entry_condition   | JSONB      | Structured entry conditions                  |
| exit_condition    | JSONB      | Structured exit conditions                   |
| proxy_tier        | TEXT       | Proxy tier classification                    |
| confidence        | REAL       | Confidence score (0.0–1.0)                   |
| created_at        | TIMESTAMPTZ| Creation timestamp                           |
| updated_at        | TIMESTAMPTZ| Last update timestamp                        |

Table: `origin_events`

| Column        | Type        | Description                                   |
|---------------|------------|-----------------------------------------------|
| id            | UUID       | Primary key                                   |
| rule_id       | UUID       | FK to origin_rules                            |
| event_type    | TEXT       | 'APPLIED'/'BREACHED'/'OVERRIDDEN'             |
| payload       | JSONB      | Event details                                 |
| created_at    | TIMESTAMPTZ| Event timestamp                               |

### 15.2 Drift Detection Schema

Table: `drift_windows`

| Column        | Type        | Description                           |
|---------------|------------|---------------------------------------|
| id            | UUID       | Primary key                           |
| symbol        | TEXT       | Instrument identifier (optional)      |
| window_start  | TIMESTAMPTZ| Start of window                       |
| window_end    | TIMESTAMPTZ| End of window                         |
| expected_mean | REAL       | Expected return                        |
| actual_mean   | REAL       | Observed return                        |
| drift_value   | REAL       | Drift metric                           |
| alert_raised  | BOOLEAN    | Whether alert triggered                |
| created_at    | TIMESTAMPTZ| Record creation time                   |

### 15.3 Trade Log Schema (for MirrorLock / Market Context)

Table: `trade_log`

| Column           | Type        | Description                            |
|------------------|------------|----------------------------------------|
| id               | UUID       | Primary key                            |
| timestamp        | TIMESTAMPTZ| Trade time                             |
| symbol           | TEXT       | Instrument                             |
| direction        | TEXT       | 'LONG'/'SHORT'                         |
| size             | REAL       | Position size                          |
| stated_ma        | REAL       | Moving average used in plan            |
| actual_ma        | REAL       | Moving average at execution            |
| stated_stoch     | REAL       | Planned stochastic                     |
| actual_stoch     | REAL       | Actual stochastic                      |
| stated_trend     | REAL       | Planned trend descriptor               |
| actual_trend     | REAL       | Actual trend descriptor                |
| pnl              | REAL       | Profit/loss for the trade              |
| tags             | TEXT[]     | Labels for this trade                  |
| metadata         | JSONB      | Extra data                             |

---

## 16. IPC and API Interfaces

### 16.1 Task Intake API (NUC)

Local-only HTTP or Unix-socket API (implementation choice) with JSON payloads.

Endpoint: `POST /tasks/intake`

Request body:

```json
{
  "task_id": "string",
  "type": "TRADE" or "ANALYSIS" or "ADMIN",
  "priority": 0,
  "effort_min": 60,
  "importance": 0.8,
  "social_intensity": 0.2,
  "metadata": {
    "symbol": "EURUSD",
    "note": "London open setup"
  }
}

Response:

```json
{
  "decision": "ACCEPT" or "DEFER",
  "reason": "breathing_room_exceeded" or "tier_too_high" or "ok",
  "current_tier": 1
}
```

16.2 Comm Bridge Decision API (M1)
Endpoint:  POST /comm/decide 
Request:

```json
{
  "effort_min": 90,
  "importance": 0.9,
  "social_intensity": 0.6,
  "context": {
    "description": "New collaboration request",
    "source": "email"
  }
}
```

Response:

```json
{
  "decision": "ACCEPT" or "NEGOTIATE_SCOPE" or "NEGOTIATE_DEADLINE" or "DECLINE",
  "capacity": 0.45,
  "demand": 0.78,
  "masking_flag": true
}
```
16.3 Pixel 9 Pro Polling (to NUC)
Protocol: TCP, custom simple binary or JSON over TCP.
Minimum JSON response schema:

```json
{
  "timestamp": "2026-02-22T15:30:00Z",
  "intervention_tier": 2,
  "sensory_load": 0.71,
  "masking_flag": true,
  "breathing_room": 0.38,
  "drift_alert": false
}
```
## 17. Ethernet Packet Layout (Wire Format)

### 17.1 UDP Multicast Packet (Pre-Encryption)

All multi-byte integers are network byte order.

| Offset | Field          | Size | Type      | Description                      |
|--------|----------------|------|-----------|----------------------------------|
| 0      | magic          | 4    | uint32    | 0x4E4E5359                       |
| 4      | version        | 1    | uint8     | Protocol version                 |
| 5      | flags          | 1    | uint8     | Reserved                         |
| 6      | reserved       | 2    | uint16    | Reserved                         |
| 8      | device_id      | 8    | uint64    | Sender device id                 |
| 16     | sequence       | 4    | uint32    | Monotonic per device            |
| 20     | timestamp      | 4    | uint32    | Seconds since epoch or monotonic|
| 24     | payload        | 40   | struct    | Compact state fields             |

Example payload encoding (40 bytes):

- 0: intervention_tier (uint8)
- 1: masking_flag (uint8)
- 2–5: sensory_load (float32)
- 6–9: drift_score (float32)
- 10–13: profile_confidence (float32)
- 14–17: last_state_update (uint32)
- 18–21: last_profile_update (uint32)
- 22–25: reserved
- 26–39: reserved_future

Total pre-encryption length: 64 bytes.

### 17.2 AES-GCM Wrapper

- Nonce: 12 bytes (e.g., high bits device_id, low bits sequence).
- Ciphertext: encryption of header + payload (64 bytes).
- Tag: 16 bytes.

Final UDP payload length: 12 + 64 + 16 = 92 bytes.

---

## 18. Acceptance Criteria and Test Matrix

### 18.1 Key Acceptance Criteria

AC-18.1: Origin Vault read queries on NUC meet NFR-7.1.1 under test load.

AC-18.2: Drift detector raises alerts when synthetic drift > 0.618 and remains silent below threshold.

AC-18.3: Ethernet sync passes:
- Encryption correctness tests.
- Anti-replay tests.
- Merge strategy tests across 3-node simulated fabric.

AC-18.4: Task Manager and Context Gate:
- Enforce max_concurrent_tasks and breathing room thresholds correctly.
- Never auto-execute trades (manual gate preserved).

AC-18.5: Anti-masking detection triggers when capacity < 0.4 AND demand > 0.7 in test scenarios.

AC-18.6: E2 workloads (MirrorLock, Convergence Bond) execute only on M1 in deployment config.

### 18.2 Test Matrix (High-Level)

| Test ID | Description | Node | Related Sections | Pass Criteria |
|---------|-------------|------|------------------|--------------|
| T-001 | Origin Vault basic CRUD | NUC | 6.1, 15.1 | All CRUD ops succeed, persisted |
| T-002 | MirrorLock mismatch logging | M1 | 6.2, 15.3 | Profitable mismatches tagged as IMPLICIT KNOWLEDGE |
| T-003 | Convergence Bond overlap thresholds | M1 | 6.3 | Overlap < 0.382 yields 2193x leverage flag |
| T-004 | Drift alert threshold | NUC | 6.4, 15.2 | Alert raised only when drift > 0.618 |
| T-005 | State monitor timing | Orin | 6.5, 7.1 | Tier updates within 50 ms of threshold crossing |
| T-006 | Task Manager breathing room | NUC | 6.6, 5 | Tasks rejected when breathing_room < 0.382 |
| T-007 | Context Gate manual-only mode | NUC | 6.7 | No auto-execution when shock flag set |
| T-008 | Profile refinement sync | M1 + NUC + Orin | 6.8, 6.10 | Profile updates visible on all nodes within 150 ms |
| T-009 | Anti-masking decision logic | M1 | 6.9 | Decisions match spec across test grid |
| T-010 | Ethernet sync anti-replay | All | 6.10, 17 | Replayed packets dropped, latest state preserved |

---

## 19. Document Status

This document (LSA-SPEC-002) is considered complete for:

- System Requirements (functional and non-functional).
- Node roles and workload routing.
- Daemon responsibilities and deployment.
- Data, DB, IPC, and wire-format design.
- Implementation roadmap for AI coder models.
- Acceptance criteria and test planning.

Any further changes SHOULD be tracked via version bump and explicit change log.
```
