# NNOS -- Technical Implementation Specification
## Neurodivergent Neural-Link Operating System Implementation Guide

| Field | Value |
|---|---|
| Document ID | NNOS-TECH-001 |
| Version | 2.0.0 |
| Date | 2026-02-23 |
| Status | APPROVED |
| Related Specs | NNOS-SRS-001, NNOS-DS-001 |
| Encoding | UTF-8 without BOM, ASCII-safe punctuation |

---

## 1. Development Environment

### 1.1 Target Platforms

**NVIDIA Jetson Orin Nano**

- Architecture: ARM64 (aarch64)
- CPU: NVIDIA Jetson Orin (ARM Cortex-A78AE @ 1.5 GHz, 6 cores)
- OS: JetPack 6.0 (Ubuntu 22.04 LTS based)
- Compiler: GCC 11.4.0 or Clang 14.0.0
- Purpose: Primary real-time monitoring node
- Boot Flow Reference: https://docs.nvidia.com/jetson/archives/r36.4.4/DeveloperGuide/AR/BootArchitecture/JetsonOrinSeriesBootFlow.html

**ASUS NUC 15 Pro**

- Architecture: x86_64 (AMD64)
- CPU: Intel Core i7-1360P or similar (12th/13th gen)
- OS: Ubuntu 24.04 LTS
- Compiler: GCC 13.1.0 or Clang 16.0.0
- Purpose: Primary governance and coordination node

### 1.2 Toolchain Requirements

**Compiler**

- GCC 11.0+ or Clang 14.0+
- Must support C++17 fully
- Must support `-fno-exceptions -fno-rtti`

**Build System**

- CMake 3.18 or higher
- Ninja (recommended) or Make

**Required Libraries**

- OpenSSL 3.0+ (for AES-256-GCM)
  - libssl-dev (Debian/Ubuntu)
  - openssl-devel (RHEL/Fedora)
- POSIX threads (pthread) - typically part of glibc
- POSIX shared memory (librt) - typically part of glibc

**Optional Dev Tools**

- Valgrind (memory leak detection)
- AddressSanitizer (compile-time sanitizer via `-fsanitize=address`)
- gdb / lldb (debugging)
- strace (system call tracing)
- perf (performance profiling on Linux)

### 1.3 Development Workflow

1. **Code on host** (x86_64 Linux workstation or WSL2)
2. **Cross-compile for Jetson** using JetPack SDK Docker container or native toolchain
3. **Deploy to targets** via rsync or scp
4. **Test on device** with systemd or manual launch
5. **Validate logs** in `/var/log/nnos/events.jsonl`
6. **Iterate** based on log analysis and behavior validation

---

## 2. Repository Structure

### 2.1 Complete Directory Layout

```
nnos/
├── CMakeLists.txt                  # Root build config
├── README.md                        # Quick start guide
├── LICENSE                          # Project license
├── .gitignore
│
├── docs/
│   ├── NNOS-SRS-001.md             # Software Requirements Specification
│   ├── NNOS-DS-001.md              # Design Specification
│   └── NNOS-TECH-001.md            # Technical Implementation Specification (this file)
│
├── include/nnos/
│   ├── common.hpp                   # Constants, enums, utility macros
│   ├── config.hpp                   # Configuration file parsing
│   ├── logging.hpp                  # Structured JSON logging
│   ├── shared_state.hpp             # SharedState struct and helpers
│   ├── neuro_profile.hpp            # Archetype definitions
│   ├── profile_matcher.hpp          # Profile matching algorithm
│   ├── state_monitor.hpp            # State monitoring logic
│   ├── task_manager.hpp             # Task management logic
│   ├── context_gate.hpp             # Context switch and hyperfocus logic
│   ├── comm_bridge.hpp              # Anti-masking and communication decisions
│   ├── profile_refiner.hpp          # Daily profile refinement
│   └── ethernet_sync.hpp            # Ethernet synchronization and encryption
│
├── src/
│   ├── common.cpp                   # Implementation of common utilities
│   ├── config.cpp                   # Config file parsing implementation
│   ├── logging.cpp                  # JSON logging implementation
│   ├── shared_state.cpp             # SharedState initialization and validation
│   ├── neuro_profile.cpp            # Archetype table definitions
│   ├── profile_matcher.cpp          # Matching algorithm implementation
│   ├── boot_daemon.cpp              # Main for nnos_boot_daemon
│   ├── state_monitor.cpp            # Main for nnos_state_monitor
│   ├── task_manager.cpp             # Main for nnos_task_manager
│   ├── context_gate.cpp             # Main for nnos_context_gate
│   ├── comm_bridge.cpp              # Main for nnos_comm_bridge
│   ├── profile_refiner.cpp          # Main for nnos_profile_refiner
│   └── ethernet_sync.cpp            # Main for nnos_ethernet_sync
│
├── systemd/
│   └── nnos-boot.service            # systemd service unit for boot daemon
│
├── scripts/
│   ├── bootstrap_encoding.sh        # Encoding validation and setup
│   ├── generate_sync_key.sh         # Generate AES-256 key for sync
│   └── deploy.sh                    # Deployment script to targets
│
├── tests/
│   ├── CMakeLists.txt               # Test build config
│   ├── test_shared_state.cpp        # Unit tests for SharedState
│   ├── test_profile_matcher.cpp     # Unit tests for profile matching
│   ├── test_state_monitor.cpp       # Unit tests for state monitor logic
│   ├── test_task_manager.cpp        # Unit tests for task manager
│   ├── test_context_gate.cpp        # Unit tests for context gate
│   ├── test_comm_bridge.cpp         # Unit tests for comm bridge
│   └── test_ethernet_sync.cpp       # Unit tests for ethernet sync
│
└── config/
    ├── config.ini.example           # Example main config
    ├── observed_traits.ini.example  # Example observed traits
    └── overrides.ini.example        # Example manual overrides
```

