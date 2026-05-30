# NNOS -- Design Specification
## Neurodivergent Neural-Link Operating System Design Document

| Field | Value |
|---|---|
| Document ID | NNOS-DS-001 |
| Version | 2.0.0 |
| Date | 2026-02-23 |
| Status | APPROVED |
| Related Specs | NNOS-SRS-001 |
| Classification | CONFIDENTIAL |
| Encoding | UTF-8 without BOM, ASCII-safe punctuation only |

---

## 1. Introduction

### 1.1 Purpose

This Design Specification translates the requirements defined in NNOS-SRS-001
into a concrete architectural and implementation blueprint. It defines:

- System architecture and daemon constellation design.
- Data structures and memory layouts.
- Algorithms for profile matching, load computation, and intervention logic.
- Inter-process and inter-device communication protocols.
- File system layout, configuration formats, and operational procedures.

This document is normative: implementations MUST conform to the structures
and algorithms defined here unless explicitly noted as "implementation choice."

### 1.2 Scope

This design covers the complete NNOS firmware layer running on:

- Jetson Orin Nano (ARM64)
- ASUS NUC 15 Pro (x86_64)

It does NOT cover:

- Mobile client app UI/UX (separate spec).
- Wearable sensor hardware interfaces (future extension).
- Cloud sync or remote telemetry (NNOS operates offline-first).

### 1.3 Design Principles

1. **Determinism First**: Given identical inputs and state, NNOS SHALL produce
   identical outputs. No randomness in hot paths; PRNGs only for key generation.

2. **Real-Time Guarantee**: Critical paths (tier computation, task decisions)
   MUST complete within hard deadlines (50 ms, 10 ms respectively).

3. **Zero-Copy Shared State**: All cross-process communication via shared memory.
   No message queues, no pipes, no sockets between daemons on same host.

4. **Fail-Safe Defaults**: On error or ambiguity, default to the safest choice
   (defer tasks, protect rest time, escalate tier).

5. **Observable Behavior**: All state transitions and decisions logged as
   structured JSON for offline analysis.

6. **Security by Design**: Encrypt network data, validate all inputs, run with
   least privilege, isolate processes.

---

## 2. System Architecture

### 2.1 Daemon Constellation

NNOS runs as seven cooperating processes on each device:

```
┌─────────────────────────────────────────────────────────────┐
│                       nnos_boot_daemon                       │
│  - Creates and initializes SharedState in /dev/shm          │
│  - Matches profile to archetype                             │
│  - Forks and supervises six child daemons                   │
│  - Restarts crashed children (circuit breaker after 5×)     │
└────────────┬────────────────────────────────────────────────┘
             │ forks
             ├──> nnos_state_monitor
             │    - Samples signals at 4 Hz
             │    - Computes sensory_load, emotional_load, tier
             │    - Writes to SharedState every cycle
             │
             ├──> nnos_task_manager
             │    - Listens for task intake events (IPC or socket)
             │    - Enforces max_concurrent_tasks
             │    - Returns ACCEPT/DEFER within 10 ms
             │
             ├──> nnos_context_gate
             │    - Tracks context switch budget
             │    - Protects/breaks hyperfocus
             │    - Generates gentle reminders during healthy focus
             │
             ├──> nnos_comm_bridge
             │    - Computes capacity and demand scores
             │    - Detects masking
             │    - Recommends ACCEPT/NEGOTIATE/DECLINE
             │
             ├──> nnos_profile_refiner
             │    - Sleeps 23h 59m, wakes at 03:00 daily
             │    - Analyzes logs, adjusts traits, re-matches archetype
             │    - Updates SharedState and persists to disk
             │
             └──> nnos_ethernet_sync
                  - Broadcasts SharedState summary via UDP multicast (100 ms)
                  - Receives and merges state from peer devices
                  - Runs TCP server on :20047 for mobile polling
```

### 2.2 Shared Memory Region

All daemons attach to a single POSIX shared memory object:

- Name: `/dev/shm/nnos_neural_link`
- Size: 8192 bytes (fixed, power of 2 for page alignment)
- Contents: One `SharedState` struct (4096 bytes) + reserved (4096 bytes)
- Permissions: 0660, owner `nnos:nnos`

On boot:

1. `nnos_boot_daemon` calls `shm_open(O_CREAT | O_EXCL | O_RDWR)`.
2. If exists (stale from previous boot), `shm_unlink` and retry.
3. `ftruncate` to 8192 bytes.
4. `mmap(MAP_SHARED)` into process address space.
5. Zero entire region with `memset`.
6. Write magic, version, boot_timestamp.
7. Match profile, write profile fields.
8. Fork children; each child opens same shm, validates magic/version, mmaps.

On shutdown (SIGTERM to boot daemon):

1. Boot daemon sends SIGTERM to all children.
2. Wait 10 seconds for children to exit cleanly.
3. Force SIGKILL any remaining.
4. `munmap` shared memory.
5. `shm_unlink("/nnos_neural_link")`.
6. Exit.

