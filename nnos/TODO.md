# NNOS TODO

1. Add the missing `CMakeLists.txt` and split the current demo entry points into real `lsa_boot_dcn`, `lsa_boot_hcn`, and `lsa_boot_epn` targets.

**2026-05-29 Pivot from developer-portal M3:** After strong completion of the M3 Production Multi-Angle Visibility phase on the pragmatic Option C track (cards + full-spectrum tests + specs), focus has returned to maturing the sovereign Option D NeuroDiOS / Jasterish Micro-Kernel foundation.

**2026-05-28 Major Direction + Strategic Option D (still active):** The significantly improved and expanded Jasterish Micro-Kernel has been integrated. The JStar-2 codegen data-fixup bug has been fixed. 

More importantly, a deliberate long-term sovereign path (Option D) is now active: build a custom proprietary platform orchestration layer on the Jasterish/NeuroDiOS foundation to reduce dependence on third-party heavyweight orchestrators like OpenChoreo. 

- Design Specification for the Sovereign Platform Kernel created.
- Technical Specification stub for minimal viable orchestration primitives created.
- Detailed gap analysis (current Jasterish assets vs OpenChoreo's actual responsibilities) created.

In parallel, a sub-agent has been spawned to pursue Option C (developer-portal as first-class extension of OpenChoreo) as the pragmatic interim solution while the sovereign foundation matures.

The full triad of specs for the current kernel has been created and will be extended. Existing nnos daemons should be re-planned as user-space services. Next priorities: QEMU bring-up, toolchain integration, Phase 2 spec extensions, and exploration of the full sovereign platform kernel scope.

**2026-05-28:** Full NNOS Daemon Constellation + Jasterish Validation/Morph Port triad (Requirements/Design/Technical) created in docs/. These are now the authoritative baseline. Use them when implementing the daemons and the Jasterish ports.

**2026-05-29 Session Priority after Pivot:** 
- Foundational primitives work completed: Catalogue of Behavioral Health Condition Primitives + full triad created.
- Roller Coaster Framework (major new artifact) completed: primitives clustered and analyzed through mirror/counter/orthogonal/synth/observer/anchored relational modes; anchoring by emulation of a prime defined; controlled stress-relief cycles for uplift/insight generation specified with full triad.
- Systemic Denominators Dissection completed: 5 Validated Denominators extracted and accepted.
- Roller Coaster Framework completed.
- NeuroBalance Engine Integration (major): Original neurobalance-engine.py elevated as central health governor.
- Expanded Uplift System: Uplift now explicitly includes active generation of solutions to real problems + constant creation of compounding improvement capacity.
- **Major Directive (2026-05-30):** Multi-Agent Support Layer Activated.
  - Five specialized background sub-agents spawned to support the Kimi executor in parallel across all five operational modes.
  - Live Context Maintenance, Parallel Deep Analysis, Tooling/Script Generation, Criteria Enforcement/Review, and Dual-Root Synchronization.
  - All agents feed outputs back into the Chained Source of Truth binding documents in real time.
- **2026-05-30 Recipes Folder Review Complete:** Full analysis of all 20 C recipes in engine/recipes/.
  - No new base denominators promoted.
  - Recipes classified as the "Recipes & Validation Layer".
- **2026-05-30 Binary Optimization Scan Complete:** Systematic review of all significant compiled artifacts.
  - New plan: `docs/2026-05-30-Binary-Optimization-Plan.md`.
  - Critical gaps: Jasterish self-host structural defects (no section headers, .data problems), lack of multi-arch for TP-HCF, weak provenance, missing minimal-footprint discipline on binaries.
  - 4-phase optimization plan defined (T-Diagram stabilization → efficiency/minimal footprint → provenance + multi-arch → continuous optimization).
- Next: Get the expanded Phase 2 Jasterish Micro-Kernel (11 modules) actually booting in QEMU using the current Makefile.
- Integrate / validate against the improved JStar compiler outputs from the apps/ tree.
- Extend any missing triad coverage for new subsystems as they are activated.
- Re-plan the older C++ NNOS daemons as user-space services on the new kernel where appropriate.
- Keep dual-track visibility: Option C (developer-portal) remains the usable interim surface.
2. Reconcile naming across the specs: `boot_daemon` and `nnos_*` child processes in the SRS versus `lsa_boot_*` executables in `LSA-TECH-002`.
3. Define the real runtime surface for containers:
   - Docker image build inputs
   - supervisord config ownership
   - Podman rootless launch and quadlet strategy
4. Decide whether the expanded daemon set (`nnos_morph_engine`, `nnos_threat_scanner`, `nnos_neuro_analyzer`) belongs in the baseline constellation or a later profile of the system.
5. Add versioned shared-state schemas before introducing new daemon fields such as `threat_score`, `insight_score`, or `morph_cycle`.
6. Provide deterministic benchmark fixtures from real NNOS traces or canonical state captures, then wire them into `scripts/benchmark_determinism.sh`.
7. Add the missing tests called out in the design docs: shared-state, ethernet sync, task manager, context gate, drift detection, and origin-vault coverage.
8. Smoke-test the bootstrap and runtime flow on the intended targets:
   - Ubuntu 24.04 on the NUC
   - JetPack 6.x on the Orin
   - Parrot OS host with Ubuntu guest VM
   - Docker and Podman rootless on Linux
9. Materialize the Jasterish validation-layer port in a runtime module rather than trying to validate every compiler IR instruction.
10. Populate the bootstrap registry stub from the real NNOS capability rules once the 180-pattern and use-case workstreams are finalized.
11. Run the Linux-only Jasterish self-host ladder on the NUC with the full `compiler.jstr` source present, then verify that the validation runtime can be linked without breaking the fixpoint.
12. Use `apps/scripts/jstar_bootstrap_check.sh` as the mandatory preflight before any future Jasterish bootstrap run.
13. Add Linux-run stage hashing only after `compiler.jstr` is restored; the current checkout cannot produce a meaningful `hashes.txt`.
14. Keep diagnosis disciplined: do not reopen `.data` fixup theory unless new evidence contradicts the existing linker behavior in `apps/src/jstar/linker.rs`.
15. Lock the shared-state schema before implementing the design-stage Jasterish morph engine runtime.