### 2.2 Build Output

After build:

```
nnos/
├── build/
│   ├── bin/
│   │   ├── nnos_boot_daemon
│   │   ├── nnos_state_monitor
│   │   ├── nnos_task_manager
│   │   ├── nnos_context_gate
│   │   ├── nnos_comm_bridge
│   │   ├── nnos_profile_refiner
│   │   └── nnos_ethernet_sync
│   └── tests/
│       ├── test_shared_state
│       ├── test_profile_matcher
│       ├── test_state_monitor
│       ├── test_task_manager
│       ├── test_context_gate
│       ├── test_comm_bridge
│       └── test_ethernet_sync
```

---

## 3. CMake Build Configuration

### 3.1 Root CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.18)
project(NNOS VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -fno-rtti")

# Build type defaults
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Find required packages
find_package(OpenSSL 3.0 REQUIRED)
find_package(Threads REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${OPENSSL_INCLUDE_DIR})

# Subdirectories
add_subdirectory(src)
add_subdirectory(tests)

# Install targets
install(DIRECTORY ${CMAKE_SOURCE_DIR}/systemd/
        DESTINATION /etc/systemd/system
        FILES_MATCHING PATTERN "*.service")

install(DIRECTORY ${CMAKE_SOURCE_DIR}/config/
        DESTINATION /etc/nnos
        FILES_MATCHING PATTERN "*.ini.example")
```

### 3.2 src/CMakeLists.txt

```cmake
# Common library (shared by all daemons)
add_library(nnos_common STATIC
    common.cpp
    config.cpp
    logging.cpp
    shared_state.cpp
    neuro_profile.cpp
    profile_matcher.cpp
)

target_link_libraries(nnos_common
    OpenSSL::SSL
    OpenSSL::Crypto
    Threads::Threads
    rt
)

# Daemon executables
add_executable(nnos_boot_daemon boot_daemon.cpp)
target_link_libraries(nnos_boot_daemon nnos_common)

add_executable(nnos_state_monitor state_monitor.cpp)
target_link_libraries(nnos_state_monitor nnos_common)

add_executable(nnos_task_manager task_manager.cpp)
target_link_libraries(nnos_task_manager nnos_common)

add_executable(nnos_context_gate context_gate.cpp)
target_link_libraries(nnos_context_gate nnos_common)

add_executable(nnos_comm_bridge comm_bridge.cpp)
target_link_libraries(nnos_comm_bridge nnos_common)

add_executable(nnos_profile_refiner profile_refiner.cpp)
target_link_libraries(nnos_profile_refiner nnos_common)

add_executable(nnos_ethernet_sync ethernet_sync.cpp)
target_link_libraries(nnos_ethernet_sync nnos_common)

# Install binaries
install(TARGETS
    nnos_boot_daemon
    nnos_state_monitor
    nnos_task_manager
    nnos_context_gate
    nnos_comm_bridge
    nnos_profile_refiner
    nnos_ethernet_sync
    DESTINATION /usr/local/bin
)
```

### 3.3 tests/CMakeLists.txt

```cmake
enable_testing()

# Test executables
add_executable(test_shared_state test_shared_state.cpp)
target_link_libraries(test_shared_state nnos_common)
add_test(NAME test_shared_state COMMAND test_shared_state)

add_executable(test_profile_matcher test_profile_matcher.cpp)
target_link_libraries(test_profile_matcher nnos_common)
add_test(NAME test_profile_matcher COMMAND test_profile_matcher)

add_executable(test_state_monitor test_state_monitor.cpp)
target_link_libraries(test_state_monitor nnos_common)
add_test(NAME test_state_monitor COMMAND test_state_monitor)

add_executable(test_task_manager test_task_manager.cpp)
target_link_libraries(test_task_manager nnos_common)
add_test(NAME test_task_manager COMMAND test_task_manager)

add_executable(test_context_gate test_context_gate.cpp)
target_link_libraries(test_context_gate nnos_common)
add_test(NAME test_context_gate COMMAND test_context_gate)

add_executable(test_comm_bridge test_comm_bridge.cpp)
target_link_libraries(test_comm_bridge nnos_common)
add_test(NAME test_comm_bridge COMMAND test_comm_bridge)

add_executable(test_ethernet_sync test_ethernet_sync.cpp)
target_link_libraries(test_ethernet_sync nnos_common)
add_test(NAME test_ethernet_sync COMMAND test_ethernet_sync)
```

### 3.4 Build Commands

**Native build (x86_64):**

```bash
cd nnos
mkdir -p build
cd build
cmake -G Ninja ..
ninja
sudo ninja install
```

**Cross-compile for Jetson (using JetPack SDK container):**

```bash
# Assumes JetPack SDK container is running and toolchain is set up
cd nnos
mkdir -p build-jetson
cd build-jetson
cmake -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/jetson-toolchain.cmake \
  ..
ninja
```

**Run tests:**

```bash
cd build
ctest --output-on-failure
```

---

## 4. Per-File Implementation Notes

### 4.1 include/nnos/common.hpp

**Purpose:** Constants, enums, utility macros.

**Contents:**

```cpp
#ifndef NNOS_COMMON_HPP
#define NNOS_COMMON_HPP

#include <cstdint>

namespace nnos {

// Magic number: "NNOSLINK" in ASCII
constexpr uint64_t NNOS_MAGIC = 0x4E4E4F534C494E4BULL;

// Version
constexpr uint32_t NNOS_VERSION = 1;

// Shared memory path
constexpr const char* NNOS_SHM_PATH = "/nnos_neural_link";

// Multicast group and port
constexpr const char* NNOS_MCAST_GROUP = "239.73.78.69";
constexpr uint16_t NNOS_MCAST_PORT = 20046;

// TCP polling port
constexpr uint16_t NNOS_TCP_PORT = 20047;

// Sync packet magic
constexpr uint32_t NNOS_SYNC_MAGIC = 0x4E4E5359U; // "NNSY"

// Intervention tiers
enum class Tier : uint8_t {
    TIER_0 = 0,
    TIER_1 = 1,
    TIER_2 = 2,
    TIER_3 = 3
};

// Archetype IDs
enum class ArchetypeID : uint8_t {
    SYSTEMS_HYPERFOCUS = 0,
    DIVERGENT_CREATIVE = 1,
    SENSORY_SOCIAL_FRAGILE = 2,
    INTENSE_MOOD_VARIANCE = 3,
    MIXED_BASELINE = 255
};

// Task demand types
enum class DemandType : uint8_t {
    DEEP_FOCUS = 0,
    CREATIVE = 1,
    ROUTINE = 2,
    SOCIAL = 3
};

// Communication decisions
enum class CommDecision : uint8_t {
    ACCEPT = 0,
    NEGOTIATE_SCOPE = 1,
    NEGOTIATE_DEADLINE = 2,
    DECLINE = 3
};

} // namespace nnos

#endif // NNOS_COMMON_HPP
```

### 4.2 include/nnos/shared_state.hpp

**Purpose:** SharedState struct definition and helper functions.

**Contents (excerpt):**

```cpp
#ifndef NNOS_SHARED_STATE_HPP
#define NNOS_SHARED_STATE_HPP

#include "common.hpp"
#include <atomic>
#include <cstdint>

namespace nnos {

struct alignas(64) SharedState {
    // Header
    uint64_t magic;
    uint32_t version;
    uint32_t reserved0;

    // Signals
    std::atomic<float> noise_level;
    std::atomic<float> light_level;
    std::atomic<uint16_t> notifications_count;
    std::atomic<float> heart_rate_variability;
    std::atomic<uint8_t> self_report_overwhelm;
    uint8_t padding0[3];

    // Profile identity
    std::atomic<uint8_t> active_profile_id;
    std::atomic<float> profile_confidence;
    uint8_t reserved_profile[3];

    // Traits (0-10)
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
    uint8_t padding2[3];

    // Timestamps (seconds since epoch)
    std::atomic<uint32_t> last_state_update;
    std::atomic<uint32_t> last_profile_update;
    std::atomic<uint32_t> last_task_event;
    std::atomic<uint32_t> boot_timestamp;

    // Reserved
    uint8_t reserved_region[1024];
};

// Helper functions
SharedState* create_shared_state();
SharedState* attach_shared_state();
bool validate_shared_state(const SharedState* state);
void destroy_shared_state(SharedState* state);

} // namespace nnos

#endif // NNOS_SHARED_STATE_HPP
```

### 4.3 include/nnos/neuro_profile.hpp

**Purpose:** Archetype trait and threshold tables.

**Contents (excerpt):**

```cpp
#ifndef NNOS_NEURO_PROFILE_HPP
#define NNOS_NEURO_PROFILE_HPP

#include "common.hpp"
#include <cstdint>

namespace nnos {

struct Traits {
    uint8_t need_for_structure;
    uint8_t novelty_seeking;
    uint8_t hyperfocus_inclination;
    uint8_t sensory_sensitivity;
    uint8_t social_energy_capacity;
    uint8_t exec_function_difficulty;
};

struct Thresholds {
    uint8_t max_concurrent_tasks;
    uint8_t max_ctx_switches_per_hour;
    uint16_t deep_work_block_min;
    uint16_t light_work_block_min;
    uint16_t min_recovery_block_min;
    uint16_t max_social_min_per_day;
    float sensory_alert_threshold;
};

struct Archetype {
    ArchetypeID id;
    const char* name;
    Traits traits;
    Thresholds thresholds;
};

// Four compiled-in archetypes
extern const Archetype ARCHETYPE_SYSTEMS_HYPERFOCUS;
extern const Archetype ARCHETYPE_DIVERGENT_CREATIVE;
extern const Archetype ARCHETYPE_SENSORY_SOCIAL_FRAGILE;
extern const Archetype ARCHETYPE_INTENSE_MOOD_VARIANCE;
extern const Archetype ARCHETYPE_MIXED_BASELINE;

extern const Archetype* ALL_ARCHETYPES[];
extern const size_t NUM_ARCHETYPES;

} // namespace nnos

#endif // NNOS_NEURO_PROFILE_HPP
```

### 4.4 include/nnos/profile_matcher.hpp

**Purpose:** Profile matching algorithm.

**Contents:**

```cpp
#ifndef NNOS_PROFILE_MATCHER_HPP
#define NNOS_PROFILE_MATCHER_HPP

#include "neuro_profile.hpp"

namespace nnos {

struct MatchResult {
    const Archetype* archetype;
    float confidence;
};

// Match observed traits to archetypes
MatchResult match_profile(const Traits& observed);

// Similarity score computation
float compute_similarity(const Traits& observed, const Traits& archetype);

} // namespace nnos

#endif // NNOS_PROFILE_MATCHER_HPP
```

### 4.5 include/nnos/logging.hpp

**Purpose:** Structured JSON logging.

**Contents:**

```cpp
#ifndef NNOS_LOGGING_HPP
#define NNOS_LOGGING_HPP

#include <string>
#include <map>

namespace nnos {

enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR,
    CRITICAL
};

class Logger {
public:
    Logger(const std::string& daemon_name, const std::string& log_path);
    ~Logger();

    void log(LogLevel level, const std::string& event,
             const std::map<std::string, std::string>& payload);

private:
    std::string daemon_name_;
    int log_fd_;
};

} // namespace nnos