### 2.3 Inter-Daemon Communication

**On same host**: Via SharedState only. Daemons read/write atomic fields.

**Across network**: Via UDP multicast (state sync) and TCP polling (mobile).

No message passing, no IPC queues between daemons on same device.

### 2.4 Process Lifecycle

Each daemon:

1. Parses command-line args (`--simulate`, `--log-level=debug`, etc.).
2. Opens `/dev/shm/nnos_neural_link` via `shm_open(O_RDWR)`.
3. `mmap` with `MAP_SHARED`.
4. Validates magic and version; if fail, log error and exit non-zero.
5. Enters main loop (specific to daemon type).
6. On SIGTERM, flushes logs, `munmap`, exits with code 0.

Circuit breaker (boot daemon only):

- If a child crashes > 5 times in 60 seconds, stop restarting it.
- Log critical error with child name and PID history.
- Other daemons continue running (degraded mode).

---

## 3. Data Structures

### 3.1 SharedState Struct

```cpp
#include <atomic>
#include <cstdint>

struct alignas(64) SharedState {
    // Header (non-atomic, written once at init)
    uint64_t magic;                     // 0x4E4E4F534C494E4B ("NNOSLINK")
    uint32_t version;                   // 1
    uint32_t reserved0;

    // Physiological signals (updated by state_monitor)
    std::atomic<float> noise_level;             // 0.0-1.0
    std::atomic<float> light_level;             // 0.0-1.0
    std::atomic<uint16_t> notifications_count;  // raw count
    std::atomic<float> heart_rate_variability;  // 0.0-1.0 (proxy)
    std::atomic<uint8_t> self_report_overwhelm; // 0-10

    // Profile identity
    std::atomic<uint8_t> active_profile_id;     // 0-3 or 255 (baseline)
    std::atomic<float> profile_confidence;      // 0.0-1.0

    // Trait dimensions (0-10, updated by profile_refiner daily)
    std::atomic<uint8_t> need_for_structure;
    std::atomic<uint8_t> novelty_seeking;
    std::atomic<uint8_t> hyperfocus_inclination;
    std::atomic<uint8_t> sensory_sensitivity;
    std::atomic<uint8_t> social_energy_capacity;
    std::atomic<uint8_t> exec_function_difficulty;

    // Thresholds and limits (updated by profile_refiner)
    std::atomic<uint8_t> max_concurrent_tasks;
    std::atomic<uint8_t> max_ctx_switches_per_hour;
    std::atomic<uint16_t> deep_work_block_min;
    std::atomic<uint16_t> light_work_block_min;
    std::atomic<uint16_t> min_recovery_block_min;
    std::atomic<uint16_t> max_social_min_per_day;
    std::atomic<float> sensory_alert_threshold;

    // Runtime state (updated by various daemons)
    std::atomic<uint8_t> active_task_count;
    std::atomic<uint8_t> ctx_switches_this_hour;
    std::atomic<uint16_t> social_minutes_today;
    std::atomic<uint8_t> intervention_tier;      // 0-3
    std::atomic<uint8_t> hyperfocus_active;      // 0 or 1
    std::atomic<uint16_t> hyperfocus_minutes;    // duration in current session
    std::atomic<uint8_t> masking_alert;          // 0 or 1

    // Timestamps (seconds since Unix epoch)
    std::atomic<uint32_t> last_state_update;
    std::atomic<uint32_t> last_profile_update;
    std::atomic<uint32_t> last_task_event;
    std::atomic<uint32_t> boot_timestamp;

    // Reserved for future expansion
    uint8_t reserved_region[1024];
};

static_assert(sizeof(SharedState) <= 4096, "SharedState exceeds 4 KB");
```

**Design Notes**:

- `alignas(64)` ensures cache line alignment on both ARM64 and x86_64.
- All mutable fields are `std::atomic<T>` for lock-free access.
- Default memory ordering is `memory_order_seq_cst`; performance-critical
  code MAY use `memory_order_acquire` / `memory_order_release` explicitly.
- Reserved region allows future expansion without breaking binary compatibility.

### 3.2 Archetype Profile Data

Each archetype is a compile-time constant struct:

```cpp
struct ArchetypeProfile {
    uint8_t id;
    const char* name;
    uint8_t traits[6];  // [need_for_structure, novelty_seeking, hyperfocus_inclination,
                        //  sensory_sensitivity, social_energy_capacity, exec_function_difficulty]
    uint8_t max_concurrent_tasks;
    uint8_t max_ctx_switches_per_hour;
    uint16_t deep_work_block_min;
    uint16_t light_work_block_min;
    uint16_t min_recovery_block_min;
    uint16_t max_social_min_per_day;
    float sensory_alert_threshold;
};

constexpr ArchetypeProfile SYSTEMS_HYPERFOCUS = {
    .id = 0,
    .name = "SYSTEMS_HYPERFOCUS",
    .traits = {9, 3, 9, 5, 4, 4},
    .max_concurrent_tasks = 2,
    .max_ctx_switches_per_hour = 2,
    .deep_work_block_min = 90,
    .light_work_block_min = 20,
    .min_recovery_block_min = 30,
    .max_social_min_per_day = 120,
    .sensory_alert_threshold = 0.65f
};

constexpr ArchetypeProfile DIVERGENT_CREATIVE = {
    .id = 1,
    .name = "DIVERGENT_CREATIVE",
    .traits = {3, 9, 6, 4, 6, 7},
    .max_concurrent_tasks = 3,
    .max_ctx_switches_per_hour = 5,
    .deep_work_block_min = 45,
    .light_work_block_min = 15,
    .min_recovery_block_min = 15,
    .max_social_min_per_day = 180,
    .sensory_alert_threshold = 0.70f
};

constexpr ArchetypeProfile SENSORY_SOCIAL_FRAGILE = {
    .id = 2,
    .name = "SENSORY_SOCIAL_FRAGILE",
    .traits = {7, 4, 5, 9, 2, 8},
    .max_concurrent_tasks = 1,
    .max_ctx_switches_per_hour = 2,
    .deep_work_block_min = 60,
    .light_work_block_min = 15,
    .min_recovery_block_min = 45,
    .max_social_min_per_day = 60,
    .sensory_alert_threshold = 0.45f
};

constexpr ArchetypeProfile INTENSE_MOOD_VARIANCE = {
    .id = 3,
    .name = "INTENSE_MOOD_VARIANCE",
    .traits = {6, 5, 7, 6, 5, 6},
    .max_concurrent_tasks = 2,
    .max_ctx_switches_per_hour = 3,
    .deep_work_block_min = 60,
    .light_work_block_min = 20,
    .min_recovery_block_min = 30,
    .max_social_min_per_day = 150,
    .sensory_alert_threshold = 0.55f
};

constexpr ArchetypeProfile ARCHETYPES[4] = {
    SYSTEMS_HYPERFOCUS,
    DIVERGENT_CREATIVE,
    SENSORY_SOCIAL_FRAGILE,
    INTENSE_MOOD_VARIANCE
};
```

### 3.3 Sync Packet Structures

**Pre-Encryption Header (24 bytes)**:

```cpp
struct SyncHeader {
    uint32_t magic;        // 0x4E4E5359 ("NNSY")
    uint8_t version;       // 1
    uint8_t flags;         // reserved
    uint16_t reserved;
    uint64_t device_id;    // stable unique ID per device
    uint32_t sequence;     // monotonic per device
    uint32_t timestamp;    // seconds since epoch
} __attribute__((packed));
```

**Payload (40 bytes)**:

```cpp
struct SyncPayload {
    uint8_t intervention_tier;
    uint8_t masking_alert;
    uint8_t active_profile_id;
    uint8_t active_task_count;
    uint8_t ctx_switches_this_hour;
    uint8_t hyperfocus_active;
    uint16_t reserved;
    float sensory_load;
    float emotional_load;
    float profile_confidence;
    uint32_t last_state_update;
    uint32_t last_profile_update;
    uint8_t reserved2[8];
} __attribute__((packed));
```

**Wire Format**:

```
[ Nonce: 12 bytes ]
[ Ciphertext: sizeof(SyncHeader) + sizeof(SyncPayload) ]
[ GCM Tag: 16 bytes ]
```

Total: 12 + 64 + 16 = 92 bytes per packet.

---

## 4. Algorithms

### 4.1 Profile Matching Algorithm

**Inputs**:
- `observed_traits[6]`: trait values from config or default (5, 5, 5, 5, 5, 5)
- `ARCHETYPES[4]`: compile-time archetype array

**Output**:
- `best_id`: index (0-3) or 255 (baseline fallback)
- `best_confidence`: similarity score (0.0-1.0)

**Algorithm**:

```python
def match_profile(observed_traits):
    best_id = 255  # baseline
    best_confidence = 0.0

    for i, archetype in enumerate(ARCHETYPES):
        similarity = 0.0
        for j in range(6):
            # Linear distance similarity
            distance = abs(observed_traits[j] - archetype.traits[j])
            trait_sim = 1.0 - (distance / 10.0)
            similarity += trait_sim

        # Average across 6 traits
        similarity /= 6.0

        if similarity > best_confidence:
            best_confidence = similarity
            best_id = i

    if best_confidence < 0.6:
        # No archetype meets threshold, use baseline
        return (255, best_confidence)
    else:
        return (best_id, best_confidence)
```

**C++ Implementation Notes**:
- Use integer arithmetic where possible: `distance = abs(obs - arch)`, then
  `trait_sim = (10 - distance) / 10.0`.
- Accumulate similarity as `int` (0-60 range) then divide by 6.0 at end.
- Compare against threshold 0.6 (or 36/60 in integer form).

