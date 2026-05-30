# NeuroDiOS Tooling & Analysis Suite — Requirements

**Document ID:** NEURODIOS-TOOL-REQ-001  
**Version:** 1.0.0  
**Date:** 2026-05-30  
**Status:** Accepted — Binding for Tooling & Script Generation Agent  
**Authoring Agent:** Tooling & Script Generation Agent (Grok sub-agent per Chained_Source_of_Truth_Kimi_Binding.md §6)  
**Governing Criteria:** 8 Validated Denominators + Efficiency Mandate (NEURODIOS-EFF-001) + Binary Optimization Plan v1.1 (NEURODIOS-BIN-OPT-001)  

---

## 1. Purpose

The Tooling & Analysis Suite exists to eliminate manual effort for the Kimi executor (and supporting parallel agents) while executing the Binary Optimization Plan and related workstreams on the NeuroDiOS sovereign Jasterish stack.

It directly operationalizes:
- Denominator #2 (Budget / Resource Accounting) via explicit Compute Footprint measurement and minimal-type enforcement.
- Denominators #6 (Primitive Traceability), #7 (Origin Vault), and #8 (Drift Detection) via automated provenance, reproducibility, and drift tooling.
- The Efficiency Mandate: smallest safe integer/fixed-point types in all hot paths and emitted artifacts; zero tolerance for oversized float64 or unnecessary dynamic structures in deterministic control loops.

All artifacts produced by the suite are deterministic, machine-consumable (JSON), and cross-reference the Chained Source of Truth.

## 2. Scope

In-scope (Phase 1 delivery):
- Efficiency Mandate Auditor: static + lightweight dynamic analysis of Python (neurobalance_coordinator.py priority) and C/C++ sources for float literals, oversized type usage, and compute-footprint estimation. Generates exact remediation patches and mathematical savings proofs.
- Binary Footprint Analyzer: inspection of ELF/Mach-O artifacts (build/, jasterish-microkernel outputs, Jasterish self-host .bin) for section sizes, data type inference where possible, reproducibility hashes, and Phase 2 optimization recommendations.
- Chained Truth Maintainer: validation of cross-document coherence (Chained_Source_of_Truth_Kimi_Binding.md, Binary-Optimization-Plan.md, Validated-Denominators.md, PROJECT_SUMMARY.md, TODO.md, specs) including legacy "5 Denominators" detection, link integrity, and status reporting against the 8 denominators.

Out-of-scope for this triad (future phases):
- Full AST rewriting engines (use external after audit).
- Jasterish-language-specific parser (deferred until compiler stabilization per Phase 1 of Binary Opt Plan).
- Runtime monitoring hooks inside the micro-kernel (post-QEMU bring-up).

## 3. Functional Requirements

FR-01: The suite SHALL produce only deterministic outputs. Identical inputs + environment flags MUST yield byte-identical reports (modulo timestamps explicitly isolated in a "meta" object).

FR-02: Efficiency Mandate Auditor SHALL:
- Detect all Python float literals and float comparisons used in hot-path functions (assess_and_offset, generate_uplift_opportunities, offset engine recommend methods).
- Map each violation to the specific ValidatedDenominator(s) it undermines.
- Compute exact bit-level savings: for each float64 value replaced by Intensity8 (8-bit) or FixedQ7_9 (16-bit), savings = 64 - target_bits bits per instance per evaluation cycle.
- Emit a machine-readable JSON report + human remediation diff suitable for safe application.
- Provide a --strict-mode that fails CI if any float remains in audited hot paths.

FR-03: Binary Footprint Analyzer SHALL:
- Invoke platform binutils (size, readelf, objdump, nm, otool on Darwin) without external Python packages.
- Report per-binary: total size, .text/.data/.rodata/.bss breakdown, presence of debug symbols, architecture.
- Detect and flag oversized immediate/data patterns (e.g., 64-bit constants in control paths that could be 8/16-bit).
- Produce reproducibility score: SHA256 of stripped binary + section hash vector.
- Output explicit Phase 2/3 recommendations aligned to Binary-Optimization-Plan.md.

FR-04: Chained Truth Maintainer SHALL:
- Parse the canonical Chained binding and cross-reference documents.
- Report any reference to "5 Validated Denominators" or pre-2026-05-29 language as HISTORICAL with pointer to NEURODIOS-VD-001.
- Verify that Binary Optimization Plan "Current Execution Status" accurately reflects observed state of Phase 1 items (Jasterish .data emission).
- Emit a compact JSON status packet consumable by Live Context Maintenance Agent for binding updates.

