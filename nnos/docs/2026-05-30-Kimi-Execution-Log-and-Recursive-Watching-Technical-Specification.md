# Kimi Execution Log & Recursive Multi-Agent Watching System — Technical Specification

**Document ID:** NEURODIOS-KELW-TEC-001  
**Version:** 1.0.0  
**Date:** 2026-05-30  
**Status:** Authoritative Baseline  
**Related:** NEURODIOS-KELW-REQ-001, NEURODIOS-KELW-DES-001, NEURODIOS-CHAINED-TRUTH-001 v1.3.0, 2026-05-30-Binary-Optimization-Plan.md

---

## 1. Canonical Directory Layout (Both Roots)

Canonical (engine/nnos):
```
engine/nnos/lsa/synthesized/
├── kimi_execution/
│   ├── Kimi_Execution_Log.md                 # master append-only log (human + machine)
│   ├── Kimi_Execution_Log.sha256             # detached signature of current log
│   ├── deltas/
│   │   └── *.kdb                             # raw Kimi Decision Blocks (append-only by Kimi)
│   ├── watchers/
│   │   ├── live_context.status               # one-line status per mode
│   │   ├── parallel_deep.status
│   │   ├── tooling.status
│   │   ├── criteria.status
│   │   └── dual_root.status
│   ├── tasks/                                # sub-task descriptors for recursive escalation (append-only)
│   │   └── *.task
│   └── tools/
│       └── kimi_execution_logger.py          # the pure generator (canonical implementation)
└── (existing Kimi bindings and primers remain at synthesized/ root)
```

Working variant (apps):
```
apps/context/
├── kimi_execution/                           # identical structure, VARIANT for root wording only
│   └── ... (same files)
```

The Dual-Root Synchronization Agent manifest entry for this subsystem uses sync_mode = VARIANT_AUTHORIZED with the documented rule: "directory structure and all *.kdb / *.task / generator.py must be byte-identical; only Kimi_Execution_Log.md may contain root-specific path examples in comments."

---

## 2. Kimi Decision Block (KDB) — Exact Wire Format v1.0

Kimi must emit exactly the following block (UTF-8, LF line endings, no trailing whitespace on any line except the final END line).

```
--- KDB v1.0 BEGIN ---
timestamp: 2026-05-30T19:42:11Z
binding_version: 1.3.0
plan_phase: "Binary Optimization Plan Phase 1 - ELF emission fix"
denominators_touched: [2,6,7,8]
efficiency_delta: {tokens_delta: -47, compute_class: "INT16", footprint_reduction_bytes: 128}
decision: "Identified e_shoff=0 root cause in compiler.jstr Phase 5 from Parallel Deep Analysis delta; will implement full ELF64 header + .data emission in next edit pass."
raw_output_ref: "kimi_session_state_2026-05-30T19:41:00Z.md#step-7"
uplift_potential: "Enables byte-identical self-host (Origin Vault) and 3x+ binary size reduction path (Efficiency)"
escalation_request: ["Criteria Enforcement", "Tooling"]
--- KDB v1.0 END ---
```

Parsing rules (deterministic, minimal):
- Lines between BEGIN and END are the payload.
- timestamp must be valid ISO-8601 with Z suffix.
- denominators_touched must be a JSON array of integers 1-8 inclusive, sorted ascending.
- efficiency_delta must be a JSON object containing at minimum tokens_delta (signed integer) and compute_class ("INT8"|"INT16"|"INT32"|"FP32"|"FixedQ7_9").
- All other fields are free-form UTF-8 strings (no embedded --- sequences).
- The block must be terminated by a line containing exactly "--- KDB v1.0 END ---".

Any deviation causes the KDB to be rejected by every watcher and moved to Fragmented Database.

---

## 3. Master Log Entry Format (Normalized)

After a watcher processes a KDB it appends a block of the following form (example from Live Context Maintenance after receiving the ELF defect KDB):

