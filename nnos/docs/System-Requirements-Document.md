# NNOS -- Software Requirements Specification
## Neurodivergent Neural-Link Operating System

| Field | Value |
|---|---|
| Document ID | NNOS-SRS-001 |
| Version | 2.0.0 |
| Date | 2026-02-22 |
| Status | APPROVED |
| Author | Evolution Strategist |
| Classification | CONFIDENTIAL |
| Encoding | UTF-8 without BOM, ASCII-safe punctuation only |

---

## ENCODING NOTICE

This document is UTF-8 without BOM. All code generated from this spec MUST use:
- Plain ASCII quotes: " and '
- Plain hyphens: -
- No smart quotes, no em dashes, no special Unicode punctuation
- Explicit UTF-8 encoding declarations where needed

---

## 1. Introduction

### 1.1 Purpose

This Software Requirements Specification (SRS) defines the complete functional
and non-functional requirements for the Neurodivergent Neural-Link Operating
System (NNOS) firmware. NNOS is a deterministic, real-time operating system
layer designed to protect neurodivergent individuals from burnout, sensory
overload, and executive function collapse through continuous physiological
monitoring and intelligent workload regulation.

This document is the authoritative source of truth for what NNOS must do. It
is intended to be consumed by:

- Human engineers implementing the system.
- AI code generation models (Claude, Qwen, DeepSeek, etc.) producing C++ source.
- Quality assurance personnel validating behavior against requirements.

### 1.2 Scope

NNOS runs as a compiled C++ daemon constellation on edge computing devices:

- NVIDIA Jetson Orin Nano (ARM64, JetPack 6.x)
- ASUS NUC 15 Pro (x86_64, Ubuntu 24.04)

It synchronizes state across devices via encrypted Ethernet multicast over a
local area network. Mobile clients (Google Pixel 10 Pro XL, or similar) consume
state via a TCP polling endpoint.

The system operates with sub-second latency and provides deterministic behavioral
interventions based on real-time physiological and contextual data.

NNOS does NOT:
- Execute trades or financial transactions.
- Collect biometric data from external wearable hardware (future extension).
- Require Internet connectivity for core operation.
- Use interpreted languages in any hot path.

### 1.3 Definitions and Acronyms

| Term | Definition |
|---|---|
| Archetype | One of four compiled-in neurodivergent profile templates with distinct trait levels and thresholds |
| SYSTEMS_HYPERFOCUS | Archetype: high structure need, high hyperfocus inclination, low novelty seeking |
| DIVERGENT_CREATIVE | Archetype: high novelty seeking, moderate hyperfocus, low structure need |
| SENSORY_SOCIAL_FRAGILE | Archetype: high sensory sensitivity, low social energy, high exec function difficulty |
| INTENSE_MOOD_VARIANCE | Archetype: high emotional variability, moderate sensory sensitivity, moderate structure need |
| Intervention Tier | Escalation level (0-3) determining system protective response intensity |
| Tier 0 | Normal operation, no protective action needed |
| Tier 1 | Mild overload detected, environment adjustments suggested |
| Tier 2 | Significant overload, guided regulation activated, new tasks deferred |
| Tier 3 | Emergency intervention, all non-critical activity halted |
| Hyperfocus | Sustained deep attention state that can be protective or harmful depending on physiological load and duration |
| Masking | Social camouflaging behavior where a neurodivergent person conceals internal struggles, leading to accelerated burnout |
| Context Switch | Change from one task or attention domain to another; excessive switches cause cognitive overload and recovery time |
| Shared State | Fixed-size memory-mapped region (SharedState struct) synchronized across all NNOS processes on a host, and optionally across devices |
| Sensory Load | Computed metric (0.0-1.0) representing aggregate environmental stimuli intensity |
| Emotional Load | Computed metric (0.0-1.0) representing internal stress/overwhelm level |
| Capacity Score | Computed metric (0.0-1.0) representing how much additional load the user can safely handle |
| Demand Score | Computed metric (0.0-1.0) representing how heavy an incoming request or commitment is |
| Breathing Room | Intentional under-utilization of capacity to preserve buffer for unexpected load |
| SHM | POSIX shared memory (shm_open / mmap) |
| GCM | Galois/Counter Mode (AES-256-GCM authenticated encryption) |
| PSK | Pre-shared key |
| HRV | Heart rate variability (proxy for autonomic nervous system state) |
| GSR | Galvanic skin response (future sensor input) |

### 1.4 System Context

NNOS operates in a multi-device ecosystem:

- Edge devices (Jetson Orin Nano, NUC):
  - Run the full daemon constellation.
  - Perform all computation locally.
  - Host the shared memory region.
  - Communicate via encrypted Ethernet multicast.

- Mobile device (Pixel 10 Pro XL, currently; Pixel 9 Pro in future with custom ROM):
  - Consumes state via TCP polling to NUC or Jetson.
  - Displays intervention tier, masking alerts, and recommendations.
  - Collects self-report inputs (overwhelm level, energy level).
  - Does NOT run NNOS daemons.

- Wearable sensors (future extension):
  - Provide HRV, GSR, skin temperature data via Bluetooth or USB.
  - Fed into state_monitor daemon as additional signal sources.

- LAN infrastructure:
  - 1 Gbps Ethernet or 802.11ax (WiFi 6) for sub-100 ms sync latency.
  - Multicast-capable network (standard for home/office LANs).

### 1.5 Document Conventions

- "SHALL" = mandatory requirement.
- "SHOULD" = recommended but not strictly required.
- "MAY" = optional.
- Each functional requirement is tagged FR-X.Y.Z.
- Each non-functional requirement is tagged NFR-X.Y.Z.
- Each acceptance criterion is tagged AC-X.Y.

---

## 2. Neurodivergent Profile System

### 2.1 Trait Dimensions

NNOS models the user's neurodivergent characteristics across six ordinal trait
dimensions, each scored 0-10:

| Dimension | Low (0-3) | Mid (4-6) | High (7-10) |
|---|---|---|---|
| need_for_structure | Comfortable with ambiguity, fluid planning | Moderate routine preference | Requires rigid routines, distressed by unexpected changes |
| novelty_seeking | Prefers familiar patterns, resists change | Balanced exploration | Constantly seeks new stimuli, bored by repetition |
| hyperfocus_inclination | Rarely enters deep focus states | Occasional hyperfocus | Frequently locks into intense focus, difficult to disengage |
| sensory_sensitivity | Low reactivity to noise, light, crowds | Moderate sensitivity | Easily overwhelmed by sensory input, requires controlled environments |
| social_energy_capacity | Social interaction is deeply draining | Moderate social tolerance | Can sustain social engagement with manageable cost |
| exec_function_difficulty | Minimal difficulty with planning, sequencing, initiation | Some executive challenges | Severe difficulty initiating tasks, maintaining sequences, switching contexts |

### 2.2 Archetype Definitions

Four pre-compiled archetypes encode common neurodivergent profiles:

**SYSTEMS_HYPERFOCUS (ID: 0)**

| Trait | Value |
|---|---|
| need_for_structure | 9 |
| novelty_seeking | 3 |
| hyperfocus_inclination | 9 |
| sensory_sensitivity | 5 |
| social_energy_capacity | 4 |
| exec_function_difficulty | 4 |

Thresholds:
- max_concurrent_tasks: 2
- max_ctx_switches_per_hour: 2
- deep_work_block_min: 90
- light_work_block_min: 20
- min_recovery_block_min: 30
- max_social_min_per_day: 120
- sensory_alert_threshold: 0.65

---

**DIVERGENT_CREATIVE (ID: 1)**

| Trait | Value |
|---|---|
| need_for_structure | 3 |
| novelty_seeking | 9 |
| hyperfocus_inclination | 6 |
| sensory_sensitivity | 4 |
| social_energy_capacity | 6 |
| exec_function_difficulty | 7 |

Thresholds:
- max_concurrent_tasks: 3
- max_ctx_switches_per_hour: 5
- deep_work_block_min: 45
- light_work_block_min: 15
- min_recovery_block_min: 15
- max_social_min_per_day: 180
- sensory_alert_threshold: 0.70

---

**SENSORY_SOCIAL_FRAGILE (ID: 2)**

| Trait | Value |
|---|---|
| need_for_structure | 7 |
| novelty_seeking | 4 |
| hyperfocus_inclination | 5 |
| sensory_sensitivity | 9 |
| social_energy_capacity | 2 |
| exec_function_difficulty | 8 |

Thresholds:
- max_concurrent_tasks: 1
- max_ctx_switches_per_hour: 2
- deep_work_block_min: 60
- light_work_block_min: 15
- min_recovery_block_min: 45
- max_social_min_per_day: 60
- sensory_alert_threshold: 0.45

---

**INTENSE_MOOD_VARIANCE (ID: 3)**

