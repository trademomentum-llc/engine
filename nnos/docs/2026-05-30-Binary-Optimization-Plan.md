# Binary Optimization Plan — NeuroDiOS / Jasterish Sovereign Stack

**Document ID:** NEURODIOS-BIN-OPT-001  
**Version:** 1.1  
**Date:** 2026-05-30  
**Last Maintained:** 2026-05-30 (Live Context Maintenance Agent baseline pass)

---

## Executive Summary

A scan of all significant compiled artifacts across the active development roots (`apps/` and `engine/nnos/`) reveals several systemic gaps relative to the 8 Validated Denominators and the Efficiency Mandate.

**Primary Problems Identified:**
1. Jasterish self-hosting outputs suffer from severe structural defects (missing section headers, truncated or absent .data, extreme size variance between "successful" builds). This directly violates determinism, Origin Vault, and minimal footprint principles.
2. No consistent multi-architecture / cross-compilation discipline aligned with the TP-HCF (Tri-Plane Heterogeneous Compute Fabric).
3. Production daemons and kernel artifacts are not yet being built with the minimal-footprint discipline (stripping, LTO, smallest safe data types in emitted code, etc.).
4. Very weak linkage between built binaries and the provenance / traceability requirements (Origin Vault, Primitive Traceability).
5. The sovereign Jasterish Micro-Kernel has a good Makefile but no active, verified build artifacts in the current tree.

**Binding Layer Status (2026-05-30 Live Context Maintenance Baseline):** This plan is now referenced from the canonical Chained_Source_of_Truth_Kimi_Binding.md (both roots, v1.3.0). All phases explicitly serve denominators #2 (Budget/Compute Footprint), #6 (Primitive Traceability), #7 (Origin Vault), #8 (Drift Detection) and the Efficiency Mandate (smallest safe integer types in codegen + build profiles, provenance manifests, reproducible hashes). No new denominators; strict consistency maintained.

**2026-05-30 Criteria Enforcement / Reviewer Agent Baseline Audit Cross-Link:** Full audit record (including critical flag on neurobalance_coordinator.py Efficiency violation) incorporated into Chained bindings (both roots) §8 and PROJECT_SUMMARY/TODO. Binary Opt phases 2+ (Efficiency & Minimal Footprint, Provenance/Traceability) now gated on remediation of the live governor's non-use of minimal_types + footprint calculator. See Criteria Enforcement Record in bindings for deterministic justification requirements on all future changes. No impact to Phase 1 stabilization work.

---

## Current State of Key Binaries (Summary)

### Apps/ (Jasterish Compiler & Bootstrap)
- **compiler_raw.bin** (~3.8M): Main Rust bootstrap compiler output. Structurally incomplete (no section headers reported).
- **jstar1 / jstar1_fresh variants** (~3.7–3.8M): Larger self-host attempts. Same structural defect.
- **jstar2** (67K): Smaller self-host attempt, also defective.
- Multiple 0B or 146B "jstar*" files: Clear failed/truncated self-host attempts (historical .data section bug still manifesting).
- Many .core files: Evidence of instability during testing.

### engine/nnos/build/ (NNOS / LSA Daemons)
- All daemons are Mach-O arm64 (local development).
- Sizes: 89K–181K. Reasonable for C++ daemons.
- No stripped release variants visible.
- No ELF cross-compiled variants for the x86_64 NUC or other TP-HCF nodes.

### Jasterish Micro-Kernel (engine/nnos/neurodios/jasterish-microkernel/)
- Mature Makefile exists (supports direct QEMU boot, ISO, debug).
- **No built kernel.bin / kernel.elf** present in the current tree.

---

## Gaps vs Established Criteria

