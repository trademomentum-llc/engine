# NNOS Project Summary

Project: Neurodivergent Neural-Link Operating System on the TP-HCF stack.

Current focus (pivoted 2026-05-29 from developer-portal M3 Production Model completion):
- **NeuroDiOS Jasterish Micro-Kernel Foundation** (integrated + significantly expanded 2026-05-28, Phase 2 subsystems largely complete per EXPANSION_PLAN.md) — the deterministic bare-metal sovereign base for Option D.
- Foundational work on Behavioral Health Condition Primitives (the "very beginning" primitives requested 2026-05-29). Full catalogue + triad created.
- Roller Coaster Framework designed on top of the primitives: clusters defined, relational modes formalized (mirror, counter, orthogonal, synth, observer + compounds), anchoring by emulation of a prime, and controlled stress-relief cycles for deliberate uplift/insight generation. Full Requirements/Design/Technical triad created.
- Systemic Denominators Dissection completed (2026-05-29): Rigorous extraction and evaluation of common elements against strict criteria (atomic + kinetic + fractal + relationship-earned mechanisms + deterministic uplift). 5 Validated Denominators accepted. Fragmented Database created for non-qualifying concepts.
- Advancing the expanded kernel (IDT, higher-half, ATA/VFS, ELF exec, CoW fork, ~13k lines) toward actual QEMU bootable image and toolchain integration with the improved apps/ JStar-2 compiler.
- Long-term sovereign platform kernel (Option D) to reduce dependence on third-party orchestrators.
- Supporting specs for VM, container, benchmark, daemon-expansion, and kernel evolution work.
- Determinism benchmarking around fixed-input NNOS/NeuroDiOS computations and runtime variance.
- Design-stage planning for Jasterish ports of the NNOS validation and morph engines (now also as user-space services on the new kernel).
- Grounded diagnosis of the Jasterish bootstrap path based on the real `apps/` checkout.

What now exists in this tree:
- `scripts/bootstrap_encoding.sh` to normalize UTF-8 and line endings across the NNOS subtree.
- `scripts/propagate_nnos.sh` to validate the environment, detect native versus VM versus container modes, and prepare build and runtime wiring.
- `scripts/benchmark_determinism.sh` to hash and time repeated runs of a real command against a fixed fixture.
- Draft specs for validation-layer and morph-engine Jasterish porting, plus a grounded codegen/bootstrap diagnosis.
- A repository-local Jasterish bootstrap check script in `apps/scripts/jstar_bootstrap_check.sh`.

## NeuroDiOS Jasterish Micro-Kernel (Integrated 2026-05-28)

**Source Origin:** `~/Downloads/Kimi_Agent_Jasterish Micro-Kernel Build/jasterish-microkernel/`

**Integration Location:** `engine/nnos/neurodios/jasterish-microkernel/`

This is the seed deterministic micro-kernel written in Jasterish that will become the privileged foundation for the full NeuroDiOS (evolution of nnos). 

**Major Update (integrated 2026-05-28 from improved work in Apps folder):**
The kernel has been significantly expanded with the latest versions from apps/jasterish-microkernel/. It now includes:

- All previous v1 modules (improved/larger versions)
- IDT & full exception handling (idt.jstr)
- ATA PIO disk driver + simple VFS/RAMFS with persistence (disk.jstr + vfs.jstr)
- ELF loader + sys_exec (elf.jstr)
- Copy-on-Write fork
- Expanded syscall table and shell commands
- EXPANSION_PLAN.md documenting systematic Phase 2 completion

Current approximate size: ~12,700 lines of JStar across 11 modules.

**Critical Compiler Fix (2026-05-28):** The root cause of persistent self-host divergence in the JStar 2 (Rust bootstrap) compiler — cumulative re-patching of data_fixups inside emit_function() on every function entry, causing compounding offsets and non-deterministic .text — has been identified and removed. The linker (patch_data_addresses) was already performing the single correct pass. Both the expanded NeuroDiOS kernel sources and the self-hosted compiler (compiler.jstr) now build cleanly without divergence. This unblocks reliable development of the kernel foundation.