| Trait | Value |
|---|---|
| need_for_structure | 6 |
| novelty_seeking | 5 |
| hyperfocus_inclination | 7 |
| sensory_sensitivity | 6 |
| social_energy_capacity | 5 |
| exec_function_difficulty | 6 |

Thresholds:
- max_concurrent_tasks: 2
- max_ctx_switches_per_hour: 3
- deep_work_block_min: 60
- light_work_block_min: 20
- min_recovery_block_min: 30
- max_social_min_per_day: 150
- sensory_alert_threshold: 0.55

### 2.3 Mixed-Mode Baseline

If no archetype achieves the minimum match confidence, the system falls back
to a conservative baseline:

- max_concurrent_tasks: 1
- max_ctx_switches_per_hour: 2
- deep_work_block_min: 45
- light_work_block_min: 15
- min_recovery_block_min: 30
- max_social_min_per_day: 90
- sensory_alert_threshold: 0.50

All trait values set to 5 (midpoint).

---

## 3. Functional Requirements

### 3.1 Boot and Initialization

FR-3.1.1: The boot daemon SHALL create the POSIX shared memory object
`/dev/shm/nnos_neural_link` with a fixed size sufficient to hold the
SharedState struct plus reserved space.

FR-3.1.2: The boot daemon SHALL zero the entire shared memory region before
writing any data.

FR-3.1.3: The boot daemon SHALL write the magic number (0x4E4E4F534C494E4B,
ASCII "NNOSLINK") and version (1) into the SharedState header.

FR-3.1.4: The boot daemon SHALL write boot_timestamp as seconds since Unix
epoch at the moment of initialization.

FR-3.1.5: The boot daemon SHALL load the four compiled-in archetype profiles
and perform profile matching (see 3.2).

FR-3.1.6: The boot daemon SHALL write the matched profile's id, confidence,
trait values, and thresholds into SharedState.

FR-3.1.7: The boot daemon SHALL fork and supervise six child daemons:
- nnos_state_monitor
- nnos_task_manager
- nnos_context_gate
- nnos_comm_bridge
- nnos_profile_refiner
- nnos_ethernet_sync

FR-3.1.8: The boot daemon SHALL monitor child processes using waitpid and
restart any child that exits within 2 seconds.

FR-3.1.9: The boot daemon SHALL log all child exits with:
- Child daemon name
- PID
- Exit code or signal number
- Restart timestamp

FR-3.1.10: On receiving SIGTERM, the boot daemon SHALL:
- Send SIGTERM to all child processes.
- Wait up to 10 seconds for children to exit cleanly.
- Force-kill (SIGKILL) any remaining children.
- Call munmap and shm_unlink to clean up shared memory.
- Exit with code 0.

FR-3.1.11: The boot daemon SHALL complete all initialization (shared memory
creation, profile matching, child spawning) within 2 seconds on a Jetson
Orin Nano at stock clock speeds.

### 3.2 Profile Matching

FR-3.2.1: The system SHALL load four pre-compiled neurodivergent profile
archetypes at boot time.

FR-3.2.2: The system SHALL accept observed trait values from:
- A configuration file (`/etc/nnos/observed_traits.ini`), or
- Default midpoint values (all traits = 5) if no config exists.

FR-3.2.3: The system SHALL compute a weighted similarity score between
observed traits and each archetype using:
- Jaccard similarity for categorical/binary traits.
- Linear distance (1.0 - abs(observed - archetype) / 10.0) for ordinal traits.
- Equal weighting across all six trait dimensions unless overridden.

FR-3.2.4: The system SHALL select the archetype with the highest similarity
score, provided the score meets or exceeds a minimum confidence threshold
of 0.6.

FR-3.2.5: If no archetype achieves the minimum confidence threshold, the
system SHALL activate the mixed-mode baseline profile defined in Section 2.3.

FR-3.2.6: The system SHALL publish the active profile to shared memory
within 500 ms of boot completion, including:
- active_profile_id
- profile_confidence
- All six trait values
- All threshold values

FR-3.2.7: The system SHALL support daily profile refinement based on
aggregated behavioral logs (see Section 3.7).

### 3.3 SharedState Structure

FR-3.3.1: The SharedState struct SHALL be fixed-size with no variable-length
members and no pointers.

FR-3.3.2: The struct SHALL be aligned to a 64-byte boundary using
`alignas(64)` to prevent false sharing on both ARM64 and x86_64.

FR-3.3.3: All cross-process mutable fields SHALL use `std::atomic<T>` with
explicitly specified memory ordering.