#endif // NNOS_LOGGING_HPP
```

### 4.6 src/boot_daemon.cpp

**Purpose:** Main entry point for nnos_boot_daemon.

**Key steps:**

1. Parse command-line arguments (--simulate, --config).
2. Load config from `/etc/nnos/config.ini`.
3. Create and initialize SharedState.
4. Load observed traits from `/etc/nnos/observed_traits.ini` or use defaults.
5. Run profile matching.
6. Write matched profile to SharedState.
7. Fork and exec child daemons.
8. Enter supervision loop:
   - waitpid on children.
   - Restart crashed children.
   - Apply circuit breaker if too many crashes.
9. On SIGTERM:
   - Send SIGTERM to children.
   - Wait for clean exit.
   - munmap and shm_unlink.
   - Exit.

**Pseudocode:**

```cpp
int main(int argc, char** argv) {
    // Parse args
    // Load config
    // Initialize logging

    SharedState* state = create_shared_state();
    if (!state) {
        log_critical("Failed to create shared state");
        return 1;
    }

    // Profile matching
    Traits observed = load_observed_traits();
    MatchResult match = match_profile(observed);
    write_profile_to_state(state, match);

    // Spawn children
    spawn_daemon("nnos_state_monitor");
    spawn_daemon("nnos_task_manager");
    spawn_daemon("nnos_context_gate");
    spawn_daemon("nnos_comm_bridge");
    spawn_daemon("nnos_profile_refiner");
    spawn_daemon("nnos_ethernet_sync");

    // Supervision loop
    while (running) {
        pid_t pid = waitpid(-1, &status, 0);
        if (pid > 0) {
            log_child_exit(pid, status);
            if (should_restart(pid)) {
                restart_daemon(pid);
            }
        }
    }

    // Cleanup
    destroy_shared_state(state);
    return 0;
}
```

### 4.7 src/state_monitor.cpp

**Purpose:** Main for nnos_state_monitor daemon.

**Key steps:**

1. Attach to SharedState.
2. Set SCHED_FIFO priority 40 (if permitted).
3. Enter 4 Hz sample loop:
   - Read raw inputs (noise, light, notifications, HRV, self-report).
   - Compute sensory_load, emotional_load, total_load.
   - Compute new intervention_tier.
   - If tier changed:
     - store new tier (release).
     - Update last_state_update.
     - Log TIER_CHANGE.
4. On SIGTERM, exit cleanly.

**Formulas (as per SRS):**

```cpp
float sensory_load = (noise_level * sensory_sensitivity / 10.0f * 0.4f)
                   + (light_level * sensory_sensitivity / 10.0f * 0.3f)
                   + (notifications_count / 20.0f * 0.3f);