FR-05: Every tool SHALL accept a --denominator-map or equivalent that forces explicit justification of output against the 8 denominators. Non-mapped outputs are rejected.

FR-06: All tools SHALL be executable from scripts/ with zero installation beyond a Python 3.10+ interpreter and standard POSIX utilities.

## 4. Non-Functional Requirements

NFR-01 (Determinism): All analysis paths use only pure functions + fixed seed where randomness is required (none in v1). Hash all intermediate artifacts.

NFR-02 (Footprint): The auditor and analyzer themselves MUST run with < 64 MiB RSS and complete a full tree scan in < 30 s on reference NUC hardware.

NFR-03 (Provenance): Every report SHALL embed the git commit, build host, and SHA256 of the exact source files analyzed (Origin Vault).

NFR-04 (Safety): No tool shall write to source trees unless invoked with an explicit, narrowly-scoped --apply-safe flag that first creates a timestamped backup and records the change in a local audit log.

## 5. Constraints

- Language: Python 3.10+ (stdlib only: ast, re, subprocess, hashlib, dataclasses, json, pathlib) or POSIX shell.
- No network, no PyPI dependencies.
- Must coexist with and enhance (never duplicate) existing scripts/benchmark_determinism.sh, binary_drift_check.sh, generate_provenance_manifest.sh.
- All numeric claims (footprint deltas) MUST be accompanied by the exact arithmetic derivation.

## 6. Success Criteria (Measurable)

SC-01: Running efficiency_mandate_auditor.py --target neurobalance/neurobalance_coordinator.py produces a report that enumerates every float literal in hot paths with line numbers and exact bit-savings calculation (target: >= 12 violations flagged, >= 70% potential reduction in hot-path scalar storage).

SC-02: binary_footprint_analyzer.py run against build/ and build-minsize/ produces section tables whose sums match `size` output within 1 byte and identifies at least 3 opportunities for INT8/INT16 promotion or stripping.

SC-03: chained_source_maintainer.py run against the dual-root binding set reports 0 undetected legacy "5 Denominator" references in active documents and correctly classifies current Binary Opt Plan Phase 1 status.

SC-04: All three tools produce identical output on two consecutive runs with identical inputs (modulo isolated timestamp field).

## 7. Traceability to 8 Validated Denominators & Efficiency Mandate

| Denominator                  | Direct Support in Suite                                                                 |
|------------------------------|-----------------------------------------------------------------------------------------|
| 1. Fluctuation Dynamics      | Auditor detects velocity/acceleration computations using oversized floats (indirect)   |
| 2. Budget / Resource Accounting + Compute Footprint | Primary: explicit footprint math + minimal-type enforcement in auditor and analyzer    |
| 3. Contrast Differential     | Maintainer reports contrast between bloated vs minimal binary states as uplift source  |
| 4. Controlled Oscillation    | Tools support safe measurement of before/after optimization cycles                     |
| 5. Adaptation Offset         | Auditor suggests concrete minimal-type OffsetActions for code remediation              |
| 6. Primitive Traceability    | Analyzer + Maintainer emit atomic dependency maps for every analyzed artifact          |
| 7. Origin Vault              | Every report embeds full provenance (commit, host, file hashes)                        |
| 8. Drift Detection           | Direct integration points with binary_drift_check.sh; reproducibility scoring          |

Efficiency Mandate (sub-budget of #2 + token layer): The entire suite exists to detect and eliminate violations of "smallest safe integer type" rule.

## 8. Interfaces

- CLI: `python3 scripts/<tool>.py --help` with consistent flags: --root, --json, --strict, --apply-safe, --denominators.
- Output contract: Top-level object always contains "meta": {"tool": "...", "version": "1.0", "git_commit": "...", "timestamp_iso": "...", "denominator_justification": [...] }.
- Exit codes: 0 = clean, 1 = violations found (actionable), 2 = internal error.

---

**End of Requirements**

This document, together with the linked Design Specification (NEURODIOS-TOOL-DES-001) and Technical Specification (NEURODIOS-TOOL-TECH-001), constitutes the complete authoritative baseline for the NeuroDiOS Tooling & Analysis Suite. All subsequent implementation and usage MUST reference these three documents.