FR-3.3.4: The struct SHALL contain a reserved region of at least 1024 bytes
for future expansion without changing the struct's total size.

FR-3.3.5: The struct SHALL contain the following field groups:

Header:
- magic (uint64_t, non-atomic, written once at init)
- version (uint32_t, non-atomic, written once at init)

Physiological Signals:
- noise_level (atomic float, 0.0-1.0)
- light_level (atomic float, 0.0-1.0)
- notifications_count (atomic uint16_t)
- heart_rate_variability (atomic float, 0.0-1.0)
- self_report_overwhelm (atomic uint8_t, 0-10)

Profile Identity:
- active_profile_id (atomic uint8_t)
- profile_confidence (atomic float, 0.0-1.0)

Trait Dimensions (each atomic uint8_t, 0-10):
- need_for_structure
- novelty_seeking
- hyperfocus_inclination
- sensory_sensitivity
- social_energy_capacity
- exec_function_difficulty

Thresholds and Limits:
- max_concurrent_tasks (atomic uint8_t, 1-4)
- max_ctx_switches_per_hour (atomic uint8_t, 2-5)
- deep_work_block_min (atomic uint16_t, minutes)
- light_work_block_min (atomic uint16_t, minutes)
- min_recovery_block_min (atomic uint16_t, minutes)
- max_social_min_per_day (atomic uint16_t, minutes)
- sensory_alert_threshold (atomic float, 0.0-1.0)

Runtime State:
- active_task_count (atomic uint8_t)
- ctx_switches_this_hour (atomic uint8_t)
- social_minutes_today (atomic uint16_t)
- intervention_tier (atomic uint8_t, 0-3)
- hyperfocus_active (atomic uint8_t, 0 or 1)
- hyperfocus_minutes (atomic uint16_t)
- masking_alert (atomic uint8_t, 0 or 1)

Timestamps (each atomic uint32_t, seconds since epoch):
- last_state_update
- last_profile_update
- last_task_event
- boot_timestamp

Reserved:
- reserved_region (uint8_t[1024])

FR-3.3.6: Every daemon, on startup, SHALL validate SharedState by checking:
- magic == 0x4E4E4F534C494E4B
- version is within a supported range
If validation fails, the daemon SHALL log an error and exit with a
non-zero code. The boot daemon SHALL then restart it.

### 3.4 State Monitoring and Intervention

FR-3.4.1: The state monitor daemon SHALL sample physiological signals at a
minimum of 4 Hz (one sample every 250 ms).

FR-3.4.2: The system SHALL compute sensory_load using:

    sensory_load = (noise_level * sensory_sensitivity/10 * 0.4)
                 + (light_level * sensory_sensitivity/10 * 0.3)
                 + (notifications_count / max_notifications * 0.3)

where max_notifications is a configurable ceiling (default: 20).

FR-3.4.3: The system SHALL compute emotional_load using:

    emotional_load = ((1.0 - heart_rate_variability) * 0.5)
                   + (self_report_overwhelm / 10.0 * 0.5)

FR-3.4.4: The system SHALL compute total_load as:

    total_load = (sensory_load * 0.5) + (emotional_load * 0.5)

FR-3.4.5: The system SHALL determine intervention_tier by comparing
total_load to the active profile's sensory_alert_threshold:

- Tier 0: total_load < 0.6 * threshold
- Tier 1: 0.6 * threshold <= total_load < 0.9 * threshold
- Tier 2: 0.9 * threshold <= total_load < threshold
- Tier 3: total_load >= threshold

FR-3.4.6: The system SHALL publish intervention_tier updates to SharedState
using `memory_order_release` within 50 ms of detecting a tier change.

FR-3.4.7: The system SHALL publish last_state_update timestamp on every
sample cycle.

FR-3.4.8: The system SHALL log every tier change as a structured JSON event
containing:
- old_tier
- new_tier
- sensory_load
- emotional_load
- total_load
- timestamp

FR-3.4.9: On Tier 1, the system SHOULD generate suggestions for environment
adjustment (reduce notifications, dim lights, etc.) via log events.

FR-3.4.10: On Tier 2, the system SHALL activate guided regulation mode:
- Defer new tasks (coordinated with task manager via SharedState).
- Increase recovery prompts.

FR-3.4.11: On Tier 3, the system SHALL activate emergency intervention:
- Halt all non-critical task acceptance.
- Generate urgent log events.
- Set masking_alert if capacity indicators are critically low.

### 3.5 Task Management

FR-3.5.1: The task manager daemon SHALL enforce the active profile's
max_concurrent_tasks limit.

