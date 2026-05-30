# NNOS -- Software Requirements Specification
## Neurodivergent Neural-Link Operating System

| Field | Value |
|---|---|
| **Document ID** | NNOS-SRS-001 |
| **Version** | 1.0.0 |
| **Date** | 2026-02-22 |
| **Status** | APPROVED |
| **Author** | Evolution Strategist |
| **Classification** | CONFIDENTIAL |

---

## 1. Introduction

### 1.1 Purpose

This Software Requirements Specification (SRS) defines the functional and non-functional requirements for the Neurodivergent Neural-Link Operating System (NNOS) firmware. NNOS is a deterministic, real-time operating system layer designed to protect neurodivergent individuals from burnout, sensory overload, and executive function collapse through continuous physiological monitoring and intelligent workload regulation.

### 1.2 Scope

NNOS runs as a compiled C++ daemon constellation on edge computing devices (NVIDIA Jetson Orin Nano, ASUS NUC 15 Pro, Google Pixel 9 Pro) and synchronizes state across devices via encrypted Ethernet multicast. The system operates with sub-second latency and provides deterministic behavioral interventions based on real-time physiological and contextual data.

### 1.3 Definitions and Acronyms

| Term | Definition |
|---|---|
| **Archetype** | One of four compiled-in neurodivergent profile templates (SYSTEMS_HYPERFOCUS, DIVERGENT_CREATIVE, SENSORY_SOCIAL_FRAGILE, INTENSE_MOOD_VARIANCE) |
| **Intervention Tier** | Escalation level (0-3) determining system protective response |
| **Hyperfocus** | Sustained attention state that can be protective or harmful depending on physiological load |
| **Masking** | Social camouflaging behavior where a neurodivergent person conceals struggles, leading to burnout |
| **Context Switch** | Change from one task to another; excessive switches cause cognitive overload |
| **Shared State** | 4KB memory-mapped region synchronized across all devices |
| **Sensory Load** | Computed metric (0.0-1.0) representing environmental stimuli intensity |

### 1.4 System Context

NNOS operates in a multi-device ecosystem:
- **Edge devices** (Jetson, NUC): Run full daemon constellation, perform computation, host shared memory
- **Mobile device** (Pixel 9 Pro): Consumes state via TCP polling, displays interventions, collects self-reports
- **Wearable sensors** (future): Provide HRV, GSR, temperature data via Bluetooth/USB
- **LAN infrastructure**: 1Gbps Ethernet or 802.11ax WiFi 6 for sub-100ms sync latency

---

## 2. Functional Requirements

### 2.1 Profile Matching and Initialization

**FR-2.1.1**: The system SHALL load four pre-compiled neurodivergent profile archetypes at boot.

**FR-2.1.2**: The system SHALL perform weighted similarity matching between observed traits and archetypes using Jaccard similarity for categorical traits and linear distance for ordinal traits.

**FR-2.1.3**: The system SHALL select the archetype with highest similarity score (confidence >= 0.6) or fall back to a mixed-mode baseline if no archetype achieves threshold.

**FR-2.1.4**: The system SHALL publish the active profile to shared memory within 500ms of boot completion.

**FR-2.1.5**: The system SHALL support daily profile refinement based on aggregated behavioral logs.

### 2.2 State Monitoring and Intervention

**FR-2.2.1**: The system SHALL sample physiological signals at minimum 4 Hz (250ms intervals).

**FR-2.2.2**: The system SHALL compute sensory load as:

sensory_load = (noise * sensitivity * 0.4) + (light * sensitivity * 0.3) + (notifications * 0.3)

**FR-2.2.3**: The system SHALL compute emotional load as:

emotional_load = ((1.0 - HRV) * 0.5) + (self_report_overwhelm / 10 * 0.5)


**FR-2.2.4**: The system SHALL determine intervention tier based on total load relative to profile-specific threshold:
- **Tier 0**: load < 0.6 * threshold (normal operation)
- **Tier 1**: 0.6 * threshold <= load < 0.9 * threshold (adjust environment)
- **Tier 2**: 0.9 * threshold <= load < threshold (guided regulation)
- **Tier 3**: load >= threshold (emergency intervention)