### 4.2 Load Computation Algorithm

**Inputs** (from SharedState):
- `noise_level` (0.0-1.0)
- `light_level` (0.0-1.0)
- `notifications_count` (uint16)
- `heart_rate_variability` (0.0-1.0)
- `self_report_overwhelm` (0-10)
- `sensory_sensitivity` (0-10, from profile)

**Outputs** (written to SharedState):
- `sensory_load` (0.0-1.0)
- `emotional_load` (0.0-1.0)
- `total_load` (0.0-1.0, derived, not stored directly)

**Algorithm**:

```cpp
float compute_sensory_load(const SharedState& state) {
    const float sens_factor = state.sensory_sensitivity.load() / 10.0f;
    const float max_notifications = 20.0f;

    float notif_contrib = std::min(1.0f, state.notifications_count.load() / max_notifications);

    return (state.noise_level.load() * sens_factor * 0.4f)
         + (state.light_level.load() * sens_factor * 0.3f)
         + (notif_contrib * 0.3f);
}

float compute_emotional_load(const SharedState& state) {
    float hrv_stress = 1.0f - state.heart_rate_variability.load();
    float self_report = state.self_report_overwhelm.load() / 10.0f;

    return (hrv_stress * 0.5f) + (self_report * 0.5f);
}

float compute_total_load(float sensory, float emotional) {
    return (sensory * 0.5f) + (emotional * 0.5f);
}
```

**Notes**:
- Clamp intermediate values to [0.0, 1.0].
- Load computations run every 250 ms (4 Hz).
- No floating-point exceptions; ensure divisors are never zero.

### 4.3 Intervention Tier Determination

**Inputs**:
- `total_load` (0.0-1.0)
- `sensory_alert_threshold` (from profile, e.g., 0.65)

**Output**:
- `new_tier` (0-3)

**Algorithm**:

```cpp
uint8_t determine_tier(float total_load, float threshold) {
    if (total_load < 0.6f * threshold) return 0;  // Normal
    if (total_load < 0.9f * threshold) return 1;  // Mild overload
    if (total_load < threshold)        return 2;  // Significant overload
    return 3;  // Emergency
}
```

**Tier Actions** (handled by state_monitor):

- Tier 0 → 1: Log event, no immediate action.
- Tier 1 → 2: Set flag for task_manager to defer new tasks.
- Tier 2 → 3: Set flag for emergency mode; comm_bridge sets masking_alert if
  capacity is critically low.

### 4.4 Capacity and Demand Scoring

**Capacity Score** (comm_bridge):

```cpp
float compute_capacity(const SharedState& state, uint8_t energy, uint8_t social_battery) {
    float sensory_load = compute_sensory_load(state);
    uint8_t commitments = state.active_task_count.load();

    return (energy / 10.0f * 0.4f)
         + ((10.0f - sensory_load * 10.0f) / 10.0f * 0.3f)
         + (social_battery / 10.0f * 0.2f)
         + ((10.0f - commitments) / 10.0f * 0.1f);
}
```

**Demand Score** (for an incoming request):

```cpp
float compute_demand(uint16_t effort_min, uint8_t importance, uint8_t social_intensity) {
    return (std::min(effort_min, 240) / 240.0f * 0.5f)
         + (importance / 10.0f * 0.3f)
         + (social_intensity / 10.0f * 0.2f);
}
```

**Masking Detection**:

```cpp
bool detect_masking(float capacity, float demand, uint8_t tier, uint8_t active_tasks, uint8_t max_tasks) {
    if (capacity < 0.4f && demand > 0.7f) return true;
    if (active_tasks >= max_tasks && tier >= 2) return true;
    return false;
}
```

**Decision Logic**:

```cpp
enum Decision { ACCEPT, NEGOTIATE_SCOPE, NEGOTIATE_DEADLINE, DECLINE };

Decision recommend_decision(float capacity, float demand) {
    if (capacity >= 0.6f && demand <= 0.6f) return ACCEPT;
    if (capacity < 0.3f && demand > 0.6f) return DECLINE;
    if (capacity < 0.5f) return NEGOTIATE_SCOPE;
    return NEGOTIATE_DEADLINE;
}
```

### 4.5 Context Switch Budget Enforcement

**Inputs**:
- `ctx_switches_this_hour` (from SharedState)
- `max_ctx_switches_per_hour` (from profile)
- Incoming interrupt priority vs. active task priority

**Algorithm**:

```cpp
bool allow_context_switch(const SharedState& state, uint8_t interrupt_priority, uint8_t active_priority) {
    uint8_t budget = state.max_ctx_switches_per_hour.load();
    uint8_t used = state.ctx_switches_this_hour.load();

    if (used < budget) return true;  // Budget available

    // Budget exhausted; allow only if priority difference >= 2
    return (interrupt_priority >= active_priority + 2);
}
```