FR-3.5.2: The system SHALL schedule tasks into profile-specific time slots:
- Deep work slots: high-focus tasks requiring deep_work_block_min minutes.
- Light work slots: low-cognitive-load tasks, light_work_block_min minutes.
- Rest slots: no task scheduling, min_recovery_block_min minutes.

FR-3.5.3: The system SHALL classify incoming tasks by demand type:
- DEEP_FOCUS
- CREATIVE
- ROUTINE
- SOCIAL

FR-3.5.4: The system SHALL match task demand type to profile strengths
and vulnerabilities:
- DEEP_FOCUS tasks preferred when hyperfocus_inclination >= 7.
- SOCIAL tasks limited when social_energy_capacity <= 3.
- CREATIVE tasks preferred when novelty_seeking >= 7.

FR-3.5.5: The system SHALL defer task acceptance if ANY of the following
are true:
- active_task_count >= max_concurrent_tasks
- Current time slot is incompatible with the task demand type
- intervention_tier >= 2

FR-3.5.6: The system SHALL return a decision (ACCEPT or DEFER) within 10 ms.

FR-3.5.7: On ACCEPT, the system SHALL:
- Increment active_task_count atomically.
- Update last_task_event timestamp.
- Log the acceptance with task id and demand type.

FR-3.5.8: On DEFER, the system SHALL:
- NOT modify active_task_count.
- Log the deferral with task id, demand type, and reason (which condition
  triggered deferral).

FR-3.5.9: On task completion (signaled by the user or an external event),
the system SHALL:
- Decrement active_task_count atomically.
- Update last_task_event timestamp.
- Generate a task handoff note if the task was interrupted rather than
  completed.

FR-3.5.10: Task handoff notes SHALL contain:
- Task identifier
- Last completed step index
- Next step index
- Timestamp
- Reason for interruption (tier change, budget exhaustion, user-initiated)

### 3.6 Context Gate

FR-3.6.1: The context gate daemon SHALL enforce the active profile's
max_ctx_switches_per_hour limit.

FR-3.6.2: The system SHALL track ctx_switches_this_hour in SharedState
and reset it to zero at the start of each clock hour.

FR-3.6.3: On an incoming context switch request (interrupt, notification,
meeting, etc.), the system SHALL:
- Check ctx_switches_this_hour against max_ctx_switches_per_hour.
- If budget remaining: allow and increment counter.
- If budget exhausted: defer unless incoming priority >= active priority + 2.

FR-3.6.4: The system SHALL protect hyperfocus states when ALL of the
following are true:
- hyperfocus_active == 1
- hyperfocus_minutes < 90 (configurable per profile)
- intervention_tier < 2
- Current task aligns with profile strength flags

FR-3.6.5: The system SHALL break hyperfocus when ANY of the following
are true:
- intervention_tier >= 2 (physiological risk detected)
- User self-reports dissociation (self_report_overwhelm >= 8)
- hyperfocus_minutes exceeds 120 (configurable max, default 2 hours)

FR-3.6.6: During healthy hyperfocus (protected, not broken), the system
SHALL generate gentle reminders every 30 minutes:
- "Hydrate check"
- "Posture check"
- "Eye rest reminder (20-20-20 rule)"

FR-3.6.7: The system SHALL log all context gate decisions:
- Allowed switches (with remaining budget).
- Deferred interrupts (with reason).
- Hyperfocus protect decisions.
- Hyperfocus break decisions (with trigger reason).

### 3.7 Profile Refinement

FR-3.7.1: The profile refiner daemon SHALL run a daily refinement cycle
at 03:00 local time.

FR-3.7.2: The system SHALL analyze the previous 30 days of structured
log data to compute:
- Average intervention tier per hour of day.
- Context switch frequency distribution.
- Task completion rate by demand type.
- Observed sensory threshold vs. configured profile threshold.
- Average hyperfocus duration and frequency.
- Masking detection frequency.

FR-3.7.3: The system SHALL adjust trait levels by +/-1 step if observed
behavior diverges from the active archetype's expected behavior by more
than 20% for 7 or more consecutive days.

FR-3.7.4: The system SHALL re-match adjusted traits against all four
archetypes and update active_profile_id if the new best-match confidence
exceeds the current match by >= 0.15.

FR-3.7.5: The system SHALL preserve any manual overrides stored in
`/etc/nnos/overrides.ini` indefinitely; the refiner SHALL NOT modify
fields that have manual overrides.