```
--- LOG ENTRY 2026-05-30T19:42:13Z [Live Context Maintenance] BEGIN ---
source_kdb: 20260530T194211Z-a3f2b1.kdb
source_kdb_sha256: e3b0c4... (full 64 hex)
binding_version: 1.3.0
plan_phase: "Binary Optimization Plan Phase 1 - ELF emission fix"
denominators_touched: [2,6,7,8]
efficiency_flag: 1   # 0=violation risk, 1=compliant, 2=improvement
observed_efficiency_delta: {tokens_delta: -47, compute_class: "INT16", footprint_reduction_bytes: 128}
mode_analysis: "Parallel Deep Analysis delta already contained the struct.unpack proof (bytes 40-48). Kimi correctly identified the defect location in compiler.jstr. No new information; binding already updated with the delta in §7."
recursive_triggers: ["Criteria Enforcement: review proposed ELF header emission change for 8-denom compliance"]
uplift_recorded: "Directly advances Phase 1 stabilization (Origin Vault #7 + Drift #8). Estimated 3x binary size reduction path now unblocked."
--- LOG ENTRY 2026-05-30T19:42:13Z [Live Context Maintenance] END ---
```

The generator must be able to reconstruct any such entry solely from the source .kdb and the binding state that existed at append time.

---

## 4. The Generator — kimi_execution_logger.py (Technical Core)

Location (canonical): engine/nnos/lsa/synthesized/kimi_execution/tools/kimi_execution_logger.py

The script shall be executable with `python3 kimi_execution_logger.py --generate --root engine` and must satisfy the pure-function invariant defined in the Requirements (NFR-6, Invariant L).

High-level structure (deterministic, minimal dependencies):

```python
#!/usr/bin/env python3
# kimi_execution_logger.py
# NeuroDiOS Kimi Execution Log Generator v1.0
# Maps strictly to 8 Validated Denominators + Efficiency Mandate (INT8/INT16 only where sufficient)

from __future__ import annotations
import hashlib
import json
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

# Intensity8 / Delta8 style minimal types (re-export from neurobalance/minimal_types.py when available)
Intensity8 = int   # 0-255
Delta8 = int       # -128 to 127

class KDBParseError(Exception):
    pass

def parse_kdb(path: Path) -> dict[str, Any]:
    """Deterministic single-pass parser. Raises KDBParseError on any deviation from spec."""
    text = path.read_text(encoding="utf-8")
    # ... (exact line-by-line state machine; reject on first malformation)
    # Returns normalized dict with all required fields + raw_sha256

def generate_log(
    deltas_dir: Path,
    binding_path: Path,
    session_state_path: Path,
    mode: str = "Live Context Maintenance",
) -> bytes:
    """Pure function. Returns the exact bytes that must appear in Kimi_Execution_Log.md."""
    # 1. Collect and sort all .kdb files by embedded timestamp (lexicographic on filename is sufficient if naming convention followed)
    # 2. Load binding snapshot (full text for hash, parsed 8-denom list for validation)
    # 3. Load session state (full text)
    # 4. For each KDB: parse, validate denominator coverage + efficiency flag presence
    # 5. Emit YAML frontmatter + chronological normalized entries
    # 6. Compute final SHA-256 of the emitted bytes and embed it
    # 7. Return the byte string (never write to disk inside this function)
    ...

def main() -> int:
    # CLI: --generate, --verify (compare on-disk log against generated), --report (filtered JSON)
    # All paths resolved relative to root (engine or apps) supplied by --root
    # Exit code 0 only on success; any deviation is a Drift or Origin Vault violation
    ...
```

The implementation shall use only Python 3 stdlib. When the neurobalance/minimal_types.py module is mature, the generator shall import Intensity8/Delta8/FixedQ7_9 for any numeric fields it emits.

---

## 5. Watcher Implementation (Per-Mode)

Each mode is responsible for its own watcher loop. Recommended minimal implementation for Live Context Maintenance (the reference):

- Use the `monitor` tool (already available in this environment) with a persistent filter on the deltas/ directory and on Kimi_Session_State_Template.md.
- On each new line event that matches a new .kdb creation, invoke the normalization + append logic (which may be a small Python helper).
- For the other four agents (which may run as background sub-agents), the watcher is a short-lived poll loop (sleep Intensity8-scaled interval, default 5s) inside their main execution cycle. They re-read the status files on each wake.