**Hourly Reset** (context_gate daemon):

Every clock hour (HH:00:00):

```cpp
state.ctx_switches_this_hour.store(0, std::memory_order_release);
log_event("BUDGET_RESET", {{"hour", current_hour}});
```

### 4.6 Hyperfocus Protection and Breaking

**Protect Conditions** (all must be true):

```cpp
bool should_protect_hyperfocus(const SharedState& state) {
    return state.hyperfocus_active.load() == 1
        && state.hyperfocus_minutes.load() < 90  // configurable
        && state.intervention_tier.load() < 2;
}
```

**Break Conditions** (any must be true):

```cpp
bool should_break_hyperfocus(const SharedState& state) {
    if (state.intervention_tier.load() >= 2) return true;
    if (state.self_report_overwhelm.load() >= 8) return true;
    if (state.hyperfocus_minutes.load() >= 120) return true;  // hard max
    return false;
}
```

**Actions**:

- Protect: Defer all interrupts, log protect event.
- Break: Generate break prompt, reset `hyperfocus_active` to 0, log break event with reason.

### 4.7 Profile Refinement Algorithm

Runs daily at 03:00 local time.

**Inputs**:
- Log data from past 30 days (`/var/log/nnos/events.jsonl`)
- Current `SharedState` profile fields

**Steps**:

1. **Parse logs**: Extract TIER_CHANGE, TASK_ACCEPTED, TASK_DEFERRED, CONTEXT_SWITCH, MASKING_DETECTED events.

2. **Compute aggregates**:
   - Average tier per hour of day.
   - Context switch frequency distribution.
   - Task completion rate by demand type.
   - Observed sensory threshold (average load at tier transitions).
   - Hyperfocus duration and frequency.
   - Masking detection count.

3. **Detect deviations**:
   - If observed sensory threshold differs from profile by > 0.1 for >= 7 days:
     Adjust `sensory_alert_threshold` by +/- 0.05.
   - If context switches consistently exceed budget:
     Increase `max_ctx_switches_per_hour` by 1 (cap at 5).
   - If context switches consistently below budget:
     Decrease by 1 (floor at 2).
   - If hyperfocus durations average > 90 min and tier stays < 2:
     Increase `deep_work_block_min` by 15 min (cap at 120).

4. **Trait adjustment**:
   - For each trait, compare observed behavior to archetype expectation.
   - If deviation >= 20% for >= 7 days:
     Adjust trait by +1 or -1 (clamp to [0, 10]).

5. **Re-match archetype**:
   - Run `match_profile` with adjusted traits.
   - If new match confidence > old confidence + 0.15:
     Update `active_profile_id` and all threshold fields.

6. **Persist**:
   - Write refined profile to `/etc/nnos/active_profile.bin` (binary format).
   - Update SharedState with new values.
   - Set `last_profile_update` timestamp.

7. **Log**:
   - Generate PROFILE_REFINED event with old/new traits, old/new archetype,
     old/new confidence, and list of changed fields.

**C++ Pseudocode**:

```cpp
void refine_profile(SharedState& state) {
    // Load and parse logs (30-day window)
    auto aggregates = analyze_logs("/var/log/nnos/events.jsonl", 30);

    // Adjust thresholds
    if (aggregates.sensory_threshold_deviation > 0.1 && aggregates.deviation_days >= 7) {
        float current = state.sensory_alert_threshold.load();
        float adjusted = current + (aggregates.sensory_threshold_deviation > 0 ? 0.05f : -0.05f);
        state.sensory_alert_threshold.store(std::clamp(adjusted, 0.4f, 0.8f));
    }

    // Adjust traits
    uint8_t new_traits[6];
    for (int i = 0; i < 6; i++) {
        new_traits[i] = adjust_trait(state, i, aggregates);
    }

    // Re-match
    auto [new_id, new_conf] = match_profile(new_traits);
    float old_conf = state.profile_confidence.load();

    if (new_conf > old_conf + 0.15f) {
        apply_archetype(state, ARCHETYPES[new_id]);
        state.active_profile_id.store(new_id);
        state.profile_confidence.store(new_conf);
    }

    // Persist and log
    persist_profile("/etc/nnos/active_profile.bin", state);
    state.last_profile_update.store(time(nullptr));
    log_event("PROFILE_REFINED", build_refinement_payload(state, old_conf, new_conf));
}
```

---

## 5. Communication Protocols

### 5.1 Ethernet Multicast Synchronization

**Multicast Group**: `239.73.78.69:20046`

**Broadcast Frequency**: 100 ms (10 Hz)

**Encryption**: AES-256-GCM

- Key: 32-byte PSK from `/etc/nnos/sync.key`
- Nonce: 12 bytes = `device_id (8) || sequence (4)`
- AD (Additional Data): None (header is part of plaintext encrypted)
- Tag: 16 bytes appended to ciphertext

**Send Path** (ethernet_sync daemon):