FR-3.7.6: The system SHALL persist the refined profile to disk at
`/etc/nnos/active_profile.bin` so it survives reboots.

FR-3.7.7: The system SHALL update SharedState with the refined profile
and publish last_profile_update timestamp.

FR-3.7.8: The system SHALL log all refinement actions:
- Which traits changed and by how much.
- Old vs. new archetype (if changed).
- Old vs. new confidence.
- Which manual overrides were preserved.

### 3.8 Anti-Masking and Communication Bridge

FR-3.8.1: The comm bridge daemon SHALL detect masking behavior when ANY
of the following are true:
- capacity_score < 0.4 AND demand_score > 0.7
- active_task_count >= max_concurrent_tasks AND intervention_tier >= 2
- social_minutes_today > max_social_min_per_day

FR-3.8.2: The system SHALL compute capacity_score as:

    capacity = (energy / 10.0 * 0.4)
             + ((10.0 - sensory_load * 10.0) / 10.0 * 0.3)
             + (social_battery / 10.0 * 0.2)
             + ((10.0 - commitments) / 10.0 * 0.1)

where energy and social_battery come from self-report or defaults,
and commitments = active_task_count.

FR-3.8.3: The system SHALL compute demand_score as:

    demand = (effort_min / 240.0 * 0.5)
           + (importance / 10.0 * 0.3)
           + (social_intensity / 10.0 * 0.2)

where effort_min, importance, and social_intensity describe the incoming
request.

FR-3.8.4: The system SHALL recommend one of four communication decisions:
- ACCEPT: capacity >= 0.6 AND demand <= 0.6
- NEGOTIATE_SCOPE: capacity < 0.5, suggest 50% effort reduction
- NEGOTIATE_DEADLINE: deadline pressure is primary constraint, suggest
  1.5x time extension
- DECLINE: capacity < 0.3 AND demand > 0.6

FR-3.8.5: The system SHALL set masking_alert = 1 in SharedState when
masking is detected per FR-3.8.1.

FR-3.8.6: masking_alert SHALL remain set until explicitly cleared by:
- A state_monitor cycle where capacity recovers above 0.5, or
- A manual clear event from the user.

FR-3.8.7: The system SHALL log all masking detections with:
- capacity_score
- demand_score
- Which masking condition triggered
- Recommended decision
- Timestamp

### 3.9 Ethernet Synchronization

FR-3.9.1: The ethernet sync daemon SHALL broadcast a SharedState summary
via UDP multicast to 239.73.78.69:20046 every 100 ms.

FR-3.9.2: The system SHALL encrypt all multicast packets using AES-256-GCM
with a pre-shared key loaded from `/etc/nnos/sync.key`.

FR-3.9.3: The sync key SHALL be exactly 32 bytes, generated using a
cryptographically secure random number generator (e.g., OpenSSL
RAND_bytes).

FR-3.9.4: Each encrypted packet SHALL include:
- Nonce: 12 bytes, derived from device_id and sequence number.
- Ciphertext: encrypted header + payload.
- Authentication tag: 16 bytes (GCM tag).

FR-3.9.5: The pre-encryption packet structure SHALL be:

Header (24 bytes):
- magic (4 bytes): 0x4E4E5359
- version (1 byte)
- flags (1 byte, reserved)
- reserved (2 bytes)
- device_id (8 bytes, stable per device)
- sequence (4 bytes, monotonic per device per boot)
- timestamp (4 bytes, seconds since epoch)

Payload (40 bytes):
- intervention_tier (uint8)
- masking_alert (uint8)
- sensory_load (float32)
- emotional_load (float32)
- profile_confidence (float32)
- active_profile_id (uint8)
- active_task_count (uint8)
- ctx_switches_this_hour (uint8)
- hyperfocus_active (uint8)
- last_state_update (uint32)
- last_profile_update (uint32)
- reserved (remaining bytes to 40)

FR-3.9.6: The system SHALL reject incoming packets when:
- magic != 0x4E4E5359
- GCM authentication tag verification fails
- sequence <= last_seen_sequence for that device_id (anti-replay)
- device_id is not in `/etc/nnos/trusted_devices` (if configured)

FR-3.9.7: The system SHALL merge received state using timestamp-based
conflict resolution:
- Sensor fields (noise, light, HRV, intervention_tier, etc.):
  Take from device with newest last_state_update.
- Task fields (active_task_count, ctx_switches_this_hour, etc.):
  NEVER overwrite local values.
- Profile fields (active_profile_id, confidence, traits, thresholds):
  Take from device with newest last_profile_update.