**FR-2.2.5**: The system SHALL publish intervention tier updates to shared memory with memory_order_release within 50ms of tier change.

### 2.3 Task Management and Executive Function Support

**FR-2.3.1**: The system SHALL enforce maximum concurrent task limits per active profile (range: 1-4 tasks).

**FR-2.3.2**: The system SHALL schedule tasks into profile-specific time slots:
- Deep work slots: high-focus tasks requiring 45-90 minute blocks
- Light work slots: low-cognitive-load tasks, 15-30 minutes
- Rest slots: no task scheduling, recovery periods

**FR-2.3.3**: The system SHALL match task demands (DEEP_FOCUS, CREATIVE, ROUTINE, SOCIAL) to profile strengths and vulnerabilities.

**FR-2.3.4**: The system SHALL defer task acceptance if:
- Active task count >= max_concurrent_tasks
- Current time slot incompatible with task demand
- Intervention tier >= 2

**FR-2.3.5**: The system SHALL generate task handoff notes containing:
- Task ID
- Last completed step index
- Next step index
- Timestamp for re-entry alignment

### 2.4 Context Switching and Hyperfocus Management

**FR-2.4.1**: The system SHALL enforce hourly context switch limits per active profile (range: 2-5 switches).

**FR-2.4.2**: The system SHALL protect hyperfocus states when:
- Hyperfocus duration < 90 minutes
- Intervention tier < 2
- Task aligns with profile strength flags

**FR-2.4.3**: The system SHALL break hyperfocus when:
- Intervention tier >= 2 (physiological risk detected)
- User self-reports dissociation
- Hyperfocus duration exceeds 2 hours without break

**FR-2.4.4**: The system SHALL provide gentle reminders every 30 minutes during healthy hyperfocus:
- "Hydrate check"
- "Posture check"
- "Eye rest reminder"

**FR-2.4.5**: The system SHALL defer incoming interrupts if context switch budget exhausted, unless incoming priority >= active priority + 2.

### 2.5 Communication Bridge and Anti-Masking

**FR-2.5.1**: The system SHALL detect masking behavior when:
- Capacity score < 0.4 AND demand score > 0.7
- Active tasks >= max_concurrent_tasks AND intervention tier >= 2
- Social minutes today > max_social_minutes_per_day

**FR-2.5.2**: The system SHALL compute capacity score as:

capacity = (energy/10 * 0.4) + ((10 - sensory_load)/10 * 0.3) + (social_battery/10 * 0.2) + ((10 - commitments)/10 * 0.1)

**FR-2.5.3**: The system SHALL compute demand score as:

demand = (effort_min/240 * 0.5) + (importance/10 * 0.3) + (social_intensity/10 * 0.2)


**FR-2.5.4**: The system SHALL recommend one of four communication decisions:
- **ACCEPT**: capacity >= 0.6, demand <= 0.6
- **NEGOTIATE_SCOPE**: capacity < 0.5, suggest 50% effort reduction
- **NEGOTIATE_DEADLINE**: deadline too tight, suggest 1.5x extension
- **DECLINE**: capacity < 0.3, demand > 0.6

**FR-2.5.5**: The system SHALL set masking_alert flag in shared memory when masking detected.

### 2.6 Profile Refinement

**FR-2.6.1**: The system SHALL run daily profile refinement at 03:00 local time.

**FR-2.6.2**: The system SHALL analyze previous 30 days of logs to compute:
- Average intervention tier per hour
- Context switch frequency distribution
- Task completion rate by demand type
- Observed sensory threshold vs. profile threshold

**FR-2.6.3**: The system SHALL adjust trait levels by +/-1 step if observed behavior diverges from archetype by >20% for 7+ days.

**FR-2.6.4**: The system SHALL re-match adjusted traits to archetypes and update active_profile_id if new match confidence exceeds current by >= 0.15.

