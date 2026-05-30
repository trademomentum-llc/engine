# NNOS Complete Implementation Package
## All Files in One Document - UTF-8 Without BOM

**Instructions**: Copy each section below into separate files as indicated.

---

# FILE 1: START_HERE.md

Save this as: `START_HERE.md`

```markdown
# NNOS: Start Here Guide
## Complete Walkthrough for First-Time Implementation

**You are here**: You have the specs, and you want working firmware.

**Goal**: Feed these specs into an AI code generator (Claude Code, Qwen Coder, DeepSeek, etc.) and get compiled daemons running on your Jetson or NUC.

---

## What You're Building

A **constellation of 6 C++ daemons** that:
1. Monitor your physiology in real-time (heart rate, sensory load, etc.)
2. Prevent burnout by enforcing task limits and context switch budgets
3. Detect and warn against "masking" (overcommitting when exhausted)
4. Synchronize state across all your devices via encrypted Ethernet

All compiled, no interpreted code, boots with your system.

---

## Prerequisites

### Hardware
- **Jetson Orin Nano** OR **ASUS NUC 15 Pro** (or any x86_64/ARM64 Linux box)
- 1Gbps Ethernet or WiFi 6 (for device sync)

### Software
- Ubuntu 22.04+ or Jetson Linux 36.4+
- GCC 11+ or Clang 14+
- CMake 3.18+
- OpenSSL 3.0+
- Git

Install on Ubuntu/Debian:
```bash
sudo apt update
sudo apt install -y build-essential cmake git \
  libssl-dev iconv file
```

---

## Step-by-Step Process

### STEP 1: Choose Your AI Code Generator

Pick **ONE** of these:

| Tool | Best For | How to Use |
|------|----------|------------|
| **Claude Code** | Clean, well-documented code | Chat interface, paste specs, ask "implement NNOS" |
| **Qwen2.5 Coder** | Fast, local execution | Run locally with Ollama: `ollama run qwen2.5-coder:32b` |
| **DeepSeek Coder** | Large-scale refactoring | API or local: `ollama run deepseek-coder:33b` |
| **OpenCoder** | Specialized for system code | Download from HuggingFace, run inference |

**Recommendation for beginners**: Use **Claude Code** (Anthropic's web interface) -- it's the most forgiving and gives great explanations.

---

### STEP 2: Feed the Specs to Your AI

You'll give the AI **three documents** in this order:

1. **NNOS-Requirements-Spec.md** (what to build)
2. **NNOS-Design-Spec.md** (how it's architected)
3. **NNOS-Technical-Spec.md** (exact implementation details)

**How to do this** (example with Claude Code):

```
You: "I need you to implement the NNOS firmware exactly as specified in these three documents. Start with Phase 1: Headers."

[Paste NNOS-Requirements-Spec.md]

[Wait for Claude to acknowledge]

You: "Here's the design spec:"

[Paste NNOS-Design-Spec.md]

[Wait for acknowledgment]

You: "And here's the technical implementation guide. Follow the order in Phase 1-4:"

[Paste NNOS-Technical-Spec.md]

You: "Now implement include/nnos/common.hpp first."
```

---

### STEP 3: AI Generates Code Files

The AI will output code **one file at a time** in the order specified:

**Phase 1** (Headers):
- `include/nnos/common.hpp`
- `include/nnos/neuro_profile.hpp`
- `include/nnos/profile_matcher.hpp`
- `include/nnos/shared_memory.hpp`
- `include/nnos/task_types.hpp`
- `include/nnos/sync_protocol.hpp`

**Phase 2** (Daemons):
- `src/boot_daemon.cpp`
- `src/state_monitor.cpp`
- `src/task_manager.cpp`
- `src/context_gate.cpp`
- `src/comm_bridge.cpp`
- `src/profile_refiner.cpp`
- `src/ethernet_sync.cpp`

**Phase 3** (Build system):
- `CMakeLists.txt`
- `systemd/nnos-boot.service`
- `scripts/install_jetson.sh`

**What you do**: Copy each generated file into your local `nnos/` directory, maintaining the folder structure.

---

### STEP 4: Compile and Test

```bash
cd nnos/