FR-3.9.8: The system SHALL provide a TCP polling endpoint on port 20047
for mobile device queries.

FR-3.9.9: The TCP endpoint SHALL respond to a connection with an encrypted
SharedState JSON snapshot within 50 ms.

FR-3.9.10: The system SHALL maintain a per-device sequence counter map
in memory for anti-replay, initialized to zero at boot.

### 3.10 Daemon Supervision

FR-3.10.1: The boot daemon SHALL fork and supervise six child processes:
- nnos_state_monitor
- nnos_task_manager
- nnos_context_gate
- nnos_comm_bridge
- nnos_profile_refiner
- nnos_ethernet_sync

FR-3.10.2: The boot daemon SHALL restart any crashed child within 2 seconds.

FR-3.10.3: The boot daemon SHALL log all child lifecycle events (start,
stop, crash, restart) with PID, exit code, signal, and timestamps.

FR-3.10.4: The boot daemon SHALL create the shared memory region before
spawning any children, ensuring all children can validate and attach.

FR-3.10.5: The boot daemon SHALL clean up shared memory and terminate all
children on SIGTERM, as described in FR-3.1.10.

FR-3.10.6: If a child crashes more than 5 times within 60 seconds, the
boot daemon SHALL log a critical error and stop restarting that child
(circuit breaker), while keeping other daemons running.

### 3.11 Logging and Observability

FR-3.11.1: All daemons SHALL log structured JSON lines to
`/var/log/nnos/events.jsonl`.

FR-3.11.2: Each log line SHALL contain at minimum:
- ts: ISO-8601 timestamp
- daemon: name of the logging daemon
- level: one of "debug", "info", "warn", "error", "critical"
- event: event type identifier (e.g., "TIER_CHANGE", "TASK_ACCEPTED",
  "MASKING_DETECTED", "CHILD_RESTART", "PROFILE_REFINED")
- payload: JSON object with event-specific fields

FR-3.11.3: Log files SHALL be rotated by size or time using OS-level
logrotate.

FR-3.11.4: Logs SHALL NOT contain:
- Cryptographic keys or key material.
- Full biometric data streams.
- Any information that could identify the user to a third party beyond
  what is necessary for system operation.

---

## 4. Non-Functional Requirements

### 4.1 Performance

NFR-4.1.1: State monitor SHALL publish intervention_tier updates within
50 ms of detecting a load threshold crossing.

NFR-4.1.2: Task manager SHALL return accept/defer decisions within 10 ms.

NFR-4.1.3: Ethernet sync SHALL achieve < 150 ms end-to-end propagation
latency over a 1 Gbps LAN for 99% of packets.

NFR-4.1.4: Profile matching SHALL complete within 500 ms on a Jetson Orin
Nano (Cortex-A78AE at 1.5 GHz).

NFR-4.1.5: All atomic operations in hot paths SHALL use explicit memory
ordering (acquire/release) rather than defaulting to sequential consistency.

NFR-4.1.6: No daemon main loop SHALL allocate heap memory after
initialization is complete.

### 4.2 Reliability

NFR-4.2.1: The daemon constellation SHALL achieve 99.5% uptime over a
rolling 30-day window (total unplanned downtime <= 3.6 hours per month).

NFR-4.2.2: A crash of any single daemon SHALL NOT corrupt SharedState.
Magic number and version SHALL be validated on every open.

NFR-4.2.3: Network packet loss of up to 10% SHALL NOT cause cross-device
state divergence greater than 500 ms.

NFR-4.2.4: The system SHALL survive a cold boot with corrupt log files
by skipping or regenerating them without affecting daemon startup.

NFR-4.2.5: The system SHALL survive a cold boot with a missing or empty
`/etc/nnos/observed_traits.ini` by using default midpoint traits.

### 4.3 Security

NFR-4.3.1: The sync key SHALL be generated with a cryptographically secure
RNG (e.g., OpenSSL RAND_bytes or /dev/urandom).

NFR-4.3.2: The sync key file (`/etc/nnos/sync.key`) SHALL have permissions
0600 (owner read/write only).

NFR-4.3.3: Multicast packets SHALL NOT be decryptable without the
pre-shared key.

NFR-4.3.4: The system SHALL drop packets from unknown device_ids if
`/etc/nnos/trusted_devices` is configured.

NFR-4.3.5: SharedState SHALL be accessible only to the `nnos` user/group.

NFR-4.3.6: Daemons SHALL run as a dedicated non-root user (`nnos`).