float emotional_load = ((1.0f - heart_rate_variability) * 0.5f)
                     + (self_report_overwhelm / 10.0f * 0.5f);

float total_load = (sensory_load * 0.5f) + (emotional_load * 0.5f);

Tier new_tier;
float threshold = state->sensory_alert_threshold.load(std::memory_order_acquire);
if (total_load < 0.6f * threshold) {
    new_tier = Tier::TIER_0;
} else if (total_load < 0.9f * threshold) {
    new_tier = Tier::TIER_1;
} else if (total_load < threshold) {
    new_tier = Tier::TIER_2;
} else {
    new_tier = Tier::TIER_3;
}
```

### 4.8 src/task_manager.cpp

**Purpose:** Main for nnos_task_manager daemon.

**Key steps:**

1. Attach to SharedState.
2. Listen for task intake requests (via Unix socket or stdin in --simulate mode).
3. For each request:
   - Load active_task_count, max_concurrent_tasks, intervention_tier.
   - Decide ACCEPT or DEFER.
   - On ACCEPT:
     - Increment active_task_count (fetch_add).
     - Update last_task_event.
     - Log TASK_ACCEPTED.
   - On DEFER:
     - Log TASK_DEFERRED with reason.
4. On task completion events:
   - Decrement active_task_count.
   - Log TASK_COMPLETED.

### 4.9 src/context_gate.cpp

**Purpose:** Main for nnos_context_gate daemon.

**Key steps:**

1. Attach to SharedState.
2. Track current hour boundary for ctx_switches_this_hour reset.
3. For each incoming interrupt/context switch request:
   - Check budget.
   - Allow or defer.
   - Log decision.
4. Manage hyperfocus state:
   - Read hyperfocus_active, hyperfocus_minutes, intervention_tier.
   - Apply protect/break rules per SRS FR-3.6.4 and FR-3.6.5.
   - Log HYPERFOCUS_PROTECT or HYPERFOCUS_BREAK.

### 4.10 src/comm_bridge.cpp

**Purpose:** Main for nnos_comm_bridge daemon.

**Key steps:**

1. Attach to SharedState.
2. Expose local API (HTTP or Unix socket) for incoming commitment requests.
3. For each request:
   - Parse effort_min, importance, social_intensity.
   - Compute capacity_score and demand_score using SRS FR-3.8.2 and FR-3.8.3.
   - Detect masking per FR-3.8.1.
   - Determine decision (ACCEPT, NEGOTIATE_SCOPE, NEGOTIATE_DEADLINE, DECLINE).
   - Update masking_alert if needed.
   - Log MASKING_DETECTED or COMM_DECISION.

**Formulas:**

```cpp
float capacity = (energy / 10.0f * 0.4f)
               + ((10.0f - sensory_load * 10.0f) / 10.0f * 0.3f)
               + (social_battery / 10.0f * 0.2f)
               + ((10.0f - active_task_count) / 10.0f * 0.1f);

