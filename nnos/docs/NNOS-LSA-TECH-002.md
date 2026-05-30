# Jason's Living System Architecture
# Tri-Plane Heterogeneous Compute Fabric (TP-HCF)
# Technical Specification

| Field | Value |
|---|---|
| Document ID | LSA-TECH-002 |
| Version | 1.0.0 |
| Date | 2026-02-22 |
| Status | DRAFT |
| Classification | CONFIDENTIAL |
| Encoding | UTF-8 without BOM |

***

## 1. Codebase Layout

Target mono-repo structure:

lsa/ 
|– CMakeLists.txt 
|– cmake/ 
|    -- toolchains/        # Optional: cross-compile toolchains |-- include/lsa/ |   |-- common.hpp |   |-- config.hpp |   |-- logging.hpp |   |-- shared_state.hpp |   |-- node_roles.hpp |   |-- origin_vault.hpp |   |-- mirrorlock.hpp |   |-- convergence_bond.hpp |   |-- drift_detector.hpp |   |-- task_manager.hpp |   |-- context_gate.hpp |   |-- comm_bridge.hpp |   |-- profile_refiner.hpp |    – ethernet_sync.hpp 
|– src/ 
|   |– boot_dcn.cpp 
|   |– boot_hcn.cpp 
|   |– boot_epn.cpp 
|   |– origin_vault_dcn.cpp 
|   |– mirrorlock_hcn.cpp 
|   |– convergence_bond_hcn.cpp 
|   |– drift_detector_dcn.cpp 
|   |– task_manager_dcn.cpp 
|   |– context_gate_dcn.cpp 
|   |– comm_bridge_hcn.cpp 
|   |– profile_refiner_hcn.cpp 
|   |– state_monitor_epn.cpp 
|   |– ethernet_sync_dcn.cpp 
|   |– ethernet_sync_hcn.cpp 
|    -- ethernet_sync_epn.cpp |-- systemd/ |   |-- lsa-boot-dcn.service |   |-- lsa-boot-hcn.service |    – lsa-boot-epn.service 
|– scripts/ 
|   |– bootstrap_encoding.sh 
|   |– install_dcn.sh 
|   |– install_hcn.sh 
|    -- install_epn.sh  – tests/ 
|– test_shared_state.cpp 
|– test_ethernet_sync.cpp 
|– test_task_manager.cpp 
|– test_context_gate.cpp 
|– test_drift_detector.cpp 
`– test_origin_vault.cpp


***

## 2. Toolchains and Dependencies

### 2.1 Common

- Build system: CMake 3.18+
- Language: C++17
- Required libraries:
  - POSIX:
    - `pthread`
    - `rt` (for shm, timers on Linux)
  - Crypto:
    - OpenSSL 3.x (`ssl`, `crypto`) for AES-256-GCM
  - DB (NUC only):
    - PostgreSQL client (`libpq`)

Compiler flags:

```cmake
add_compile_options(
    -std=c++17
    -O2
    -Wall -Wextra -Wpedantic
    -fno-exceptions
    -fno-rtti
)
```

Link flags per target (example):

```cmake
target_link_libraries(target_name
    pthread
    rt
    ssl
    crypto
    pq           # NUC-only components that talk to PostgreSQL
)
```

2.2 Per-Node
	•	NUC (DCN)
	•	OS: Ubuntu 24.04
	•	Compiler: GCC 13+
	•	Extra packages:
	•	 libpq-dev 
	•	 libssl-dev 
	•	M1 (HCN)
	•	OS: macOS 14+
	•	Compiler: Apple Clang 15+
	•	Optional:
	•	MLX / PyTorch with MPS for ML workloads (outside C++ core).
	•	Orin (EPN)
	•	OS: JetPack 6.x / Ubuntu 22.04
	•	Compiler: GCC 11+ (aarch64)
	•	Optional:
	•	CUDA, TensorRT for future state_monitor extensions.
3. Core Headers
3.1 common.hpp
Responsibilities:
	•	Fixed-width integer typedefs (or include  <cstdint> ).
	•	Constants:
	•	Magic numbers for shared memory and network packets.
	•	Multicast address, ports, protocol version.
	•	Enum types:
	•	NodeRole { DCN, HCN, EPN }.
	•	Decision enums (for tasks, comm bridge).
3.2 config.hpp
Responsibilities:
	•	Configuration struct(s) for:
	•	DB connection params (host, port, user, dbname).
	•	Multicast group and port.
	•	TCP polling port.
	•	Log file paths.
	•	Load from:
	•	Simple INI/TOML/YAML file per node ( /etc/lsa/config_dcn.ini , etc.).
	•	Provide:
	•	 Config load_config(NodeRole role); 
3.3 shared_state.hpp
Responsibilities:
	•	Define  SharedState  struct consistent with LSA-SPEC-002.
	•	Provide functions:
	•	 void init_shared_state(SharedState& s); 
	•	 bool validate_shared_state(const SharedState& s); 
	•	Use  std::atomic  for cross-process fields.
	•	Do not allocate dynamically; struct size is fixed.
3.4 node_roles.hpp
Responsibilities:
	•	Compile-time or run-time description of which daemons run on which node.
	•	Helper functions:
	•	 bool is_owner_of_field(NodeRole, FieldId); 
	•	 std::vector<std::string> daemons_for_role(NodeRole); 
4. Executables (Targets)
4.1 DCN (NUC) Targets
Binaries:
	•	 lsa_boot_dcn 
	•	 lsa_origin_vault_dcn 
	•	 lsa_drift_detector_dcn 
	•	 lsa_task_manager_dcn 
	•	 lsa_context_gate_dcn 
	•	 lsa_ethernet_sync_dcn 
Behavior:
	•	 lsa_boot_dcn :
	•	Initializes shared memory.
	•	Forks or launches the other DCN daemons.
	•	Monitors children and restarts as needed.
	•	Worker daemons:
	•	Attach to shared memory.
	•	Implement their domain logic as per Functional Requirements.
4.2 HCN (M1) Targets
Binaries:
	•	 lsa_boot_hcn 
	•	 lsa_mirrorlock_hcn 
	•	 lsa_convergence_bond_hcn 
	•	 lsa_profile_refiner_hcn 
	•	 lsa_comm_bridge_hcn 
	•	 lsa_ethernet_sync_hcn 
4.3 EPN (Orin) Targets
Binaries:
	•	 lsa_boot_epn 
	•	 lsa_state_monitor_epn 
	•	 lsa_ethernet_sync_epn 
5. Build Configuration (CMake)
Minimal top-level  CMakeLists.txt  skeleton:

```cmake
cmake_minimum_required(VERSION 3.18)
project(lsa_tphcf LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(
    -O2
    -Wall -Wextra -Wpedantic
    -fno-exceptions
    -fno-rtti
)

include_directories(${CMAKE_SOURCE_DIR}/include)

# Example: Shared library or object library for common code
add_library(lsa_common
    src/common_logging.cpp
    # optional: src/shared_state_utils.cpp
)
target_link_libraries(lsa_common pthread)

# DCN executables (example)
add_executable(lsa_boot_dcn src/boot_dcn.cpp)
target_link_libraries(lsa_boot_dcn lsa_common rt ssl crypto pq pthread)

# Add other executables similarly (HCN/EPN)
```

For cross-compiling to Orin from another machine, define a toolchain file under  cmake/toolchains/  and invoke CMake with  -DCMAKE_TOOLCHAIN_FILE=... .
6. Runtime Configuration
6.1 Config Files
Per-node config example:  /etc/lsa/config_dcn.ini 

```ini
[node]
role = DCN

[network]
multicast_group = 239.73.78.69
multicast_port = 20046
tcp_poll_port = 20047

[db]
host = localhost
port = 5432
user = lsa
dbname = lsa_core

[logging]
events_path = /var/log/lsa/events_dcn.jsonl
```

HCN and EPN configs mirror this with appropriate role and paths.
6.2 Secrets
	•	 /etc/lsa/sync.key  (32 bytes, AES-256 key)
	•	Owner: root
	•	Mode: 0600
	•	DB user passwords:
	•	Stored via OS keychain or  .pgpass  with restrictive permissions, not in config file.
7. Service Wiring (systemd)
Example:  /etc/systemd/system/lsa-boot-dcn.service 

```ini
[Unit]
Description=LSA Boot Daemon (DCN)
After=network.target postgresql.service

[Service]
Type=simple
ExecStart=/usr/local/bin/lsa_boot_dcn
Restart=always
RestartSec=5
User=lsa
Group=lsa

[Install]
WantedBy=multi-user.target
```

HCN/EPN boot services are analogous, with different ExecStart binaries.
Enable and start:

```bash
sudo systemctl daemon-reload
sudo systemctl enable lsa-boot-dcn.service
sudo systemctl start lsa-boot-dcn.service
```

8. Logging and Monitoring
	•	All daemons:
	•	Write JSON lines to:
	•	DCN:  /var/log/lsa/events_dcn.jsonl 
	•	HCN:  /var/log/lsa/events_hcn.jsonl 
	•	EPN:  /var/log/lsa/events_epn.jsonl 
	•	Rotate logs using  logrotate  (size or time-based rotation).
	•	For quick tail:

```bash
sudo journalctl -u lsa-boot-dcn -f
tail -f /var/log/lsa/events_dcn.jsonl
```

9. Test Harness
9.1 Unit Tests
Built with CTest or another framework:
	•	 test_shared_state.cpp 
	•	Validates init/validate behavior.
	•	 test_ethernet_sync.cpp 
	•	Simulates two nodes, verifies:
	•	Encryption.
	•	Anti-replay.
	•	Merge policies.
	•	 test_task_manager.cpp ,  test_context_gate.cpp , etc.
Run:

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)
ctest
```

9.2 Fabric Simulation
Small CLI tool (optional, can be AI-generated later):
	•	Spins up “mock” DCN, HCN, EPN processes locally, sharing a test config.
	•	Feeds synthetic:
	•	Price streams.
	•	Neuro state signals.
	•	Task requests.
	•	Verifies end-to-end behavior against expected outputs.
10. AI Coder Usage Instructions
When using Claude/Qwen/DeepSeek to implement this tech spec:
	1.	Provide both:
	•	LSA-SPEC-002 (requirements + design).
	•	LSA-TECH-002 (this document).
	2.	Ask it to:
	•	Implement headers in Section 3 first.
	•	Then executables in Section 4, starting with  lsa_boot_dcn .
	3.	After each file:
	•	Compile locally.
	•	If errors, paste compiler output back to the model for fixes.
	4.	Once core paths compile and basic tests pass:
	•	Implement remaining daemons and test harness.

---

## 11. Per-File Implementation Notes

### 11.1 include/lsa/common.hpp

Content:

- Includes:
  - `<cstdint>`, `<string>`, `<string_view>`.
- Namespace: `lsa`.
- Constants:
  - `constexpr uint64_t SHM_MAGIC = 0x4C53414C494E4B31;` // "LSALINK1"
  - `constexpr uint32_t SHM_VERSION = 1;`
  - `constexpr uint32_t NET_MAGIC = 0x4E4E5359;`         // "NNsy"
  - `constexpr uint8_t NET_VERSION = 1;`
  - `constexpr char MULTICAST_GROUP[] = "239.73.78.69";`
  - `constexpr uint16_t MULTICAST_PORT = 20046;`
  - `constexpr uint16_t TCP_POLL_PORT = 20047;`
- Enums:
  - `enum class NodeRole { DCN, HCN, EPN };`
  - `enum class TaskDecision { ACCEPT, DEFER };`
  - `enum class CommDecision { ACCEPT, NEGOTIATE_SCOPE, NEGOTIATE_DEADLINE, DECLINE };`
- Helper:
  - `NodeRole detect_node_role_from_env();` (optional).

### 11.2 include/lsa/logging.hpp

Content:

- Thin wrapper for JSON line logging:
  - `void log_event(const std::string& daemon, const std::string& level, const std::string& event, const std::string& payload_json);`
- Implementation detail:
  - Uses `std::fprintf` to `stdout` or a file path from config.
  - Payload is already JSON; function wraps timestamp, daemon, level, event.

### 11.3 include/lsa/config.hpp

Content:

- `struct Config`:
  - `NodeRole role;`
  - Network:
    - `std::string multicast_group;`
    - `uint16_t multicast_port;`
    - `uint16_t tcp_poll_port;`
  - DB:
    - `std::string db_host;`
    - `uint16_t db_port;`
    - `std::string db_user;`
    - `std::string db_name;`
  - Logging:
    - `std::string events_path;`
- Function:
  - `Config load_config(NodeRole role, const std::string& path);`
- Implementation:
  - Simple INI parser or minimal custom parser (key=value per line).

### 11.4 include/lsa/shared_state.hpp

Content:

- `struct SharedState`:
  - Fields as defined in LSA-SPEC-002 (control-plane subset).
- Functions:
  - `void init_shared_state(SharedState& s);`
    - Zero memory.
    - Set magic, version, boot timestamp.
  - `bool validate_shared_state(const SharedState& s);`
    - Magic == SHM_MAGIC, version == SHM_VERSION.

### 11.5 include/lsa/node_roles.hpp

Content:

- Functions:
  - `std::vector<std::string> daemons_for_role(NodeRole role);`
  - `bool owns_sensory_fields(NodeRole role);`
  - `bool owns_profile_fields(NodeRole role);`
  - `bool owns_governance_fields(NodeRole role);`
- Use to guide Ethernet sync merge decisions.

---

### 11.6 src/boot_dcn.cpp

Responsibilities:

- Parse config for DCN.
- Create and initialize shared memory:
  - `shm_open`, `ftruncate`, `mmap`.
  - Call `init_shared_state` and write SHM_MAGIC, SHM_VERSION.
- Spawn child daemons:
  - `lsa_origin_vault_dcn`
  - `lsa_drift_detector_dcn`
  - `lsa_task_manager_dcn`
  - `lsa_context_gate_dcn`
  - `lsa_ethernet_sync_dcn`
- Monitor children:
  - `waitpid` loop, restart on crash.
- Handle SIGTERM:
  - Send SIGTERM to children.
  - Wait with timeout.
  - Clean up shm (munmap, shm_unlink).

### 11.7 src/boot_hcn.cpp and src/boot_epn.cpp

Same pattern as DCN, but spawn the node-specific daemons:

- HCN:
  - `lsa_mirrorlock_hcn`
  - `lsa_convergence_bond_hcn`
  - `lsa_profile_refiner_hcn`
  - `lsa_comm_bridge_hcn`
  - `lsa_ethernet_sync_hcn`
- EPN:
  - `lsa_state_monitor_epn`
  - `lsa_ethernet_sync_epn`

---

### 11.8 src/origin_vault_dcn.cpp

Responsibilities:

- Connect to PostgreSQL using libpq:
  - Connection string built from Config.
- Expose internal functions (could be future service wrapper):
  - `create_rule`, `update_rule`, `get_rule`, `list_rules`, `log_event`.
- Implement FR-6.1.x:
  - Store rules with tiers and confidence.
  - Support “break glass” retrieval sorted by tier then confidence.
- For now, can be a daemon that:
  - On startup, tests DB connectivity.
  - Optionally exposes a simple local API (Unix socket or CLI) for rule management.

### 11.9 src/drift_detector_dcn.cpp

Responsibilities:

- Periodic loop (e.g., every minute):
  - Query recent returns from DB or a log-derived table.
  - Compute drift metric.
  - If drift > 0.618:
    - Raise alert in logs.
    - Update drift-related fields in SharedState.
- Use libpq queries defined in Section 15.2.

---

### 11.10 src/task_manager_dcn.cpp

Responsibilities:

- Attach to shared memory.
- Provide a local interface for task intake:
  - Could be:
    - HTTP/Unix-socket API, or
    - Simple CLI for now.
- Implement FR-6.6.x:
  - Compute breathing room.
  - Enforce max_concurrent_tasks.
  - Defer tasks when tier or breathing room constraints violated.
- Update SharedState:
  - `active_task_count`
  - `last_task_event`

---

### 11.11 src/context_gate_dcn.cpp

Responsibilities:

- Periodic loop (e.g., every minute):
  - Read `ctx_switches_this_hour`, `intervention_tier`, `hyperfocus_*` from SharedState.
  - Apply rules from FR-6.7.x.
  - Optionally write:
    - A “gate state” field (manual-only mode, etc.) into SharedState.
- Log decisions:
  - When hyperfocus is protected or broken.
  - When manual-only mode is activated.

---

### 11.12 src/mirrorlock_hcn.cpp

Responsibilities:

- Periodic batch job (or triggered):
  - Read `trade_log` from DB on NUC (or replicated/log-fed data).
  - Compute mismatches between stated and actual indicators.
  - Classify profitable mismatches as IMPLICIT KNOWLEDGE.
- Outputs:
  - Logs and/or DB records summarizing mismatches.
  - Optional: update metrics stored in SharedState (e.g., “implicit_knowledge_score”).

### 11.13 src/convergence_bond_hcn.cpp

Responsibilities:

- Batch job:
  - Compute cosine similarity between domain vectors.
  - Mark overlap < 0.382 as optimal complementarity, with leverage 2193x.
- Output:
  - Logs + DB record summarizing domain overlaps.
  - Optionally update SharedState with a “complementarity_index”.

---

### 11.14 src/profile_refiner_hcn.cpp

Responsibilities:

- Daily job at 03:00:
  - Load 30-day logs.
  - Run embedding/ML pipeline (possibly via external Python tool).
  - Compute adjusted traits and profile_id.
- Writes:
  - Updated profile fields into SharedState.
  - Persists to disk per LSA-SPEC-002 (e.g., `/etc/lsa/active_profile.bin`).
- Uses merge rules:
  - HCN is owner of profile fields.

---

### 11.15 src/comm_bridge_hcn.cpp

Responsibilities:

- Expose local API (HTTP or Unix socket) for communication decision requests.
- Implement FR-6.9.x:
  - Compute capacity and demand.
  - Decide ACCEPT/NEGOTIATE_SCOPE/NEGOTIATE_DEADLINE/DECLINE.
- Update SharedState:
  - `masking_flag` when masking detected.

---

### 11.16 src/state_monitor_epn.cpp

Responsibilities:

- 4 Hz loop:
  - Sample or simulate:
    - noise, light, notifications, HRV proxy, self-report.
  - Compute sensory_load and emotional_load.
  - Determine intervention_tier.
- Writes to SharedState (EPN-owned fields):
  - `sensory_load`
  - `intervention_tier`
  - `last_state_update`

---

### 11.17 src/ethernet_sync_*.cpp

Common responsibilities across nodes:

- Open UDP socket for multicast send/receive.
- Periodic send of local-owned fields:
  - Build header and payload as per Section 17.
  - Encrypt using AES-256-GCM.
- Receive loop:
  - Decrypt incoming packets.
  - Validate magic, version, tag.
  - Check anti-replay via per-device sequence map.
  - Merge fields according to owner and timestamps.

DCN variant:
- Additionally:
  - Serve TCP polling endpoint on `TCP_POLL_PORT`.
  - On connection:
    - Read request (or just accept).
    - Send JSON snapshot of current SharedState.

---

## 12. Implementation Notes for AI Coders

When instructing an AI coder:

- For each header/source pair, reference:
  - LSA-SPEC-002 section numbers.
  - LSA-TECH-002 sections 3 and 11.
- Always specify:
  - NodeRole context (DCN/HCN/EPN).
  - Ownership rules for fields.
- Keep code:
  - Single-threaded per daemon.
  - No dynamic allocation after startup in hot loops.
  - Straight C++17 with minimal dependencies.

---

## 13. Environment and Deployment Matrix

### 13.1 Per-Node Environment Baselines

| Node | OS | Minimal Spec | Packages (apt/brew) |
|------|----|--------------|---------------------|
| DCN (NUC) | Ubuntu 24.04 | 16 GB RAM, SSD | build-essential, cmake, libssl-dev, libpq-dev, git |
| HCN (M1) | macOS 14+ | 16 GB RAM | Xcode CLT, cmake, openssl (brew), git |
| EPN (Orin) | JetPack 6.x | 8 GB RAM | build-essential, cmake, libssl-dev, git |

Each node SHALL have:
- Dedicated `lsa` system user and group.
- `/etc/lsa/` directory for configs and keys.
- `/var/log/lsa/` directory for logs (owned by `lsa`).

### 13.2 Deployment Steps (Per Node)

Standardized procedure (idempotent):

1. Fetch code:
   ```bash
   git clone https://your.git/lsa.git
   cd lsa
   ```

2.	Normalize encodings:

```bash
./scripts/bootstrap_encoding.sh .
```

3. Build:

```bash
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
sudo make install
```

4.	Install configs:
	•	Place node-specific config in  /etc/lsa/config_<role>.ini .
	•	Place  sync.key  in  /etc/lsa/sync.key  with  chmod 600 .

5.	Enable services:

```bash
sudo systemctl daemon-reload
sudo systemctl enable lsa-boot-<role>.service
sudo systemctl start lsa-boot-<role>.service
```

14. Operational Runbook
14.1 Daily Checks
DCN (NUC):
	•	Verify boot daemon status:

```bash
sudo systemctl status lsa-boot-dcn
```

	•	Tail logs:

```bash
tail -n 100 /var/log/lsa/events_dcn.jsonl
```

HCN (M1):
	•	Confirm nightly jobs executed:
	•	Profile refinement logs present for 03:00 window.
EPN (Orin):
	•	Confirm state_monitor:

```bash
sudo systemctl status lsa-boot-epn
tail -n 50 /var/log/lsa/events_epn.jsonl
```

14.2 Common Incident Procedures
Symptom: DCN boot daemon failed.
	•	Action:

```bash
sudo journalctl -u lsa-boot-dcn -n 200
sudo systemctl restart lsa-boot-dcn
```

	•	If shared memory corrupted:
	•	Stop service, manually remove SHM object (document exact name).
	•	Restart service to recreate.
Symptom: Nodes not syncing.
	•	Action:
	•	Check multicast reachability:

```bash
sudo tcpdump -i <iface> host 239.73.78.69 and port 20046
```

	•	Check  lsa_ethernet_sync_*  logs on all nodes.
Symptom: Drift alerts firing continuously.
	•	Action:
	•	Dump  drift_windows  from DB.
	•	Verify input returns are sane.
	•	Potentially adjust thresholds via config (with change control).
Symptom: Jason overload without interventions.
	•	Action:
	•	Inspect state from DCN:

```bash
# Example JSON snapshot from TCP poll endpoint
curl http://dcn:20047/state
```

	•	Verify  intervention_tier ,  sensory_load ,  masking_flag .
15. Change Management and Versioning
15.1 Versioning Rules
	•	Specs:
	•	LSA-SPEC-002 and LSA-TECH-002 use semantic versions (MAJOR.MINOR.PATCH).
	•	MAJOR: breaking changes to node roles, workload routing, or state structures.
	•	MINOR: new features, new daemons, new fields.
	•	PATCH: clarifications, bugfixes in text.
	•	Code:
	•	Repo tagged with matching version (e.g.,  v2.0.0-lsa ).
	•	Binaries embed spec version via  --version  flag.
15.2 Compatibility Rules
	•	SharedState:
	•	Versioned with  SHM_VERSION .
	•	Any change to struct layout increments SHM_VERSION.
	•	Mixed-version nodes are not supported unless explicitly handled (future extension).
	•	Wire Protocol:
	•	 NET_VERSION  increments for packet format changes.
	•	Older nodes drop packets with unknown version.
15.3 Change Approval
	•	Any change to:
	•	Workload routing (E1/E2/E3 mapping),
	•	Ownership rules (who writes which fields),
	•	Governance rules (Origin Vault, Drift thresholds), MUST be:
	•	Documented in LSA-SPEC-002 change log.
	•	Reviewed against:
	•	Breathing room law.
	•	“AI never executes trades” constraint.

```markdown
---

## 16. Security Hardening Checklist

### 16.1 Network Surface

- DCN (NUC):
  - Exposed only on LAN/VPN.
  - Allowed inbound:
    - TCP 20047 (Pixel client / internal tools).
  - Blocked inbound from Internet:
    - Use host firewall (ufw/iptables) to restrict to trusted subnets.

- HCN (M1) and EPN (Orin):
  - No Internet-facing services.
  - Only multicast UDP 239.73.78.69:20046 and any local admin ports.
  - Use host firewall to block unsolicited inbound TCP.

Checklist:

- [ ] Configure ufw/iptables on NUC:
  - Allow: SSH from admin IPs.
  - Allow: TCP 20047 from trusted LAN.
  - Deny: All other inbound by default.
- [ ] On Orin and M1, block inbound except SSH and any dev-only ports.

### 16.2 OS Users and Permissions

- Create dedicated user/group:

  ```bash
  sudo useradd -r -s /usr/sbin/nologin lsa
  sudo mkdir -p /etc/lsa /var/log/lsa
  sudo chown -R lsa:lsa /etc/lsa /var/log/lsa
  ```

- Run all LSA daemons as `lsa` user, not root.
- Ensure:

  - `/etc/lsa/sync.key`:
    - Owner: `root` or `lsa`.
    - Mode: `600`.
  - Config files in `/etc/lsa/`:
    - Mode: `640` or stricter.
  - Logs in `/var/log/lsa/`:
    - Owned by `lsa`, not world-readable if sensitive.

Checklist:

- [ ] `lsa` user exists, no login shell.
- [ ] All systemd units use `User=lsa` and `Group=lsa`.
- [ ] Key and config file permissions verified with `ls -l /etc/lsa`.

### 16.3 Database Security (NUC)

- PostgreSQL:
  - Bind to localhost or private LAN only.
  - Use strong password for `lsa` DB user.
  - Restrict `pg_hba.conf` to trusted hosts.

Checklist:

- [ ] `listen_addresses = 'localhost,<LAN_IP>'` in `postgresql.conf`.
- [ ] `pg_hba.conf` only allows `lsa` from NUC + HCN (if needed).
- [ ] No superuser rights for `lsa` DB user; minimum privileges only.

### 16.4 Secrets Management

- Secrets stored only in:
  - `/etc/lsa/sync.key`.
  - DB credentials (e.g., `.pgpass` with `600` permissions).
- NEVER:
  - Commit secrets to Git.
  - Put secrets in logs.
  - Put secrets in world-readable files.

Checklist:

- [ ] Grep repo for obvious secrets before pushing:
  ```bash
  git grep -i 'password\|secret\|api_key'
  ```
- [ ] `.gitignore` includes local config overrides.

### 16.5 Process and Resource Limits

- Use `systemd` to set:
  - Memory limits.
  - Open file limits.
  - No core dumps in production.

Example snippet in `lsa-boot-dcn.service`:

```ini
[Service]
# Existing entries...
MemoryMax=2G
LimitNOFILE=65536
LimitCORE=0
NoNewPrivileges=true
PrivateTmp=true
ProtectSystem=full
ProtectHome=true
```

Checklist:

- [ ] Resource limits configured to prevent runaway processes.
- [ ] Services do not run with unnecessary privileges.

### 16.6 Logging and PII

- Logs contain:
  - System state, tiers, drift, decisions.
- Logs SHALL NOT contain:
  - Exchange API keys.
  - Full account numbers.
  - Personally identifying information beyond what Jason explicitly accepts.

Checklist:

- [ ] Logging code redacts sensitive fields (if any).
- [ ] Regular review of `/var/log/lsa/*.jsonl` for accidental leakage.

---

## 17. Summary

With:

- LSA-SPEC-002 (Requirements + Design),
- LSA-TECH-002 (Technical Implementation + Ops + Security),

you have an enterprise-class specification set that:

- Defines what TP-HCF must do,
- Specifies exactly how to build and deploy it on each node,
- Provides guidance for operations and incident response,
- And sets clear security and change-management expectations.