# Normalize encodings (removes BOM, ensures UTF-8)
./scripts/bootstrap_encoding.sh .

# Build
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

# Check for errors
echo $?  # Should print "0"
```

**If you get errors**:
1. Copy the error message
2. Paste it back to the AI: "I got this error: [paste]. Fix it."
3. AI gives you corrected code
4. Replace the file and rebuild

---

### STEP 5: Install on Your Device

**For Jetson Orin Nano**:
```bash
cd nnos/
./scripts/install_jetson.sh
sudo reboot
```

**For NUC**:
```bash
cd nnos/
./scripts/install_nuc.sh
sudo reboot
```

After reboot, check if it's running:
```bash
sudo systemctl status nnos-boot
# Should say "active (running)"

cat /dev/shm/nnos_neural_link | xxd | head
# Should show magic bytes: 4e 4e 4f 53 4c 49 4e 4b
```

---

### STEP 6: Generate Sync Key (For Multi-Device Setup)

On your **first device** (e.g., Jetson):
```bash
cd nnos/
./scripts/generate_sync_key.sh
# Creates /etc/nnos/sync.key
```

Copy this key to your **other devices** (e.g., NUC):
```bash
scp /etc/nnos/sync.key user@nuc:/tmp/sync.key

# On NUC:
sudo mkdir -p /etc/nnos
sudo mv /tmp/sync.key /etc/nnos/sync.key
sudo chmod 0600 /etc/nnos/sync.key
sudo systemctl restart nnos-boot
```

Now your devices sync their state every 100ms over encrypted Ethernet multicast.

---

## Troubleshooting

### "cmake: command not found"
```bash
sudo apt install cmake
```

### "cannot find -lssl"
```bash
sudo apt install libssl-dev
```

### "Daemon keeps crashing"
Check logs:
```bash
sudo journalctl -u nnos-boot -f
```

Look for error messages, paste them to your AI for fixes.

### "How do I know it's working?"
Watch live intervention tiers:
```bash
sudo journalctl -u nnos-boot -f | grep TIER_CHANGE
```

You should see tier changes as sensory load fluctuates.

---

## What Happens Next

Once installed, NNOS:
1. **Boots automatically** with your system (via systemd)
2. **Matches your neurotype** to one of 4 archetypes (or refines daily)
3. **Monitors** your physiological state 4 times per second
4. **Protects** you by deferring tasks when overloaded
5. **Syncs** with other devices so your NUC knows when your Jetson detected burnout

Your Pixel 9 Pro (future integration) will poll the NUC for current state and display interventions.

---

## Quick Reference Card

| Action | Command |
|--------|---------|
| Install deps | `sudo apt install build-essential cmake libssl-dev git iconv` |
| Generate code | Paste 3 specs to Claude Code / Qwen / DeepSeek |
| Build | `mkdir build && cd build && cmake .. && make` |
| Install | `./scripts/install_jetson.sh` (or `install_nuc.sh`) |
| Check status | `sudo systemctl status nnos-boot` |
| View logs | `sudo journalctl -u nnos-boot -f` |
| Generate key | `./scripts/generate_sync_key.sh` |

---

## Questions?

This is a **deterministic system**: same inputs -> same outputs. If something doesn't work:

1. Check the error message
2. Paste it to your AI: "Fix this error: [message]"
3. AI gives corrected code
4. Replace file, rebuild

You're not expected to understand C++ -- the AI does that. You're the **pilot**: you tell it what to build, verify it compiles, and deploy it.

---

**Ready?** Start with STEP 1 above. Choose your AI and feed it the three specs below.
```

---

# FILE 2: NNOS-Requirements-Spec.md

Save this as: `docs/NNOS-Requirements-Spec.md`

