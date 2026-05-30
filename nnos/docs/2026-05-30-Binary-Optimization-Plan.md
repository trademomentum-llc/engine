# Binary Optimization Plan — NeuroDiOS / Jasterish Sovereign Stack

**Document ID:** NEURODIOS-BIN-OPT-001  
**Version:** 1.0  
**Date:** 2026-05-30

---

## Executive Summary

A scan of all significant compiled artifacts across the active development roots (`apps/` and `engine/nnos/`) reveals several systemic gaps relative to the 8 Validated Denominators and the Efficiency Mandate.

**Primary Problems Identified:**
1. Jasterish self-hosting outputs suffer from severe structural defects (missing section headers, truncated or absent .data, extreme size variance between "successful" builds). This directly violates determinism, Origin Vault, and minimal footprint principles.
2. No consistent multi-architecture / cross-compilation discipline aligned with the TP-HCF (Tri-Plane Heterogeneous Compute Fabric).
3. Production daemons and kernel artifacts are not yet being built with the minimal-footprint discipline (stripping, LTO, smallest safe data types in emitted code, etc.).
4. Very weak linkage between built binaries and the provenance / traceability requirements (Origin Vault, Primitive Traceability).
5. The sovereign Jasterish Micro-Kernel has a good Makefile but no active, verified build artifacts in the current tree.

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
2. Add deterministic build verification to the existing `jstar_bootstrap_check.sh` and Makefile:
   - Record SHA256 of final .bin/.elf.
   - Compare against previous known-good on every build.
3. Produce at least one verified, bootable Jasterish Micro-Kernel binary using the current Makefile + a stable jstar (Rust bootstrap for now).

### Phase 2 — Efficiency & Minimal Footprint (2–4 weeks)
4. Apply the `minimal_types` discipline to the Jasterish codegen where possible (prefer 16-bit/8-bit immediates and registers for control flow, bounds checks, etc.).
5. Add release optimization flags to the NNOS daemon build (LTO, -Os or -Oz, strip debug symbols from __LINKEDIT).
6. Create a "minimal footprint" build profile for the Jasterish compiler itself (smaller emitted code for the sovereign kernel use case).

### Phase 3 — Provenance, Traceability & Multi-Arch (4–8 weeks)
7. Integrate binary provenance into the Origin Vault / Primitive Traceability system:
   - Every produced .bin/.elf must embed or be accompanied by a manifest linking it to source commit + primitive map + compiler version.
8. Establish cross-compilation for the TP-HCF:
   - x86_64 ELF daemons and kernel for NUC nodes.
   - aarch64 for Orin / M1 nodes.
   - Unified build system (CMake or the existing Makefile extended).
9. Add automated binary drift detection (hash + size + section layout comparison) as part of the existing `drift_detection.c` / recipe infrastructure.

### Phase 4 — Continuous Optimization & Uplift
10. Make the NeuroBalance Engine (once ported to run on the sovereign kernel) actively monitor and report Compute Footprint of running binaries/daemons.
11. Use successful minimal-footprint improvements as "compounding wins" in the Improvement Ledger.
12. Feed binary optimization insights back into the Roller Coaster cycles (use the contrast between bloated vs minimal builds as a source of system-level uplift).

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