float demand = (effort_min / 240.0f * 0.5f)
             + (importance / 10.0f * 0.3f)
             + (social_intensity / 10.0f * 0.2f);

bool masking = (capacity < 0.4f && demand > 0.7f)
            || (active_task_count >= max_concurrent_tasks && intervention_tier >= 2)
            || (social_minutes_today > max_social_min_per_day);
```

### 4.11 src/profile_refiner.cpp

**Purpose:** Main for nnos_profile_refiner daemon.

**Key steps:**

1. Attach to SharedState.
2. Wait until scheduled time (03:00 local).
3. Read logs from `/var/log/nnos/events.jsonl`.
4. Aggregate statistics over last 30 days.
5. Compare to active archetype expectations.
6. Adjust traits by ±1 if deviation exceeds threshold.
7. Re-match archetype if needed.
8. Write refined profile to SharedState and persist to `/etc/nnos/active_profile.bin`.
9. Log PROFILE_REFINED.

### 4.12 src/ethernet_sync.cpp

**Purpose:** Main for nnos_ethernet_sync daemon.

**Key steps:**

1. Attach to SharedState.
2. Load sync key from `/etc/nnos/sync.key` (32 bytes).
3. Create UDP multicast socket, join group 239.73.78.69.
4. Create TCP listening socket on port 20047.
5. Spawn two threads:
   - Transmit thread:
     - Every 100 ms, snapshot SharedState.
     - Build header + payload.
     - Encrypt with AES-256-GCM.
     - sendto multicast group.
   - Receive thread:
     - recvfrom on multicast socket.
     - Decrypt.
     - Validate magic, version, tag, sequence.
     - Merge fields per SRS FR-3.9.7.
6. Accept TCP connections:
   - Serialize SharedState to JSON.
   - Encrypt (optional).
   - Send to client.

**Packet structure (pre-encryption):**

```cpp
struct SyncHeader {
    uint32_t magic;         // 0x4E4E5359
    uint8_t version;        // 1
    uint8_t flags;
    uint16_t reserved;
    uint64_t device_id;
    uint32_t sequence;
    uint32_t timestamp;
};

