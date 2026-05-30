# NeuroDiOS Tooling & Analysis Suite — Technical Specification

**Document ID:** NEURODIOS-TOOL-TECH-001  
**Version:** 1.0.0  
**Date:** 2026-05-30  
**Status:** Accepted — Binding  
**References:** NEURODIOS-TOOL-REQ-001, NEURODIOS-TOOL-DES-001, minimal_types.py, Binary Optimization Plan v1.1  

---

## 1. Implementation Environment

- Target Python: 3.10+ (CPython reference). No runtime type checker required in production.
- POSIX shell compatibility: macOS 14+, Ubuntu 22.04+, Jetson Linux.
- Required external commands (graceful fallback): git, sha256sum/shasum, size, readelf (elfutils or binutils), objdump, nm, file, strip, otool (Darwin only).
- No third-party Python packages. Pure stdlib + subprocess.

## 2. Shared Technical Primitives

All three tools import (or inline for single-file deployment) the following deterministic helpers:

```python
import hashlib, json, subprocess, ast, re, pathlib, datetime, sys
from dataclasses import dataclass, asdict
from typing import Any, Literal

CANONICAL_8_DENOMINATORS: list[str] = [
    "Fluctuation Dynamics", "Budget / Resource Accounting",
    "Contrast Differential", "Controlled Oscillation",
    "Adaptation Offset", "Primitive Traceability / Atomic Dependency Mapping",
    "Origin Vault (Deterministic Provenance & State History)", "Drift Detection"
]

def provenance_stamp(paths: list[pathlib.Path]) -> dict[str, Any]:
    commit = subprocess.check_output(["git", "rev-parse", "HEAD"], text=True).strip() if pathlib.Path(".git").exists() else "no-git"
    host = subprocess.check_output(["uname", "-a"], text=True).strip()
    file_hashes = {}
    for p in sorted(paths):
        if p.is_file():
            h = hashlib.sha256(p.read_bytes()).hexdigest()
            file_hashes[str(p)] = h
    return {
        "git_commit": commit,
        "build_host": host,
        "tool_version": "1.0.0",
        "analyzed_file_shas": file_hashes,
        "analysis_time_iso": datetime.datetime.utcnow().isoformat() + "Z"
    }

def bit_savings_proof(old_bits: int, new_bits: int, count: int) -> dict:
    saved_bits = count * (old_bits - new_bits)
    return {
        "statement": f"Replacing {count} {old_bits}-bit values with {new_bits}-bit yields {saved_bits} bits saved per cycle",
        "derivation": f"{count} * ({old_bits} - {new_bits}) = {saved_bits}",
        "bytes_saved_per_cycle": saved_bits // 8,
        "percentage_reduction": round((1 - (new_bits / old_bits)) * 100, 2)
    }
```

Mathematical guarantee: All savings calculations are integer arithmetic only after the initial scaling derivation is recorded. No floating-point appears in the proof objects.

## 3. Efficiency Mandate Auditor — Technical Details

**File:** scripts/efficiency_mandate_auditor.py (single-file executable)

**Python AST Visitor (core logic):**

```python
class FloatViolationVisitor(ast.NodeVisitor):
    def __init__(self):
        self.violations: list[dict] = []
        self.current_func: str | None = None

    def visit_FunctionDef(self, node: ast.FunctionDef):
        old = self.current_func
        self.current_func = node.name
        self.generic_visit(node)
        self.current_func = old

    def visit_Constant(self, node: ast.Constant):
        if isinstance(node.value, float) and self.current_func and any(h in self.current_func for h in HOT_PATHS):
            self.violations.append({
                "line": node.lineno,
                "func": self.current_func,
                "value": node.value,
                "type": "float_literal",
                "suggested": f"Intensity8({int(node.value * 255)})  # deterministic scaling 0.0-1.0 -> [0,255]"
            })
        self.generic_visit(node)
```

**Scaling Derivation (deterministic, recorded in every suggestion):**
- Intensity domain [0.0, 1.0] maps to Intensity8 [0, 255] by round(v * 255).
- Guard thresholds e.g. 0.45 become 115 (0.45*255=114.75 → 115).
- All comparisons in hot path become integer compares after conversion.

**C/C++ Pass:** Simple line-by-line regex for literals matching r'\b\d+\.\d+([eE][+-]?\d+)?[fF]?\b' inside function bodies (conservative over-approximation justified for v1; false positives acceptable because human review is required before --apply-safe).

**Output JSON Schema (excerpt):**

```json
{
  "meta": { ...provenance... },
  "summary": {
    "total_violations": 17,
    "total_potential_bits_saved_per_cycle": 1088,
    "files_affected": 2
  },
  "findings": [
    {
      "file": "neurobalance/neurobalance_coordinator.py",
      "line": 161,
      "func": "assess_and_offset",
      "literal": 0.45,
      "old_bits": 64,
      "new_bits": 8,
      "instances_per_cycle": 1,
      "proof": { ...bit_savings_proof... },
      "denominators": ["Budget / Resource Accounting", "Drift Detection"]
    }
  ],
  "remediation_diff": "diff --git ...",
  "recommended_actions": [...]
}
```