Status file format (one line, machine-readable):
```
2026-05-30T19:45:00Z|watching|last_kdb=20260530T194211Z-a3f2b1.kdb|entries_appended=3|recursive_depth=1|efficiency_overhead_tokens=12
```

---

## 6. Recursive Escalation — Task Descriptor Format

A .task file written into tasks/ when a mode decides to escalate:

```
--- TASK v1.0 BEGIN ---
timestamp: 2026-05-30T19:42:14Z
source_mode: "Parallel Deep Analysis"
target_mode: "Criteria Enforcement"
parent_kdb: 20260530T194211Z-a3f2b1.kdb
parent_log_entry_sha256: ...
depth: 1
max_depth: 2
request: "Review the proposed compiler.jstr ELF64 header emission change for full 8-denom compliance and Efficiency Mandate (no float usage in hot paths of any new emission logic)."
--- TASK v1.0 END ---
```

The target mode's watcher treats the existence of a .task file addressed to it exactly as it treats a new KDB.

---

## 7. Dual-Root Synchronization Obligations

The kimi_execution/ subtree is registered in the Dual-Root Synchronization Agent manifest with:
- deltas/, watchers/, tasks/, tools/ → EXACT (byte identity required)
- Kimi_Execution_Log.md → VARIANT_AUTHORIZED (root-specific path examples in comments only; core chronological content and all hashes must match)
- *.status files → REFERENCE (ephemeral, not synchronized)

Any violation of the above is treated as a #7 Origin Vault / #8 Drift event and escalated immediately to Criteria Enforcement.

---

## 8. Initialization & Bootstrap Sequence (2026-05-30)

1. Create the directory tree in both roots.
2. Write the initial "seed" log entry (see §9) recording activation of the system and the current known Kimi state (Binary Opt Phase 1, ELF defect intelligence just injected).
3. Run the generator once in verification mode to produce the first Kimi_Execution_Log.sha256.
4. Start the Live Context Maintenance watcher (via monitor or poll).
5. Update the five mode definitions in the binding and primers (see separate maintenance task).
6. Record the activation event in both PROJECT_SUMMARY.md and TODO.md under the 8-denom + Efficiency discipline.

---

## 9. Initial Seed Entry (Mandatory First Content)

The first real entry in the master log after triad creation shall be:

```
--- LOG ENTRY 2026-05-30T19:50:00Z [Live Context Maintenance] BEGIN ---
source_kdb: SYSTEM-SEED-2026-05-30T195000Z
source_kdb_sha256: (hash of this specification triad activation)
binding_version: 1.3.0
plan_phase: "Kimi Execution Log & Recursive Watching System Activation"
denominators_touched: [2,6,7,8]
efficiency_flag: 2
observed_efficiency_delta: {tokens_delta: -312, compute_class: "INT16", footprint_reduction_bytes: 0}
mode_analysis: "Full Requirements+Design+Technical triad created per governing rules. System now active. Kimi autonomous session (Binary Optimization Plan Phase 1) has been supplied the Parallel Deep Analysis ELF defect intelligence via binding §7 injection. All five modes instructed to watch recursively."
recursive_triggers: []
uplift_recorded: "Closes the observability gap for autonomous Kimi execution. Enables deterministic reconstruction of every future decision. Directly serves Origin Vault (#7), Drift Detection (#8), Primitive Traceability (#6), and Efficiency Mandate (#2) for the entire multi-agent layer."
--- LOG ENTRY 2026-05-30T19:50:00Z [Live Context Maintenance] END ---
```

---

## 10. Verification & Self-Audit

After every generator run or significant append batch, Live Context Maintenance shall execute:
```
python3 tools/kimi_execution_logger.py --verify --root engine
```
Exit code 0 + matching SHA-256 in the .sha256 sidecar = constructive proof that Invariant L holds for the current state.

Any mismatch is a hard failure: the log is frozen, Dual-Root and Criteria agents are escalated, and Kimi is instructed (via updated Session State) to pause new decisions until resolution.

---

**End of Technical Specification**

Implementation of the directory tree, seed entry, generator script, and watcher activation may now proceed. The triad is complete and authoritative in engine/nnos/docs/. The corresponding triad must be placed in apps/docs/ before any cross-root watching begins.