```markdown
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

## ENCODING NOTICE
This document is UTF-8 without BOM. All code generated from this spec must use:
- Plain ASCII quotes: " and '
- No smart punctuation
- Explicit UTF-8 encoding declarations where needed

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
```
sensory_load = (noise x sensitivity x 0.4) + (light x sensitivity x 0.3) + (notifications x 0.3)
```

**FR-2.2.3**: The system SHALL compute emotional load as:
```
emotional_load = ((1.0 - HRV) x 0.5) + (self_report_overwhelm / 10 x 0.5)
```

**FR-2.2.4**: The system SHALL determine intervention tier based on total load relative to profile-specific threshold:
- **Tier 0**: load < 0.6 x threshold (normal operation)
- **Tier 1**: 0.6 x threshold <= load < 0.9 x threshold (adjust environment)
- **Tier 2**: 0.9 x threshold <= load < threshold (guided regulation)
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
```
capacity = (energy/10 x 0.4) + ((10-sensory_load)/10 x 0.3) + 
           (social_battery/10 x 0.2) + ((10-commitments)/10 x 0.1)
```

**FR-2.5.3**: The system SHALL compute demand score as:
```
demand = (effort_min/240 x 0.5) + (importance/10 x 0.3) + (social_intensity/10 x 0.2)
```

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

**FR-2.6.4**: The system SHALL re-match adjusted traits to archetypes and update active_profile_id if new match confidence exceeds current by >=0.15.

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

**NFR-3.1.3**: Ethernet sync SHALL achieve <100ms end-to-end propagation latency over 1Gbps LAN.

**NFR-3.1.4**: Profile matching SHALL complete within 500ms on Jetson Orin Nano (Cortex-A78AE @ 1.5GHz).

**NFR-3.1.5**: All atomic operations SHALL use explicit memory ordering (no default seq_cst in hot paths).

### 3.2 Reliability

**NFR-3.2.1**: System SHALL achieve 99.9% uptime (<=43 minutes downtime per month).

**NFR-3.2.2**: Daemon crashes SHALL NOT corrupt shared memory (magic number validation on every open).

**NFR-3.2.3**: Network packet loss <=10% SHALL NOT cause state divergence >500ms.

**NFR-3.2.4**: System SHALL survive cold boot with corrupt /var/log/nnos/* by regenerating logs.

### 3.3 Security

**NFR-3.3.1**: Sync key SHALL be generated with cryptographically secure RNG (OpenSSL RAND_bytes).

**NFR-3.3.2**: Sync key file SHALL have 0600 permissions (owner read/write only).

**NFR-3.3.3**: Multicast packets SHALL NOT be decryptable without the pre-shared key.

**NFR-3.3.4**: System SHALL drop packets from unknown device_id if not in /etc/nnos/trusted_devices.

**NFR-3.3.5**: Shared memory SHALL be accessible only to nnos group (GID TBD).

---

## 4. Acceptance Criteria

**AC-4.1**: System boots and initializes shared memory in <2 seconds on Jetson Orin Nano.

**AC-4.2**: Profile matching achieves >=0.80 confidence for 90% of test users in pilot study.

**AC-4.3**: Intervention tier escalation occurs within 500ms of load threshold crossing in 95% of samples.

**AC-4.4**: Ethernet sync propagates state updates to all devices within 150ms in 99% of packets on 1Gbps LAN.

**AC-4.5**: Zero shared memory corruption observed after 1,000 daemon restarts.

**AC-4.6**: Anti-masking detection achieves >=80% precision and >=70% recall against human-labeled test cases.

**AC-4.7**: System survives 7-day continuous operation with <1% packet loss and no memory leaks.
```

---

# FILE 3: NNOS-Design-Spec.md

Save this as: `docs/NNOS-Design-Spec.md`

```markdown
# NNOS -- Design Specification
## Neurodivergent Neural-Link Operating System Architecture

| Field | Value |
|---|---|
| **Document ID** | NNOS-DS-001 |
| **Version** | 1.0.0 |
| **Date** | 2026-02-22 |
| **Status** | APPROVED |

---

## ENCODING NOTICE
UTF-8 without BOM. All implementations must use plain ASCII for code/comments.

---

## 1. System Architecture

NNOS uses a microkernel-inspired daemon constellation with lock-free shared memory.

```
Boot Daemon -> SharedMemory <- 6 Child Daemons
                    |
                    v
            Ethernet Multicast (239.73.78.69:20046)