| Criterion / Denominator          | Gap Observed in Current Binaries |
|----------------------------------|----------------------------------|
| Determinism & Reproducibility    | Extreme size variance in Jasterish self-host outputs; no systematic hash verification on final binaries. |
| Origin Vault / Provenance        | Almost no linkage between built artifacts and build provenance, source commit, or primitive mapping manifests. |
| Primitive Traceability           | No evidence that emitted binaries declare or are validated against their atomic primitive dependencies at build time. |
| Compute Footprint (Efficiency)   | No stripping, no LTO, no minimal libc, no evidence of INT8/INT16 preference in codegen for control paths. Jasterish outputs not yet benefiting from minimal_types discipline. |
| Drift Detection                  | No automated comparison of built binaries against golden baselines or previous known-good hashes in CI/build. |
| Controlled Oscillation / Uplift  | The Jasterish T-Diagram is still highly unstable — this prevents reliable use of the compiler as a tool for generating uplift (new kernel features, etc.). |
| Multi-Architecture (TP-HCF)      | Current daemon builds are arm64-only. Kernel Makefile is x86_64-only. No unified cross-build system. |
| Minimal Binary Structure         | Many Jasterish outputs lack proper ELF section headers — this breaks tooling, dynamic analysis, and size optimization. |

---

## Prioritized Optimization Plan

### Phase 1 — Immediate Stabilization (1–2 weeks)
1. Fix the Jasterish self-host .data / section header problem so that self-hosted outputs are structurally valid ELF with proper sections (repeat the earlier codegen diagnosis and apply fixes to compiler.jstr Phase 5).
   - **STATUS (2026-05-30):** Partially complete. Simple global variable datasec allocation enabled in compiler.jstr (lines 2515-2528). Previously DISABLED — all simple globals fell through to stack allocation. String literal emission was already active. Verification requires NUC Linux session (apps/ root under active Linux control; jstar2/jstar3 are ELF64 binaries that cannot be executed on macOS). See apps/TODO.md for ongoing Phase 5 work.
2. Add deterministic build verification to the existing `jstar_bootstrap_check.sh` and Makefile:
   - Record SHA256 of final .bin/.elf.
   - Compare against previous known-good on every build.
   - **STATUS (2026-05-30):** CMake provenance_manifest target generates sha256sums.txt at build time. `generate_provenance_manifest.sh` produces JSON manifests with git commit + compiler + per-binary hashes. `binary_drift_check.sh` compares current build against baseline. Verified on MinSizeRel build.
3. Produce at least one verified, bootable Jasterish Micro-Kernel binary using the current Makefile + a stable jstar (Rust bootstrap for now).
   - **STATUS (2026-05-30):** Dockerfile.build created for cross-compilation environment. `build_jmk_docker.sh` provides one-command Docker build. Makefile is mature but requires x86_64-elf-ld / jstar compiler from apps/ tree. Blocked on macOS; NUC Linux session is the correct execution environment.

### Phase 2 — Efficiency & Minimal Footprint (2–4 weeks)
4. Apply the `minimal_types` discipline to the Jasterish codegen where possible (prefer 16-bit/8-bit immediates and registers for control flow, bounds checks, etc.).
5. Add release optimization flags to the NNOS daemon build (LTO, -Os or -Oz, strip debug symbols from __LINKEDIT).
   - **STATUS (2026-05-30):** Complete. CMake MinSizeRel profile active with `-Os -fno-rtti -fno-exceptions`. LTO enabled via `check_ipo_supported()`. Post-link strip implemented per-platform (Darwin: `-x`, Linux: `--strip-all`). Size reduction: 56–69% vs debug build (e.g., lsa_context_gate: 181K → 56K). 13 binaries verified.
6. Create a "minimal footprint" build profile for the Jasterish compiler itself (smaller emitted code for the sovereign kernel use case).

### Phase 3 — Provenance, Traceability & Multi-Arch (4–8 weeks)
7. Integrate binary provenance into the Origin Vault / Primitive Traceability system:
   - Every produced .bin/.elf must embed or be accompanied by a manifest linking it to source commit + primitive map + compiler version.
   - **STATUS (2026-05-30):** `generate_provenance_manifest.sh` produces JSON manifest with git commit, compiler version, primitive map ref, and per-binary sha256/size/file_type. Flat `sha256sums.txt` for quick verification. CMake install target includes provenance. Verified on MinSizeRel build.