**FR-2.6.5**: The system SHALL preserve manual overrides in /etc/nnos/overrides.ini indefinitely.

### 2.7 Ethernet Synchronization

**FR-2.7.1**: The system SHALL broadcast shared state updates via UDP multicast to 239.73.78.69:20046 every 100ms.

**FR-2.7.2**: The system SHALL encrypt all network packets with AES-256-GCM using pre-shared key from /etc/nnos/sync.key.

**FR-2.7.3**: The system SHALL include nonce (12 bytes) and authentication tag (16 bytes) with every encrypted packet.

**FR-2.7.4**: The system SHALL reject packets with:
- Invalid magic number (0x4E4E5359)
- Sequence number <= last seen from device_id (anti-replay)
- Authentication tag verification failure

**FR-2.7.5**: The system SHALL merge received state using timestamp-based conflict resolution:
- Sensor fields: take from device with newest last_state_update
- Task fields: NEVER overwrite local (device-specific)
- Profile fields: take from device with newest last_profile_update

**FR-2.7.6**: The system SHALL provide TCP polling endpoint on port 20047 for mobile device queries.

**FR-2.7.7**: The system SHALL respond to TCP queries with encrypted SharedState snapshot within 50ms.

### 2.8 Daemon Supervision

**FR-2.8.1**: The boot_daemon SHALL fork and supervise six child processes:
- nnos_state_monitor
- nnos_task_manager
- nnos_context_gate
- nnos_comm_bridge
- nnos_profile_refiner
- nnos_ethernet_sync

**FR-2.8.2**: The boot_daemon SHALL restart any crashed child within 2 seconds.

**FR-2.8.3**: The boot_daemon SHALL log all child exits with PID, exit code, and restart timestamp.

**FR-2.8.4**: The boot_daemon SHALL create /dev/shm/nnos_neural_link shared memory region before spawning children.

**FR-2.8.5**: The boot_daemon SHALL clean up shared memory and terminate all children on SIGTERM.

---

## 3. Non-Functional Requirements

### 3.1 Performance

**NFR-3.1.1**: State monitor SHALL publish intervention tier updates within 50ms of load threshold crossing.

**NFR-3.1.2**: Task intake SHALL return accept/defer decision within 10ms.

**NFR-3.1.3**: Ethernet sync SHALL achieve < 100ms end-to-end propagation latency over 1Gbps LAN.

**NFR-3.1.4**: Profile matching SHALL complete within 500ms on Jetson Orin Nano (Cortex-A78AE @ 1.5GHz).

**NFR-3.1.5**: All atomic operations SHALL use explicit memory ordering (no default seq_cst in hot paths).

### 3.2 Reliability

**NFR-3.2.1**: System SHALL achieve 99.9% uptime (<= 43 minutes downtime per month).

**NFR-3.2.2**: Daemon crashes SHALL NOT corrupt shared memory (magic number validation on every open).

**NFR-3.2.3**: Network packet loss <= 10% SHALL NOT cause state divergence > 500ms.