```

## 2. Component Specifications

### 2.1 Shared Memory Layout

```cpp
struct alignas(64) SharedState {
    uint64_t magic;                              // 0x4E4E4F534C494E4B
    uint32_t version;                            // 1
    
    // Physiological signals
    std::atomic<float> heart_rate_variability;   // 0.0-1.0
    std::atomic<float> noise_level;              // 0.0-1.0
    std::atomic<float> light_level;              // 0.0-1.0
    std::atomic<uint16_t> notifications_count;
    std::atomic<uint8_t> self_report_overwhelm;  // 0-10
    
    // Active profile
    std::atomic<uint8_t> active_profile_id;
    std::atomic<float> profile_confidence;
    
    // Trait dimensions
    std::atomic<uint8_t> need_for_structure;
    std::atomic<uint8_t> novelty_seeking;
    std::atomic<uint8_t> hyperfocus_inclination;
    std::atomic<uint8_t> sensory_sensitivity;
    std::atomic<uint8_t> social_energy_capacity;
    std::atomic<uint8_t> exec_function_difficulty;
    
    // Thresholds
    std::atomic<uint8_t> max_concurrent_tasks;
    std::atomic<uint16_t> deep_work_block_min;
    std::atomic<uint16_t> light_work_block_min;
    std::atomic<uint16_t> min_recovery_block_min;
    std::atomic<uint8_t> max_ctx_switches_per_hour;
    std::atomic<uint16_t> max_social_min_per_day;
    std::atomic<float> sensory_alert_threshold;
    
    // Runtime counters
    std::atomic<uint8_t> active_task_count;
    std::atomic<uint8_t> ctx_switches_this_hour;
    std::atomic<uint16_t> social_minutes_today;
    std::atomic<uint8_t> intervention_tier;
    std::atomic<uint8_t> hyperfocus_active;
    std::atomic<uint32_t> hyperfocus_minutes;
    std::atomic<uint8_t> masking_alert;
    
    // Timestamps
    std::atomic<uint32_t> last_state_update;
    std::atomic<uint32_t> last_profile_update;
    std::atomic<uint32_t> last_task_event;
    std::atomic<uint32_t> boot_timestamp;
};
```

### 2.2 Ethernet Sync Protocol

**Packet Structure**: 196 bytes total
- Header (24B): magic, device_id, sequence, timestamp
- State payload (144B): SharedState snapshot
- Nonce (12B): AES-GCM IV
- Ciphertext (168B): Encrypted header+state
- Auth tag (16B): GCM authentication tag

**Encryption**: AES-256-GCM with pre-shared key from /etc/nnos/sync.key

**Anti-replay**: Per-device monotonic sequence numbers

**Merge strategy**:
- Sensor fields: newest timestamp wins
- Task fields: never overwrite local
- Profile fields: newest profile_update wins
```

---

# FILE 4: NNOS-Technical-Spec.md

Save this as: `docs/NNOS-Technical-Spec.md`

```markdown
# NNOS -- Technical Specification
## Implementation Guide for Claude Code

| Field | Value |
|---|---|
| **Document ID** | NNOS-TS-001 |
| **Version** | 1.0.0 |
| **Date** | 2026-02-22 |

---

## ENCODING NOTICE
UTF-8 without BOM. Generate all C++ files with ASCII-only characters.

---

## 1. Project Structure

```
nnos/
|-- CMakeLists.txt
|-- include/nnos/
|   |-- common.hpp
|   |-- neuro_profile.hpp
|   |-- profile_matcher.hpp
|   |-- shared_memory.hpp
|   |-- task_types.hpp
|   `-- sync_protocol.hpp
|-- src/
|   |-- boot_daemon.cpp
|   |-- state_monitor.cpp
|   |-- task_manager.cpp
|   |-- context_gate.cpp
|   |-- comm_bridge.cpp
|   |-- profile_refiner.cpp
|   `-- ethernet_sync.cpp
|-- systemd/
|   |-- nnos-boot.service
|   `-- nnos-state-monitor.service
|-- scripts/
|   |-- bootstrap_encoding.sh
|   |-- generate_sync_key.sh
|   |-- install_jetson.sh
|   `-- install_nuc.sh
`-- tests/
    |-- test_profile_matcher.cpp
    |-- test_shared_memory.cpp
    |-- test_task_manager.cpp
    |-- test_state_monitor.cpp
    |-- test_context_gate.cpp
    `-- test_ethernet_sync.cpp