8. Establish cross-compilation for the TP-HCF:
   - x86_64 ELF daemons and kernel for NUC nodes.
   - aarch64 for Orin / M1 nodes.
   - Unified build system (CMake or the existing Makefile extended).
   - **STATUS (2026-05-30):** CMake toolchain files created: `cmake/toolchains/x86_64-linux-gnu.cmake` (NUC, `-march=x86-64-v2`) and `cmake/toolchains/aarch64-linux-gnu.cmake` (Orin, `-march=armv8.2-a+crc+crypto`). Not yet verified due to lack of cross-compilers on macOS build host.
9. Add automated binary drift detection (hash + size + section layout comparison) as part of the existing `drift_detection.c` / recipe infrastructure.
   - **STATUS (2026-05-30):** `binary_drift_check.sh` compares current build artifacts against baseline manifest. Exit 0 = no drift, Exit 1 = drift detected, Exit 2 = missing baseline. Verified against MinSizeRel provenance manifest.

### Phase 4 — Continuous Optimization & Uplift
10. Make the NeuroBalance Engine (once ported to run on the sovereign kernel) actively monitor and report Compute Footprint of running binaries/daemons.
11. Use successful minimal-footprint improvements as "compounding wins" in the Improvement Ledger.
12. Feed binary optimization insights back into the Roller Coaster cycles (use the contrast between bloated vs minimal builds as a source of system-level uplift).

---

## Current Execution Status (2026-05-30 Baseline — Live Context Maintenance)

**No Kimi executor progress beyond plan creation observed in this cycle.** The following records the exact state at agent instantiation for deterministic tracking (Origin Vault principle).

- **Phase 1 (Immediate Stabilization):** ACTIVE on apps root. Root cause of jstar3 .data truncation (missing string literal + global variable emission in compiler.jstr Phase 5 codegen) identified. Work in progress: implement data emission to datasec, data_len increment, offset patching (see apps/TODO.md items and jstar_bootstrap_out/ artifacts). Directly enforces #6 Primitive Traceability, #7 Origin Vault, determinism, and minimal binary structure. Dual-root Chained binding cross-references this as primary near-term uplift.

- **Phase 2 (Efficiency & Minimal Footprint):** Not started. Pre-requisite: Phase 1 stabilization. Will apply minimal_types discipline (INT8/INT16 preference) to Jasterish codegen and NNOS daemon release profiles (LTO, -Oz, strip). Maps to Efficiency Mandate + denominator #2.

- **Phase 3 (Provenance, Traceability & Multi-Arch):** Not started. Requires stable T-Diagram. Will embed manifests linking binaries to commit + primitive_map.json + compiler version; establish x86_64/aarch64 cross-build for TP-HCF nodes.

- **Phase 4 (Continuous):** Not started. Depends on NeuroBalance port to kernel + successful minimal wins.

**Drift Detection hook:** Existing scripts/binary_drift_check.sh and recipes/drift_detection.c provide foundation. Binding requires SHA256 + section layout + size comparison on every jstar* and daemon build once Phase 1 completes.

**Relationship to 8 Denominators + Efficiency (re-validated this pass):** All gaps and phases remain strictly derived from the denominators enumerated in Chained_Source_of_Truth_Kimi_Binding.md §2. No deviations.

---

## Success Metrics (Deterministic)

- Jasterish self-host outputs have valid ELF section headers and non-zero .data when expected.
- All production binaries have recorded SHA256 that is reproducible across clean builds on the same toolchain.
- NNOS daemons and Jasterish kernel have a "release-minimal" build variant whose size is documented and tracked.
- Every binary artifact has an accompanying provenance manifest that can be verified against the Origin Vault.
- The T-Diagram for the Jasterish compiler is stable enough that self-hosted builds are routinely used for kernel development.

---

## Relationship to Existing Work

This plan directly serves and is governed by the 8 Validated Denominators (especially #2 Budget/Compute Footprint, #4 Controlled Oscillation, #6 Traceability, #7 Origin Vault, #8 Drift Detection) and the Efficiency Mandate.

It is a necessary prerequisite for reliable use of the Roller Coaster Framework and Solution-Generating Uplift at the systems level (you cannot generate trustworthy uplift from an unstable compiler or bloated, untraceable binaries).

---

**End of Binary Optimization Plan**