```cpp
void broadcast_state(const SharedState& state) {
    SyncHeader hdr = build_header(state);
    SyncPayload payload = build_payload(state);

    uint8_t plaintext[64];
    memcpy(plaintext, &hdr, 24);
    memcpy(plaintext + 24, &payload, 40);

    uint8_t nonce[12];
    build_nonce(nonce, device_id, sequence_counter);

    uint8_t ciphertext[64];
    uint8_t tag[16];
    aes256gcm_encrypt(key, nonce, plaintext, 64, ciphertext, tag);

    uint8_t packet[92];
    memcpy(packet, nonce, 12);
    memcpy(packet + 12, ciphertext, 64);
    memcpy(packet + 76, tag, 16);

    sendto(multicast_socket, packet, 92, 0, &multicast_addr, sizeof(multicast_addr));

    sequence_counter++;
}
```

**Receive Path**:

```cpp
void process_incoming_packet(uint8_t* packet, size_t len) {
    if (len != 92) return;  // malformed

    uint8_t nonce[12], ciphertext[64], tag[16];
    memcpy(nonce, packet, 12);
    memcpy(ciphertext, packet + 12, 64);
    memcpy(tag, packet + 76, 16);

    uint8_t plaintext[64];
    if (!aes256gcm_decrypt(key, nonce, ciphertext, 64, tag, plaintext)) {
        log_event("SYNC_DECRYPT_FAIL", {});
        return;  // auth failure
    }

    SyncHeader hdr;
    SyncPayload payload;
    memcpy(&hdr, plaintext, 24);
    memcpy(&payload, plaintext + 24, 40);

    if (hdr.magic != 0x4E4E5359) return;  // wrong magic

    // Anti-replay
    if (!check_sequence(hdr.device_id, hdr.sequence)) {
        log_event("SYNC_REPLAY_DETECTED", {{"device", hdr.device_id}, {"seq", hdr.sequence}});
        return;
    }

    // Merge into SharedState
    merge_state(state, hdr, payload);
}
```

**Merge Rules**:

- Sensor fields: Take newest (compare `last_state_update`).
- Task fields: NEVER overwrite local.
- Profile fields: Take newest (compare `last_profile_update`).

**Anti-Replay**:

- Maintain map: `device_id -> last_seen_sequence`.
- Accept packet only if `incoming_sequence > last_seen[device_id]`.
- Update map on successful decrypt and merge.

### 5.2 TCP Polling Endpoint (Mobile Client)

**Port**: 20047

**Protocol**: Simple request-response over TLS 1.3 (optional for LAN, required for remote).

**Request** (mobile → NNOS):

```
GET /state HTTP/1.1
Host: jetson.local
Authorization: Bearer <token>

```

**Response** (NNOS → mobile):

```
HTTP/1.1 200 OK
Content-Type: application/json

{
  "device_id": "0x123456789ABCDEF0",
  "timestamp": 1708876800,
  "intervention_tier": 1,
  "masking_alert": 0,
  "sensory_load": 0.52,
  "emotional_load": 0.38,
  "active_profile_id": 0,
  "profile_confidence": 0.87,
  "active_task_count": 1,
  "ctx_switches_this_hour": 1,
  "hyperfocus_active": 1,
  "hyperfocus_minutes": 35,
  "social_minutes_today": 45,
  "last_state_update": 1708876798,
  "last_profile_update": 1708790400
}
```

**Implementation**:

- Use a simple HTTP server library (e.g., cpp-httplib, Boost.Beast) or roll your own with raw sockets.
- On connection: read SharedState, serialize to JSON, send response.
- Latency target: < 50 ms from socket accept to response sent.

---

## 6. File System Layout

### 6.1 Directory Structure

```
/etc/nnos/
  ├── sync.key                    # 32-byte AES-256 key (0600)
  ├── observed_traits.ini         # User-provided trait overrides (optional)
  ├── overrides.ini               # Manual threshold overrides (optional)
  ├── trusted_devices             # List of allowed device_ids (optional)
  └── active_profile.bin          # Persisted refined profile (binary)

/var/log/nnos/
  └── events.jsonl                # Structured log stream (rotated by logrotate)

/dev/shm/
  └── nnos_neural_link            # Shared memory region (ephemeral)

/usr/local/bin/
  ├── nnos_boot_daemon
  ├── nnos_state_monitor
  ├── nnos_task_manager
  ├── nnos_context_gate
  ├── nnos_comm_bridge
  ├── nnos_profile_refiner
  └── nnos_ethernet_sync

/lib/systemd/system/
  └── nnos-boot.service           # systemd unit
```

### 6.2 Configuration File Formats

**observed_traits.ini**:

```ini
[traits]
need_for_structure=7
novelty_seeking=4
hyperfocus_inclination=8
sensory_sensitivity=6
social_energy_capacity=3
exec_function_difficulty=5
```

**overrides.ini**:

```ini
[thresholds]
max_concurrent_tasks=2
max_ctx_switches_per_hour=3
sensory_alert_threshold=0.60
```

If present, overrides take precedence over archetype defaults and refinement adjustments.

**trusted_devices** (one hex device_id per line):

```
0x123456789ABCDEF0
0xFEDCBA9876543210
```

If file exists, only packets from listed device_ids are accepted.

---

## 7. Operational Procedures

### 7.1 Installation

1. Install dependencies: `sudo apt install libssl-dev`.
2. Copy binaries to `/usr/local/bin/`.
3. Create `/etc/nnos/` directory.
4. Generate sync key: `openssl rand -out /etc/nnos/sync.key 32 && chmod 0600 /etc/nnos/sync.key`.
5. Create nnos user and group: `sudo useradd -r -s /bin/false nnos`.
6. Set ownership: `sudo chown nnos:nnos /etc/nnos/ -R`.
7. Install systemd service: `sudo cp nnos-boot.service /lib/systemd/system/ && sudo systemctl daemon-reload`.
8. Enable service: `sudo systemctl enable nnos-boot.service`.
9. Start service: `sudo systemctl start nnos-boot.service`.

### 7.2 First Boot

On first boot, if no `/etc/nnos/observed_traits.ini` exists:

- System uses default midpoint traits (all 5).
- Runs profile matching; likely falls back to baseline (255).
- Logs PROFILE_MATCHED event with confidence < 0.6.

User should:

1. Fill out trait questionnaire or self-assessment.
2. Create `/etc/nnos/observed_traits.ini` with values.
3. Restart: `sudo systemctl restart nnos-boot.service`.
4. Check logs: `journalctl -u nnos-boot -f`.
5. Verify matched archetype in logs (PROFILE_MATCHED event).

### 7.3 Daily Operation

- System runs autonomously.
- Logs accumulate in `/var/log/nnos/events.jsonl`.
- Profile refiner runs at 03:00 daily; check logs for PROFILE_REFINED events.
- If behavior feels mismatched, review logs and consider manual trait adjustment.

### 7.4 Manual Trait Adjustment

1. Edit `/etc/nnos/observed_traits.ini`.
2. Restart boot daemon: `sudo systemctl restart nnos-boot.service`.
3. Boot daemon will re-match profile on startup.

### 7.5 Manual Threshold Override

1. Edit `/etc/nnos/overrides.ini`.
2. Restart boot daemon: `sudo systemctl restart nnos-boot.service`.
3. Overrides persist across reboots and refinement cycles.

### 7.6 Log Analysis

Recommended tools:

- `jq` for querying JSON logs: `cat /var/log/nnos/events.jsonl | jq 'select(.event == "TIER_CHANGE")'`
- Custom scripts for aggregation (Python, R, or Julia).
- Grafana + Loki for real-time visualization (future extension).

### 7.7 Troubleshooting

**Problem**: Daemon crashes repeatedly.