struct SyncPayload {
    uint8_t intervention_tier;
    uint8_t masking_alert;
    float sensory_load;
    float emotional_load;
    float profile_confidence;
    uint8_t active_profile_id;
    uint8_t active_task_count;
    uint8_t ctx_switches_this_hour;
    uint8_t hyperfocus_active;
    uint32_t last_state_update;
    uint32_t last_profile_update;
    uint8_t reserved[8];
};
```

**Encryption:**

- Nonce: 12 bytes, derived from `device_id || sequence`.
- AAD: SyncHeader (24 bytes).
- Plaintext: SyncPayload (40 bytes).
- Output: Ciphertext (40 bytes) + Tag (16 bytes).

---

## 5. Systemd Integration

### 5.1 systemd/nnos-boot.service

```ini
[Unit]
Description=NNOS Boot Daemon
After=network.target
Requires=network.target

[Service]
Type=simple
User=nnos
Group=nnos
ExecStart=/usr/local/bin/nnos_boot_daemon --config /etc/nnos/config.ini
Restart=on-failure
RestartSec=5s

# Hardening
NoNewPrivileges=true
PrivateTmp=true
ProtectSystem=full
ProtectHome=true
ReadWritePaths=/var/log/nnos /dev/shm

[Install]
WantedBy=multi-user.target
```

### 5.2 Installation

```bash
sudo cp systemd/nnos-boot.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable nnos-boot.service
sudo systemctl start nnos-boot.service
```

### 5.3 Check Status

```bash
sudo systemctl status nnos-boot.service
sudo journalctl -u nnos-boot.service -f
```

---

## 6. Configuration Files

### 6.1 /etc/nnos/config.ini

```ini
[general]
log_path = /var/log/nnos/events.jsonl
shm_path = /nnos_neural_link

[network]
multicast_group = 239.73.78.69
multicast_port = 20046
tcp_port = 20047
sync_key_path = /etc/nnos/sync.key

[timing]
state_monitor_hz = 4
ethernet_sync_ms = 100
profile_refiner_hour = 3