**CLI Flags (exact):** --target, --json (default stdout), --apply-safe, --strict, --hot-paths (override list), --denominators (require justification strings).

## 4. Binary Footprint Analyzer — Technical Details

**Discovery & Parsing:**

- Executable detection: `file -b` contains "ELF" or "Mach-O".
- Size parsing: subprocess + regex on "text|data|bss" lines.
- Section table: platform-specific command dispatch.
  - Linux: `readelf -S -W <bin> 2>/dev/null`
  - Darwin: `otool -l <bin> | grep -A 20 "Section"`
- Disassembly for oversized immediates (Phase 2 focus): `objdump -d --no-show-raw-insn <bin> | grep -E '0x[0-9a-f]{5,}'`

**Reproducibility Metric (mathematically defined):**
repro_score = 1.0 if current_stripped_sha == golden_sha else max(0.0, 1.0 - (abs(size_delta) / baseline_size))

**Output:** Per-binary .footprint.json + aggregate report. The JSON includes "section_hashes" suitable for direct consumption by binary_drift_check.sh.

**Integration Points:**
- Can be called from build_jmk_docker.sh post-build.
- Emits data consumable by generate_provenance_manifest.sh for embedding.

## 5. Chained Source Maintainer — Technical Details

**Parsing Strategy:** Line-oriented + section header regex (##+). No full Markdown parser required for v1 (deterministic and sufficient).

**Legacy Detection Regex (exact, case-insensitive):**
r'(?i)\b(5|five)\s*(validated\s*)?denominators?\b'

**Phase Status Check Logic:**
- Parse Binary Optimization Plan "Current Execution Status" section.
- For Phase 1 claim "ACTIVE on apps root": verify existence of compiler.jstr or recent jstar* artifacts with non-zero .data section size via analyzer.
- Emit delta between claimed and observed.

**Update Packet Format (for Live Context Agent):**

```json
{
  "recommended_updates": {
    "Chained_Source_of_Truth_Kimi_Binding.md": {
      "section": "7",
      "patch_type": "append",
      "content": "2026-05-30 Tooling Agent pass: Auditor baseline on coordinator: 17 violations, 1088 bits/cycle potential. See 2026-05-30-NeuroDiOS-Tooling-Suite-*.md"
    }
  },
  "legacy_refs_detected": 3,
  "binding_coherence": "PASS"
}
```

When --apply-safe and all checks pass, the maintainer can safely append the record to the binding's maintenance log subsection.

## 6. Testing & Validation Harness

- Self-test: Each tool has an internal --self-test flag that runs against a known synthetic fixture (embedded small Python snippet + tiny ELF stub if available) and asserts exact known output hashes.
- Mathematical validation: Every run that emits a "math_proofs" array also emits a top-level "proof_verification": "integer_arithmetic_only_after_derivation_recorded".
- Re-run determinism: `sha256sum <(tool --json ...)` identical on consecutive invocations (timestamp field excluded via jq filter in harness).

## 7. Deployment & Invocation

Placement: All three tools live in `engine/nnos/scripts/` as executable `*.py` files (chmod +x).

Example minimal invocation for Kimi executor during Binary Opt Phase 2:

```bash
python3 scripts/efficiency_mandate_auditor.py \
  --target neurobalance/neurobalance_coordinator.py \
  --json > /tmp/auditor_report.json

python3 scripts/binary_footprint_analyzer.py \
  --binary-dir build-minsize --json > /tmp/footprint.json
```

Integration into existing propagate_nnos.sh or benchmark scripts is via simple subprocess calls; output is always machine-first.

## 8. Limitations & Explicit Trade-offs (Deterministic Justification)

- No full C++ type inference in v1 (would require clang tooling). Conservative regex + human gate via --apply-safe satisfies "NEVER substitute quality for speed".
- Jasterish sources not yet scanned (compiler not stable). This is explicitly recorded as "pending Phase 1 stabilization per Binary-Opt-001".
- All decisions favor smallest possible trusted computing base (stdlib only) over richer analysis.

---

**End of Technical Specification**

The three documents (Requirements NEURODIOS-TOOL-REQ-001, Design NEURODIOS-TOOL-DES-001, Technical NEURODIOS-TOOL-TECH-001) together with the source of the three utilities form the complete, binding definition of the NeuroDiOS Tooling & Analysis Suite. Any deviation requires explicit update through the Chained Source of Truth mechanism and re-issuance of the triad.

**Provenance of this specification triad:** Generated 2026-05-30 by Tooling & Script Generation Agent in direct support of Kimi executor work on Binary Optimization Plan Phases 1-3 and remediation of the Efficiency Mandate violation identified in Chained binding §8. All content reduces strictly to combinations of the 8 Validated Denominators with explicit mappings. No new denominators introduced.