```

## 2. Implementation Order

### Phase 1: Headers (CRITICAL ORDER)
1. common.hpp
2. neuro_profile.hpp
3. profile_matcher.hpp
4. shared_memory.hpp
5. task_types.hpp
6. sync_protocol.hpp

### Phase 2: Daemons (After headers complete)
1. boot_daemon.cpp
2. state_monitor.cpp
3. task_manager.cpp
4. context_gate.cpp
5. comm_bridge.cpp
6. profile_refiner.cpp
7. ethernet_sync.cpp

### Phase 3: Build System
1. CMakeLists.txt
2. systemd unit files
3. install scripts

### Phase 4: Tests
All test files in parallel

## 3. Build Instructions

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
sudo make install
```

## 4. Compiler Flags

```cmake
add_compile_options(
    -O2 -Wall -Wextra -Wpedantic
    -fno-exceptions -fno-rtti
    -std=c++17
)
```

## 5. Link Libraries

- `-lrt` (POSIX shared memory)
- `-lpthread` (threading)
- `-lssl -lcrypto` (OpenSSL for AES-GCM)

## 6. Validation

After each phase:
- Headers: `g++ -c include/nnos/*.hpp -std=c++17`
- Daemons: `make && echo $?` should return 0
- Tests: `make test && ctest` all pass
```

---

# FILE 5: bootstrap_encoding.sh

Save this as: `scripts/bootstrap_encoding.sh` and make executable: `chmod +x scripts/bootstrap_encoding.sh`

```bash
#!/usr/bin/env bash
# bootstrap_encoding.sh
# Normalize all repo text files to UTF-8 (no BOM)

set -euo pipefail

ROOT_DIR="${1:-.}"

echo "[NNOS] Normalizing text encodings under: $ROOT_DIR"

cd "$ROOT_DIR"

# Find all text files (tracked by git)
FILES=$(git ls-files \
  '*.cpp' '*.hpp' '*.h' '*.c' \
  '*.md' '*.txt' '*.ini' '*.sh' '*.service' '*.cmake' \
  'CMakeLists.txt' 2>/dev/null || echo "")

if [ -z "$FILES" ]; then
  echo "[NNOS] No git-tracked files found. Run 'git init && git add .' first."
  exit 1
fi

for f in $FILES; do
  if [ ! -f "$f" ]; then
    continue
  fi

  # Detect current encoding
  enc=$(file --mime-encoding -b "$f" 2>/dev/null || echo "utf-8")

  # Convert to UTF-8
  tmp="${f}.utf8.tmp"
  if iconv -f "$enc" -t utf-8 "$f" -o "$tmp" 2>/dev/null; then
    # Strip BOM (EF BB BF)
    sed -i '1s/^\xEF\xBB\xBF//' "$tmp" 2>/dev/null || true
    mv "$tmp" "$f"
    echo "[NNOS] $f -> UTF-8 (no BOM)"
  else
    echo "[NNOS] WARN: iconv failed for $f, leaving as-is" >&2
    rm -f "$tmp"
  fi
done

echo "[NNOS] Encoding normalization complete."
```

---

## How to Use This Document

1. **Copy each FILE section** into separate files with the indicated names
2. Create the directory structure:
   ```bash
   mkdir -p docs scripts
   ```
3. Save each section:
   - START_HERE.md -> root directory
   - NNOS-Requirements-Spec.md -> docs/
   - NNOS-Design-Spec.md -> docs/
   - NNOS-Technical-Spec.md -> docs/
   - bootstrap_encoding.sh -> scripts/ (make executable)

4. **Follow START_HERE.md** - it has the complete walkthrough

---

**All content is UTF-8 without BOM. No smart quotes, no special characters.**