[limits]
max_notifications = 20
hyperfocus_max_minutes = 120
hyperfocus_reminder_minutes = 30
```

### 6.2 /etc/nnos/observed_traits.ini

```ini
[traits]
need_for_structure = 7
novelty_seeking = 4
hyperfocus_inclination = 8
sensory_sensitivity = 6
social_energy_capacity = 3
exec_function_difficulty = 5
```

### 6.3 /etc/nnos/overrides.ini

```ini
[overrides]
# Manual overrides that profile_refiner must preserve
max_concurrent_tasks = 2
sensory_alert_threshold = 0.60
```

---

## 7. Encoding and Compliance

### 7.1 Encoding Validation Script

**scripts/bootstrap_encoding.sh:**

```bash
#!/bin/bash
set -euo pipefail

echo "Validating UTF-8 encoding for all source files..."

find . -name "*.cpp" -o -name "*.hpp" -o -name "*.md" | while read -r file; do
    if file "$file" | grep -q "UTF-8"; then
        echo "OK: $file"
    else
        echo "ERROR: $file is not UTF-8"
        exit 1
    fi
done

echo "Checking for smart quotes and em dashes..."
if grep -rn --include="*.cpp" --include="*.hpp" -P '[\x{2018}-\x{201F}\x{2013}\x{2014}]' .; then
    echo "ERROR: Found smart quotes or em dashes in source files"
    exit 1
fi

echo "All files pass encoding validation."
```

### 7.2 Sync Key Generation

**scripts/generate_sync_key.sh:**

```bash
#!/bin/bash
set -euo pipefail

KEY_PATH="/etc/nnos/sync.key"

if [ -f "$KEY_PATH" ]; then
    echo "Sync key already exists at $KEY_PATH"
    exit 1
fi

echo "Generating 256-bit AES key..."
openssl rand -out "$KEY_PATH" 32
chmod 600 "$KEY_PATH"
chown nnos:nnos "$KEY_PATH"