**NFR-3.2.4**: System SHALL survive cold boot with corrupt /var/log/nnos/* by regenerating logs.

### 3.3 Security

**NFR-3.3.1**: Sync key SHALL be generated with cryptographically secure RNG (OpenSSL RAND_bytes).

**NFR-3.3.2**: Sync key file SHALL have 0600 permissions (owner read/write only).

**NFR-3.3.3**: Multicast packets SHALL NOT be decryptable without the pre-shared key.

**NFR-3.3.4**: System SHALL drop packets from unknown device_id if not in /etc/nnos/trusted_devices.

**NFR-3.3.5**: Shared memory SHALL be accessible only to nnos group (GID TBD).

### 3.4 Maintainability

**NFR-3.4.1**: All source files SHALL compile independently with zero warnings at -Wall -Wextra -Wpedantic.

**NFR-3.4.2**: Code SHALL contain NO dynamic allocation after initialization.

**NFR-3.4.3**: All daemon loops SHALL use only noexcept functions.

**NFR-3.4.4**: System SHALL log structured JSON events to /var/log/nnos/events.jsonl for offline analysis.

### 3.5 Portability

**NFR-3.5.1**: System SHALL compile on ARM64 (aarch64) and x86_64 with same CMake project.

**NFR-3.5.2**: System SHALL run on Ubuntu 22.04+ and Jetson Linux 36.4+.

**NFR-3.5.3**: System SHALL require only POSIX.1-2017 + OpenSSL 3.0+ (no platform-specific extensions).

### 3.6 Real-Time Constraints

**NFR-3.6.1**: State monitor SHALL run at SCHED_FIFO priority 40.

**NFR-3.6.2**: Context gate SHALL run at SCHED_FIFO priority 30.

**NFR-3.6.3**: All other daemons SHALL run at SCHED_OTHER with nice -10.

**NFR-3.6.4**: Shared memory pages SHALL be locked into RAM (RLIMIT_MEMLOCK = infinity).

**NFR-3.6.5**: System SHALL NOT use swap (all allocations from fixed-size buffers).

---

## 4. System Interfaces

### 4.1 Hardware Interfaces

**SI-4.1.1**: USB HID interface for future wearable sensor integration (out of scope for v1.0).

**SI-4.1.2**: Ethernet NIC for multicast sync (1Gbps recommended, 100Mbps minimum).

**SI-4.1.3**: I2C/SPI bus for Jetson GPIO sensor expansion (future).

### 4.2 Software Interfaces

**SI-4.2.1**: POSIX shared memory (shm_open, mmap) for inter-daemon communication.

**SI-4.2.2**: systemd socket activation for TCP polling endpoint (future optimization).

**SI-4.2.3**: Syslog (or journald) for structured logging.

**SI-4.2.4**: OpenSSL 3.0 libcrypto for AES-GCM encryption.

### 4.3 Communication Interfaces

**SI-4.3.1**: UDP multicast on 239.73.78.69:20046 (TTL = 1, LAN-local).

**SI-4.3.2**: TCP server on 0.0.0.0:20047 (single-threaded, accept -> send -> close).

**SI-4.3.3**: No outbound Internet connections (air-gapped system).

---

## 5. Operational Scenarios

### 5.1 Normal Operation Flow

1. User powers on Jetson Orin Nano at 08:00
2. UEFI -> kernel -> systemd starts nnos-boot.service at sysinit.target
3. Boot daemon creates /dev/shm/nnos_neural_link, matches profile to SYSTEMS_HYPERFOCUS (confidence 0.87)
4. Six child daemons spawn, ethernet_sync joins multicast group
5. At 09:15, user opens IDE (deep work task), task_manager accepts into deep work slot
6. State monitor detects rising sensory load (0.62), sets tier 1, no action needed
7. At 10:45, notification arrives, context_gate defers (switch budget: 1/2 used)
8. User self-reports overwhelm = 7 via Pixel app, state monitor escalates to tier 2
9. Task manager suggests break, context_gate generates handoff note for IDE session
10. User takes 15-minute break, tier drops to 0, work resumes with handoff

### 5.2 Hyperfocus Protection

1. User enters hyperfocus on architecture document at 14:00 (intervention tier 0)
2. Context gate detects hyperfocus_active = true, sets protection mode
3. At 14:30, Slack notification arrives (importance 3 vs. active task importance 8)
4. Context gate defers notification, logs "hyperfocus protected"
5. At 15:00, state monitor detects HRV drop to 0.35, escalates to tier 2
6. Context gate breaks hyperfocus, generates handoff, forces 20-minute recovery block

### 5.3 Cross-Device Sync

1. Jetson runs state_monitor, detects tier 2 at 11:23:45.123
2. Jetson's ethernet_sync broadcasts encrypted packet at 11:23:45.200
3. NUC receives packet at 11:23:45.287 (87ms latency)
4. NUC validates, merges tier 2 into local shared memory
5. NUC's task_manager immediately defers new task intake
6. User's Pixel polls NUC at 11:23:46.500, receives tier 2 state
7. Pixel displays notification: "System detected overload. Resting now."

---

## 6. Data Requirements

### 6.1 Shared State Persistence

**DR-6.1.1**: Shared memory is volatile; cleared on reboot.

**DR-6.1.2**: Profile refinement results SHALL persist to /etc/nnos/active_profile.bin (binary serialization of NeuroProfile struct).

**DR-6.1.3**: Event logs SHALL persist to /var/log/nnos/events.jsonl (append-only, rotated daily).

### 6.2 Configuration Files

**DR-6.2.1**: /etc/nnos/sync.key -- 32-byte AES-256 key (binary)

**DR-6.2.2**: /etc/nnos/trusted_devices -- newline-separated list of device_id (uint64 hex)

**DR-6.2.3**: /etc/nnos/overrides.ini -- user-specified trait overrides (INI format)

### 6.3 Log Retention

**DR-6.3.1**: events.jsonl SHALL rotate at 100MB or daily, whichever comes first.

**DR-6.3.2**: Rotated logs SHALL compress with gzip.

**DR-6.3.3**: Compressed logs SHALL retain for 90 days.

---

## 7. Constraints

### 7.1 Technical Constraints

**TC-7.1.1**: C++17 only (no C++20 features; Jetson cross-compiler limitation).

**TC-7.1.2**: No exceptions (-fno-exceptions for deterministic timing).

**TC-7.1.3**: No RTTI (-fno-rtti to reduce binary size).

**TC-7.1.4**: No heap allocation in daemon main loops (fixed-size buffers only).

**TC-7.1.5**: No std::string in hot paths (use const char* or char[256]).

### 7.2 Regulatory Constraints

**RC-7.2.1**: System is NOT a medical device; no FDA/CE approval required.

**RC-7.2.2**: System SHALL NOT store PHI (no names, no SSN, device_id is pseudonymous).

**RC-7.2.3**: Encryption uses approved algorithms (AES-256, NIST FIPS 197).

### 7.3 Business Constraints

**BC-7.3.1**: Open-source release planned for Q3 2026 (license TBD).

**BC-7.3.2**: Must remain free to compile and deploy (no proprietary dependencies).

---

## 8. Acceptance Criteria

**AC-8.1**: System boots and initializes shared memory in < 2 seconds on Jetson Orin Nano.

**AC-8.2**: Profile matching achieves >= 0.80 confidence for 90% of test users in pilot study.

**AC-8.3**: Intervention tier escalation occurs within 500ms of load threshold crossing in 95% of samples.

**AC-8.4**: Ethernet sync propagates state updates to all devices within 150ms in 99% of packets on 1Gbps LAN.

**AC-8.5**: Zero shared memory corruption observed after 1,000 daemon restarts.

**AC-8.6**: Anti-masking detection achieves >= 80% precision and >= 70% recall against human-labeled test cases.

**AC-8.7**: System survives 7-day continuous operation with < 1% packet loss and no memory leaks.

---

## 9. Traceability Matrix

| Requirement ID | Design Spec Section | Test Case |
|---|---|---|
| FR-2.1.1 | DS-2.2 | TC-001 |
| FR-2.1.2 | DS-2.2 | TC-002 |
| FR-2.2.4 | DS-2.3 | TC-010 |
| FR-2.4.3 | DS-2.5 | TC-018 |
| FR-2.7.1 | DS-4.1 | TC-030 |
| NFR-3.1.1 | DS-5.2 | TC-045 |

(Full matrix in separate document NNOS-TM-001)

---

**Document Approval**

| Role | Name | Date | Signature |
|---|---|---|---|
| Author | Evolution Strategist | 2026-02-22 | [Digitally Signed] |
| Technical Review | Claude AI | 2026-02-22 | [Approved] |
| Stakeholder Approval | [Pending] | [Pending] | [Pending] |

---

**Revision History**

| Version | Date | Author | Changes |
|---|---|---|---|
| 1.0.0 | 2026-02-22 | Evolution Strategist | Initial SRS for NNOS firmware with Ethernet sync |