- Check: `journalctl -u nnos-boot -n 100`
- Look for: CHILD_RESTART events with exit codes.
- Common causes:
  - Missing `/etc/nnos/sync.key` (ethernet_sync crashes).
  - Incorrect permissions on `/dev/shm/nnos_neural_link`.
  - Network interface down (ethernet_sync can't bind multicast).

**Problem**: Profile matching returns baseline (255) every time.

- Check: `/etc/nnos/observed_traits.ini` exists and is readable.
- Verify: Trait values are integers 0-10.
- Test: Manually set extreme values (all 0 or all 10) and check if confidence improves.

**Problem**: State not syncing across devices.

- Check: Both devices on same LAN, multicast-capable network.
- Verify: `sync.key` is identical on both devices (use `md5sum`).
- Test: Run `tcpdump -i any -n port 20046` to see packets.
- Check: Firewall rules allow UDP 20046.

---

## 8. Security Considerations

### 8.1 Threat Model

**In Scope**:

- Passive network eavesdropping (LAN or WiFi).
- Active man-in-the-middle attacks on sync packets.
- Malicious processes attempting to read `/dev/shm/nnos_neural_link`.
- Unauthorized mobile clients polling TCP endpoint.

**Out of Scope** (for v1):

- Physical access to devices (assumed trusted environment).
- Compromised operating system or kernel.
- Side-channel attacks (timing, power analysis).

### 8.2 Mitigations

**Encryption**:

- All sync packets encrypted with AES-256-GCM.
- TLS 1.3 for TCP polling endpoint (optional for LAN, required for remote).

**Authentication**:

- GCM tag prevents packet tampering.
- Anti-replay using sequence numbers.
- Device whitelist via `/etc/nnos/trusted_devices`.

**Access Control**:

- SharedState readable/writable only by `nnos` user.
- Sync key file permissions 0600.
- Daemons run as non-root `nnos` user.
- systemd hardening directives (NoNewPrivileges, ProtectSystem, PrivateTmp).

**Input Validation**:

- All incoming packets validated: magic, version, sequence, GCM tag.
- Malformed packets logged and dropped.
- No buffer overflows: fixed-size structs, bounds checks.

### 8.3 Key Management

**Key Generation**:

```bash
openssl rand -out /etc/nnos/sync.key 32
chmod 0600 /etc/nnos/sync.key
chown nnos:nnos /etc/nnos/sync.key
```

**Key Rotation** (manual, recommended annually):

1. Generate new key on primary device.
2. Distribute to all devices via secure channel (USB, SSH).
3. Replace `/etc/nnos/sync.key` on all devices.
4. Restart daemons: `sudo systemctl restart nnos-boot.service`.

**Future Extension**: Automated key rotation using Diffie-Hellman or similar.

---

## 9. Testing Strategy

### 9.1 Unit Tests

Each daemon SHALL have unit tests covering:

- SharedState validation (magic, version).
- Load computation formulas (sensory, emotional, total).
- Tier determination logic.
- Profile matching algorithm.
- Capacity/demand scoring.
- Hyperfocus protect/break logic.

Framework: Google Test or Catch2.

Run: `make test` or `ctest`.

### 9.2 Integration Tests

Test daemon constellation as a whole:

- Boot daemon spawns children; all attach to SharedState.
- Inject synthetic signals; verify tier changes propagate.
- Simulate task intake; verify accept/defer decisions.
- Simulate context switch requests; verify budget enforcement.

Framework: Python scripts driving daemons via D-Bus or custom IPC.

### 9.3 Network Tests

Test Ethernet sync:

- Two devices on LAN, sync.key identical.
- Inject state change on Device A.
- Verify propagation to Device B within 150 ms.
- Inject malformed packet; verify drop and log.
- Inject replay packet; verify drop and log.

Framework: Scapy (Python) for packet injection.

### 9.4 Stress Tests

- Run continuously for 7 days; check for memory leaks (Valgrind).
- Inject 10% packet loss; verify state eventually converges.
- Crash and restart daemons 1000 times; verify no SharedState corruption.

### 9.5 Acceptance Test Matrix

| Test ID | Requirement | Pass Criteria |
|---|---|---|
| AT-1 | FR-3.1.11 | Boot completes in < 2 sec on Jetson |
| AT-2 | FR-3.2.6 | Profile published within 500 ms |
| AT-3 | FR-3.4.6 | Tier change within 50 ms |
| AT-4 | FR-3.5.6 | Task decision within 10 ms |
| AT-5 | FR-3.9.3 | All sync packets encrypted with AES-256-GCM |
| AT-6 | FR-3.9.6 | Replay packets dropped, logged |
| AT-7 | AC-6.6 | 1000 restarts, zero corruption |
| AT-8 | AC-6.8 | 7-day run, no leaks, < 1% loss |

---

## 10. Future Extensions

### 10.1 Wearable Sensor Integration

- Bluetooth or USB HID interface to wearables (Garmin, Apple Watch, Oura Ring).
- Real HRV, GSR, skin temperature inputs replace proxies.
- New daemon: `nnos_sensor_gateway`.

### 10.2 Cloud Sync and Telemetry

- Optional encrypted sync to user-controlled cloud storage (Nextcloud, S3).
- Long-term trend analysis and ML-based profile refinement.
- Privacy-first: all data encrypted at rest and in transit.

### 10.3 Mobile App Enhancements

- Push notifications for tier changes, masking alerts.
- Interactive profile adjustment UI.
- Real-time visualizations (load graphs, task timelines).

### 10.4 Multi-User Support

- Separate SharedState regions per user.
- Per-user profiles and logs.
- Daemon constellation per user, or multiplexed single constellation.

### 10.5 AI-Assisted Profile Refinement

- Use on-device LLM to analyze logs and suggest trait adjustments.
- Natural language explanations for refinement decisions.
- Requires careful privacy and compute resource management.

---

## 11. References

- NNOS-SRS-001: Software Requirements Specification
- POSIX.1-2017: IEEE Std 1003.1-2017 (shared memory, threads)
- NIST SP 800-38D: Recommendation for Block Cipher Modes of Operation: Galois/Counter Mode (GCM) and GMAC
- RFC 5869: HMAC-based Extract-and-Expand Key Derivation Function (HKDF)
- systemd documentation: https://www.freedesktop.org/software/systemd/man/
- NVIDIA Jetson Boot Flow: https://docs.nvidia.com/jetson/archives/r36.4.4/DeveloperGuide/AR/BootArchitecture/JetsonOrinSeriesBootFlow.html

---

## 12. Document History

| Version | Date | Author | Changes |
|---|---|---|---|
| 1.0.0 | 2026-02-23 | Evolution Strategist | Initial design document |
| 2.0.0 | 2026-02-23 | Evolution Strategist | Full expansion: algorithms, protocols, operational procedures, testing |

---

END OF NNOS DESIGN SPECIFICATION