echo "Sync key generated: $KEY_PATH"
```

---

## 8. Testing Strategy

### 8.1 Unit Tests

Each daemon has corresponding unit tests in `tests/`:

- `test_shared_state.cpp`: Validate SharedState creation, attachment, validation.
- `test_profile_matcher.cpp`: Test matching algorithm against known trait vectors.
- `test_state_monitor.cpp`: Test tier computation formulas with synthetic inputs.
- `test_task_manager.cpp`: Test accept/defer logic under various load conditions.
- `test_context_gate.cpp`: Test context switch budgeting and hyperfocus rules.
- `test_comm_bridge.cpp`: Test capacity/demand computation and masking detection.
- `test_ethernet_sync.cpp`: Test encryption, decryption, anti-replay, merge logic.

### 8.2 Integration Tests

Scenario-based tests run the full daemon constellation:

1. Boot constellation with synthetic sensor inputs.
2. Inject escalating load to trigger tier changes.
3. Inject task requests to test task manager deferral.
4. Inject context switch requests to test budget exhaustion.
5. Inject masking scenarios to test comm bridge detection.
6. Validate logs contain expected events with correct payloads.

### 8.3 Continuous Operation Test

Run NNOS for 7 days in a test environment:

- Monitor for memory leaks (Valgrind or AddressSanitizer).
- Monitor for unhandled crashes (systemd restart logs).
- Validate SharedState consistency across devices (if multi-device).
- Ensure log rotation works correctly.

### 8.4 Acceptance Validation

Map each acceptance criterion from SRS Section 6 to a specific test:

| AC | Test |
|---|---|
| AC-6.1 | Boot time measurement on Jetson |
| AC-6.2 | Child daemon attachment validation |
| AC-6.3 | Profile matching confidence check |
| AC-6.4 | Tier escalation latency measurement |
| AC-6.5 | Ethernet sync propagation latency |
| AC-6.6 | 1000-cycle restart test |
| AC-6.7 | Masking detection precision/recall |
| AC-6.8 | 7-day continuous operation |
| AC-6.9 | Task deferral under tier 2 |
| AC-6.10 | Hyperfocus protect/break scenarios |
| AC-6.11 | Profile refinement trait adjustment |

---

## 9. Deployment Procedure

### 9.1 Prerequisites

- Target device (Jetson or NUC) running Ubuntu 22.04+ or JetPack 6.0+.
- OpenSSL 3.0+ installed.
- User `nnos` created with appropriate permissions.
- `/var/log/nnos/` directory created, owned by `nnos:nnos`.
- `/etc/nnos/` directory created for config files.

### 9.2 Deployment Steps

1. **Build binaries** on host or cross-compile for target.
2. **Transfer binaries** to target:
   ```bash
   rsync -avz build/bin/ target:/usr/local/bin/
   ```
3. **Transfer systemd unit**:
   ```bash
   scp systemd/nnos-boot.service target:/etc/systemd/system/
   ```
4. **Transfer config examples**:
   ```bash
   scp config/*.ini.example target:/etc/nnos/
   ```
5. **Customize config** on target:
   ```bash
   ssh target
   cd /etc/nnos
   cp config.ini.example config.ini
   cp observed_traits.ini.example observed_traits.ini
   # Edit as needed
   ```
6. **Generate sync key**:
   ```bash
   sudo /usr/local/bin/scripts/generate_sync_key.sh
   ```
7. **Enable and start service**:
   ```bash
   sudo systemctl daemon-reload
   sudo systemctl enable nnos-boot.service
   sudo systemctl start nnos-boot.service
   ```
8. **Validate logs**:
   ```bash
   tail -f /var/log/nnos/events.jsonl
   ```

### 9.3 Multi-Device Setup

For Jetson + NUC synchronization:

1. Deploy to both devices.
2. Generate sync key on one device.
3. Copy sync key to other device:
   ```bash
   scp /etc/nnos/sync.key other-device:/etc/nnos/
   ```
4. Ensure both devices are on same LAN subnet supporting multicast.
5. Start NNOS on both devices.
6. Validate sync logs show received packets from peer.

---

## 10. Debugging and Troubleshooting

### 10.1 Common Issues

**SharedState validation fails:**

- Check `/dev/shm/nnos_neural_link` exists and has correct permissions.
- Verify magic number and version are correct.
- Run boot daemon with elevated logging to see initialization sequence.

**Child daemon crashes immediately:**

- Check logs in `/var/log/nnos/events.jsonl`.
- Run child daemon manually with `--simulate` flag to isolate issue.
- Use `strace` to see system call failures.

**Ethernet sync not working:**

- Verify multicast routing with `ip mroute show`.
- Check firewall rules allow UDP 20046.
- Validate sync key is identical on both devices.
- Use `tcpdump` to confirm packets are being sent/received:
  ```bash
  sudo tcpdump -i eth0 -n host 239.73.78.69
  ```

**Tier changes not happening:**

- Verify state_monitor is running (check `ps aux | grep state_monitor`).
- Check sensor inputs are being updated (add debug logs).
- Validate tier computation formulas with known test inputs.

### 10.2 Debug Flags

All daemons accept:

- `--simulate`: Use synthetic inputs for testing.
- `--verbose`: Increase log verbosity to DEBUG level.
- `--config <path>`: Override default config path.

Example:

```bash
/usr/local/bin/nnos_state_monitor --simulate --verbose
```

### 10.3 Memory and Performance Profiling

**Valgrind (memory leaks):**

```bash
valgrind --leak-check=full --track-origins=yes /usr/local/bin/nnos_boot_daemon
```

**AddressSanitizer (compile-time):**

```bash
cmake -DCMAKE_CXX_FLAGS="-fsanitize=address -g" ..
ninja
./build/bin/nnos_boot_daemon
```

**perf (CPU profiling):**

```bash
sudo perf record -g /usr/local/bin/nnos_state_monitor
sudo perf report
```

---

## 11. Document Status and Maintenance

This Technical Implementation Specification (NNOS-TECH-001) is the third and final document in the NNOS specification stack:

- NNOS-SRS-001: Requirements
- NNOS-DS-001: Design
- NNOS-TECH-001: Implementation

It provides:

- Complete toolchain and build configuration.
- Per-file implementation notes for all headers and source files.
- CMake build scripts for native and cross-compilation.
- systemd integration and deployment procedures.
- Testing strategy and debugging guidance.

Engineers and AI code generation models have all information needed to implement NNOS from these three documents.

---

## 12. References

- NNOS-SRS-001: Software Requirements Specification
- NNOS-DS-001: Design Specification
- NVIDIA Jetson Orin Boot Flow: https://docs.nvidia.com/jetson/archives/r36.4.4/DeveloperGuide/AR/BootArchitecture/JetsonOrinSeriesBootFlow.html
- POSIX Shared Memory: IEEE Std 1003.1-2017
- AES-GCM: NIST SP 800-38D
- OpenSSL 3.0 Documentation: https://www.openssl.org/docs/man3.0/
- systemd Service Management: https://www.freedesktop.org/software/systemd/man/daemon.html
- CMake Documentation: https://cmake.org/documentation/