**Governing Documents (full initial triad created 2026-05-28):**
- `neurodios/docs/2026-05-28-NeuroDiOS-Jasterish-Micro-Kernel-Requirements.md`
- `neurodios/docs/2026-05-28-NeuroDiOS-Jasterish-Micro-Kernel-Design-Specification.md`
- `neurodios/docs/2026-05-28-NeuroDiOS-Jasterish-Micro-Kernel-Technical-Specification.md`

These three documents are the authoritative starting point for all future NeuroDiOS kernel work. They were created per the governing persona rules the same day the kernel sources were integrated.

The existing nnos daemons (SystemIntegrity, Morphogenetic, Threat, etc.) are intended to become user-space services running on top of this kernel.

**2026-05-29 Pivot Note:** After major completion of the developer-portal M3 Production Multi-Angle Visibility work (five annotation-driven Backstage cards, deterministic namespace predictor fully wired + tested in full-spectrum harness, both M3 and Cards Technical Specifications delivered), primary focus has pivoted back to the sovereign Option D path in engine/nnos. The pragmatic Option C surface (developer-portal) is now in a strong, demonstrable state while the long-term NeuroDiOS / Jasterish kernel foundation is matured.

**Strategic Direction – Option D (Sovereign Platform):** A deliberate long-term path is now active to build a custom, proprietary platform orchestration and control layer on top of the Jasterish Micro-Kernel and NeuroDiOS foundation. The goal is to reduce or eventually eliminate dependence on external heavyweight orchestrators (such as OpenChoreo) for core weight-bearing concerns. Initial Requirements framing for this sovereign direction has been created in `neurodios/docs/2026-05-28-NeuroDiOS-Sovereign-Platform-Kernel-Requirements.md`.

**2026-05-29/30 Full nnos-lsa Synthesis + Recipes Review:** 
- Entire nnos-lsa folder integrated (original material in engine/nnos/lsa/original/).
- 8 Validated Denominators remain the foundation after systematic review of the `engine/recipes/` folder (20 C validation recipes).
- No new base denominators were promoted from the recipes. They primarily serve as concrete implementations and validators for the existing 8 (especially Drift Detection, Traceability, Determinism, and Morphogenetic patterns).
- One concept (Morphogenetic Repair) noted in Fragmented Database for potential future evaluation.
- All recipes now understood as the "Recipes & Validation Layer" sitting atop the denominators.

What still does not exist:
- The spec-defined `CMakeLists.txt` and the role-specific `lsa_boot_dcn`, `lsa_boot_hcn`, and `lsa_boot_epn` binaries.
- Materialized systemd units, quadlets, or container images tied to real NNOS executables.
- The current seven-daemon baseline as a runnable supervised constellation.
- A verified Linux self-host path for the Jasterish compiler that can carry NNOS runtime modules.
- The `jstar/compiler.jstr` source file required by the existing self-host ladder in `apps/src/jstar/mod.rs`.

Reality check:

## Specifications (2026-05-28 Update)

Full Requirements + Design + Technical Specification triad created for the NNOS Daemon Constellation + Jasterish Validation/Morph Port (core of the neurodivergent operating system):

- `docs/2026-05-28-NNOS-Daemon-Constellation-Requirements.md`
- `docs/2026-05-28-NNOS-Daemon-Constellation-Design-Specification.md`
- `docs/2026-05-28-NNOS-Daemon-Constellation-Technical-Specification.md`

These provide the authoritative baseline for the 6-daemon constellation, Jasterish port contracts (validation as runtime guard, morphogenetic repair), shared-state schema, and build/deployment requirements. They directly address the persona mandate and link to the Jasterish triad in apps/ and the broader ecosystem.
- `boot.cpp` and `main_engine.cpp` are still demos, not production boot daemons.
- The new scripts are honest about that gap: they can normalize files and validate the environment today, but build and install steps will fail until the build tree catches up to the specs.
- The new morphogenetic, threat, and neuro-analysis modules are exploratory extensions, not baseline NNOS requirements yet.
- The Jasterish validation port is now documented, but it is still a design artifact. The compiler-side integration is not complete, and the current design correctly treats validation as a runtime guard, not as a compiler pass over every emitted IR instruction.
- The one verified compiler defect fixed so far is indexed access over pointer-valued slots in `apps/src/jstar/codegen.rs`.
- One previously pasted diagnosis point was incorrect for this checkout: `.data` fixups are already applied in `apps/src/jstar/linker.rs`, so that is not currently the primary suspect.