NFR-4.3.7: systemd service files SHALL include hardening directives:
- NoNewPrivileges=true
- PrivateTmp=true
- ProtectSystem=full
- ProtectHome=true

### 4.4 Maintainability

NFR-4.4.1: All C++ source SHALL compile warning-free at
`-Wall -Wextra -Wpedantic` on both GCC 11+ and Clang 14+.

NFR-4.4.2: Code SHALL use C++17 only; no C++20 features (for Jetson
toolchain compatibility).

NFR-4.4.3: Code SHALL compile with `-fno-exceptions -fno-rtti`.

NFR-4.4.4: Configuration SHALL use simple text formats (INI or similar)
that are human-readable and version-controllable.

NFR-4.4.5: Each daemon SHALL support a `--simulate` flag for testing
without real sensor inputs.

### 4.5 Portability

NFR-4.5.1: Core code SHALL be portable between x86_64 and ARM64 without
platform-specific #ifdefs in business logic.

NFR-4.5.2: Platform-specific code (e.g., real-time scheduling priorities)
SHALL be isolated behind adapter functions.

---

## 5. Constraints

C-5.1: Language: C++17 only.
C-5.2: No exceptions: compile with -fno-exceptions.
C-5.3: No RTTI: compile with -fno-rtti.
C-5.4: No dynamic allocation in daemon main loops after init.
C-5.5: Dependencies limited to: POSIX APIs, OpenSSL 3.x, standard C++ library.
C-5.6: No interpreted language runtimes in any daemon process.

---

## 6. Acceptance Criteria

AC-6.1: System boots and initializes SharedState (magic, version, profile)
in < 2 seconds on Jetson Orin Nano.

AC-6.2: All six child daemons attach to SharedState and pass validation
within 1 second of boot daemon completing init.

AC-6.3: Profile matching achieves >= 0.80 confidence for test vectors
designed to match each archetype.

AC-6.4: Intervention tier escalation occurs within 500 ms of a synthetic
load threshold crossing in 95% of test samples.

AC-6.5: Ethernet sync propagates state updates to a second device within
150 ms in 99% of packets on a 1 Gbps LAN.

AC-6.6: Zero SharedState corruption observed after 1,000 sequential daemon
restart cycles.

AC-6.7: Anti-masking detection achieves >= 80% precision and >= 70% recall
against a human-labeled test set of capacity/demand scenarios.

AC-6.8: System survives 7-day continuous operation with < 1% packet loss
and no memory leaks (verified with Valgrind or AddressSanitizer).

AC-6.9: Task manager correctly defers all tasks when intervention_tier >= 2
across 100 synthetic test cases.

AC-6.10: Context gate correctly protects and breaks hyperfocus in all
defined scenarios (FR-3.6.4 and FR-3.6.5) across 50 synthetic test cases.

AC-6.11: Profile refiner adjusts traits only when deviation threshold is
met and preserves all manual overrides across 30 simulated refinement cycles.

---

## 7. Traceability Matrix

| Requirement | Component | Test |
|---|---|---|
| FR-3.1.* | boot_daemon | AC-6.1, AC-6.2 |
| FR-3.2.* | profile_matcher | AC-6.3 |
| FR-3.3.* | shared_memory | AC-6.6 |
| FR-3.4.* | state_monitor | AC-6.4 |
| FR-3.5.* | task_manager | AC-6.9 |
| FR-3.6.* | context_gate | AC-6.10 |
| FR-3.7.* | profile_refiner | AC-6.11 |
| FR-3.8.* | comm_bridge | AC-6.7 |
| FR-3.9.* | ethernet_sync | AC-6.5 |
| FR-3.10.* | boot_daemon | AC-6.6 |
| FR-3.11.* | all daemons | AC-6.8 |

---

## 8. Document History

| Version | Date | Author | Changes |
|---|---|---|---|
| 1.0.0 | 2026-02-22 | Evolution Strategist | Initial draft |
| 2.0.0 | 2026-02-22 | Evolution Strategist | Full expansion: archetypes, formulas, wire format, traceability, acceptance criteria |

---

## 9. References

- NVIDIA Jetson Orin Boot Flow: https://docs.nvidia.com/jetson/archives/r36.4.4/DeveloperGuide/AR/BootArchitecture/JetsonOrinSeriesBootFlow.html
- POSIX Shared Memory: IEEE Std 1003.1-2017
- AES-GCM: NIST SP 800-38D
- systemd service management: https://www.freedesktop.org/software/systemd/man/daemon.html
