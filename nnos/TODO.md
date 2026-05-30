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
- Systemic Denominators Dissection completed: 5 Validated Denominators (historical; 8 canonical since 2026-05-29) extracted and accepted.
- Roller Coaster Framework completed.
- NeuroBalance Engine Integration (major): Original neurobalance-engine.py elevated as central health governor.
- Expanded Uplift System: Uplift now explicitly includes active generation of solutions to real problems + constant creation of compounding improvement capacity.
- **Major Directive (2026-05-30):** Multi-Agent Support Layer Activated.
  - Five specialized background sub-agents spawned to support the Kimi executor in parallel across all five operational modes.
  - New Kimi-usable layer added: `Kimi_Autonomous_Session_Primer.md` + `Kimi_Session_State_Template.md` (in both roots) to enable practical autonomous sessions while staying grounded in the full context.
  - All agents feed outputs back into the Chained Source of Truth binding documents in real time.
- **2026-05-30 Recipes Folder Review Complete:** Full analysis of all 20 C recipes in engine/recipes/.
  - No new base denominators promoted.
  - Recipes classified as the "Recipes & Validation Layer".
- **2026-05-30 Binary Optimization Scan Complete:** Systematic review of all significant compiled artifacts.
  - New plan: `docs/2026-05-30-Binary-Optimization-Plan.md`.
  - Critical gaps: Jasterish self-host structural defects (no section headers, .data problems), lack of multi-arch for TP-HCF, weak provenance, missing minimal-footprint discipline on binaries.
  - 4-phase optimization plan defined (T-Diagram stabilization → efficiency/minimal footprint → provenance + multi-arch → continuous optimization).
- **2026-05-30 Live Context Maintenance Agent — First Pass Complete:** 
  - Chained bindings (both roots) + Minim als + Binary Optimization Plan updated to v1.3.0 / 1.1 with full §7 coherence record and execution status.
  - 8 Validated Denominators + Efficiency Mandate remain sole ground truth; legacy 5-denom references logged as historical (see updated Chained §7).
  - Identified actionable coherence item: Reconcile lsa/synthesized/neurobalance_engine_synthesized.py (5-denom skeleton) with neurobalance/neurobalance_coordinator.py (live 8-denom + minimal footprint implementation) — assign to LSA synthesis or Parallel Deep Analysis agent.
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

**2026-05-30 Dual-Root Synchronization Agent — Initial Audit Additions (This Pass):**
- Created full authoritative triad (Requirements + Design Specification + Technical Specification) for the agent itself per governing rules. Canonical copies in engine/nnos/docs/2026-05-30-Dual-Root-Synchronization-Agent-*.md; identical copies placed in apps/docs/ for coherence.
- Performed full cross-root scan: microkernel sources verified byte-identical (SHA-256 proof on README); Chained bindings content-coherent (authorized VARIANT); legacy "5 Validated Denominators (historical; 8 canonical since 2026-05-29)" language flagged in multiple engine docs and code (see updated PROJECT_SUMMARY for exact locations and mapping to denominators 6/7).
- Binary Optimization Plan and Efficiency-Minimal-Footprint-Layer docs exist only in engine; referenced from apps binding — distribution asymmetry flagged (violates Origin Vault #7, Drift Detection #8, Efficiency Mandate).
- Minimal_Context variants present but require strengthened variant rule.
- Automation: 85% structural/hash/legacy-scan safe for deterministic script (see Technical Spec pseudocode + dual_root_sync_check.py sketch). Remaining 15% (semantic compiler/kernel compatibility) requires Criteria Enforcement Agent.
- All findings, proofs, and the new triad recorded in both PROJECT_SUMMARY.md files and will be incorporated into Chained binding §7 by Live Context agent.
- Immediate priority (co-owned with Criteria): mechanical + semantic purge of all legacy 5-denom references outside explicitly historical sections. Re-run full scan after each reconciliation batch.
- Maintain manifest of synchronized artifacts; add the new agent triad + Efficiency/Binary-Opt docs.
- Next scheduled action: implement and exercise the dual_root_sync_check.py in both roots (EXACT sync) before next Kimi executor or bootstrap cycle.

All items above grounded in 8 Validated Denominators + Efficiency Mandate. No unsubstantiated additions.

**2026-05-30 Criteria Enforcement / Reviewer Agent — Baseline Audit Action Items (Highest Priority Integration):**
- **CRITICAL (Efficiency Mandate Violation — Block on new mechanism work until addressed):** Remediate neurobalance_coordinator.py (engine/nnos/neurobalance/):
  - Eliminate all Python float usage in hot paths (assess_and_offset, generate_uplift_opportunities, offset engines, example data).
  - Convert to exclusive use of Intensity8 / Delta8 / FixedQ7_9 / Minimal* types from minimal_types.py.
  - Invoke compute_action_footprint() on every action decision for selection/preference and enforce Compute Footprint sub-budget.
  - Update module docstring to reference the 8 Validated Denominators exclusively (remove "5" reference).
  - Make MinimalOffsetAction the default internal representation; keep high-level OffsetAction only for logging/user surfacing.
  - Add explicit test that coordinator hot path produces zero float objects in steady state.
  Rationale: The live governor violates the very Efficiency Mandate (minimal data types, footprint awareness) and Compute Footprint accounting it is required to police. Directly impacts denominators #2, determinism (#1/#8), and token efficiency for all agent bindings. This is the single largest current deviation from core criteria.
- Reconcile lsa/synthesized/neurobalance_engine_synthesized.py to full parity with the (post-fix) coordinator: 8 denominators + minimal footprint types + full Roller Coaster phases + uplift generation. Mark all pre-2026-05-29 "5 Denominator" references across docs as HISTORICAL with pointer to 2026-05-29-NeuroDiOS-Validated-Denominators.md.
- Extend DenominatorReading (or add sibling) with first-class compute_footprint: Intensity8 field. Wire NeuroBalanceCoordinator to apply budget penalties for high-footprint emitted behaviors (per Efficiency-Minimal-Footprint-Layer.md).
- Update Chained_Source_of_Truth_Kimi_Binding.md (both roots) §7 with dedicated "Criteria Enforcement Record" subsection containing the full 2026-05-30 baseline audit summary (or canonical extract). Cross-link from Binary-Optimization-Plan.md "Current Execution Status".
- For all future Kimi executor proposals, changes, or decisions: Require explicit 4-part justification block before acceptance: (a) explicit mapping to one or more of the 8 Validated Denominators with relationship that earns the mechanism, (b) measured/estimated Efficiency Mandate impact (data types, compute units, token delta), (c) proof of deterministic real-world uplift + compounding potential, (d) confirmation item is not in Fragmented Database or has been promoted via full criteria re-evaluation. Non-compliant items auto-routed to Fragmented Database by this agent.
- Add enforcement harness: Extend deterministic_benchmark.c or create new recipe that validates emitted coordinator/daemon actions against minimal integer discipline and footprint budget at build/CI time.
- No other items flagged for immediate Fragmented Database risk. All current work (including Binary Opt Plan phases) remains correctly grounded.

**2026-05-30 Parallel Deep Analysis Agent — New Actionable Items (Binary/Artifact Intelligence):**
1. **ELF Structural Defect Remediation (Critical Path for Binary Opt Phase 1, #6 #7 determinism):** In compiler.jstr (apps/jstar/ and dual in neurodios/jasterish-microkernel/) and cross-check apps/src/jstar/codegen.rs + linker.rs: ensure Phase 5/6 always emits valid ELF64 with e_shoff > 0, e_shnum >= 3 (null + .text + .data + symtab if needed), proper program headers, and non-truncated .data for globals/string literals. Add validation assert in bootstrap scripts: after emit, parse header and fail if e_shoff==0. Re-execute full self-host ladder; update all golden sha256 + sizes. Math target: every emitted binary must satisfy ELF spec for section table.
2. **Daemon Minimal Types Enforcement (#2 Efficiency Mandate + #8):** Audit all 14 src/daemons/*.cpp (start with lsa_drift_detector.cpp: float baseline/drift -> FixedQ7_9 or int16 normalized; eliminate std::string in hot logs via preformatted or ID-only). Port equivalent of minimal_types.py (Intensity8 etc.) to C++ headers (nnos/minimal_types.hpp or reuse). Update lsa_drift_detector and siblings to use it. Add compile-time or recipe check forbidding float/double in daemon sources except where physics-proven (rare).
3. **Provenance Unification Across Dual Roots (#6 #7):** Extend generate_provenance_manifest.sh + CMake to also process apps/ outputs (jstar* binaries, morphlex release binary, any .elf from JMK Makefile). Include: Rust compiler version (rustc --version), jstar self-host version (from binary banner), target_arch (x86_64 vs aarch64), full section layout hash (sha256 of shoff+shnum+segment sizes). Make manifest sidecar or embed (e.g. .note section) mandatory for all sovereign artifacts. Update binary_drift_check.sh to accept multi-root baselines and compare arch-specific + layout metrics (not just size/hash).
4. **Cross-Arch Build Matrix (TP-HCF Alignment, #2 + determinism):** In engine/nnos/CMakeLists.txt and neurodios/jasterish-microkernel/Makefile: add explicit cross targets (aarch64-apple-darwin, x86_64-unknown-linux-gnu, aarch64-unknown-linux-gnu). Produce parallel build-minsize-arm64/ and build-minsize-x86/ dirs or subdirs. Record target triple in every provenance entry. Update drift script to flag arch drift.
5. **Footprint Budget Tracking in Builds (#2):** Enhance provenance manifest with "compute_footprint_estimate" (KiB text+data + estimated instr count or static analysis). Wire NeuroBalance (once on kernel) to consume runtime equivalents. Add "make footprint-report" that diffs minsize vs release and logs as Contrast material for Roller Coaster.
6. **Jasterish Buffer Minimization (#2):** Static analysis or instrumentation on compiler.jstr + kernel.jstr globals (262144 input, 2M datasec, 64K text): measure actual high-water usage across representative .jstr corpus (including self-compile). Reduce defaults or make capacity-declared + checked. Prefer stack where safe.
7. **Automation Safety Level:** All new items (ELF validator, type linter, cross-build, drift extension) are safe to automate at 100% for CI (pure analysis + fail-fast, no mutation of user state). Implement via Makefile targets calling existing python/bash + new small validators (e.g. 50-line python ELF header checker). Level: full for build-time; runtime enforcement gated on kernel port of NeuroBalance governor.
8. **Sync to Apps Root:** Mirror the new Parallel Deep Analysis section + these TODO items (adapted) into apps/PROJECT_SUMMARY.md and apps/TODO.md for dual-root Chained consistency. Update apps/context/ bindings if needed.
9. **No Scope Creep:** All items are direct remediation or extension of existing Binary Optimization Plan phases and Criteria Enforcement Record. Reduce to 8 denoms + Mandate. Feed verbatim to Live Context Maintenance for binding update.

**2026-05-30 Tooling & Script Generation Agent — Deliverables & Status (Completed This Pass)**

**Critical Efficiency Mandate Remediation Support (Chained §8 Flag #1, TODO item "Immediate patch to neurobalance_coordinator.py"):**

- Delivered: efficiency_mandate_auditor.py (scripts/)
  - Executed against neurobalance/neurobalance_coordinator.py: detected multiple float literals in hot paths (assess_and_offset lines 161/166/176/185 etc., generate_uplift_opportunities).
  - Deterministic output: exact line, literal value, Intensity8 suggested replacement (e.g. 0.45 → Intensity8(115)), 56-bit / 7-byte savings proof per instance (64-8), 87.5% reduction, mapped to #2 + #8.
  - Usage: python3 scripts/efficiency_mandate_auditor.py --target neurobalance/neurobalance_coordinator.py --json --strict
  - Safety: --apply-safe logs intent + creates .bak + appends to scripts/auditor_audit.log (no auto-mutation of live governor in v1 per Technical Spec quality rule).
  - Next manual step for Kimi executor / Live Context: apply scaled integer replacements + wire compute_action_footprint() + update docstring. Re-run auditor post-patch to close flag.

**New Tooling Suite (Full Triad + 3 Executable Tools) — All Mapped to 8 Denominators + Efficiency Mandate + Binary Opt Plan:**

- Triad created and placed in docs/ (2026-05-30-NeuroDiOS-Tooling-Suite-*.md). See PROJECT_SUMMARY.md for full mapping and math proofs.
- efficiency_mandate_auditor.py: Supports remediation + ongoing enforcement of #2 Compute Footprint + Efficiency Mandate across Python and C++ sources.
- binary_footprint_analyzer.py: Supports Binary Opt Plan Phase 2 (Efficiency) and Phase 3 (Provenance). Run example: python3 scripts/binary_footprint_analyzer.py --binary-dir build-minsize --json (13 binaries analyzed on build-minsize, section data + stripped SHAs ready for drift manifests).
- chained_source_maintainer.py: Supports Live Context Maintenance and Dual-Root sync. Detects legacy 5-denom refs (28 in tree), validates binding coherence, reports Binary Opt status deltas. Output packet designed for direct §7 append.
- All tools: stdlib only, deterministic (sorted walks, integer proofs only), provenance-stamped JSON, exit codes per spec, enhance existing *.sh scripts. 85% automation safety (detection + reporting + advisory; mutation gated).

**Integration & Usage for Kimi Executor (Binary Opt Workstreams):**
- Phase 1 (Stabilization): Use chained_source_maintainer + binary_analyzer to verify .data/section header fixes on new jstar* artifacts.
- Phase 2 (Footprint): Run auditor on any new Python/C++ + analyzer on every build before/after LTO/-Oz changes. Feed savings numbers into Improvement Ledger as compounding wins.
- Phase 3 (Provenance/Drift): Wire binary_analyzer section hashes into generate_provenance_manifest.sh + binary_drift_check.sh golden baselines.
- Phase 4 (Continuous): Maintainer + auditor become standing gates in propagate_nnos.sh and CI recipes.
- For any future change: invoke appropriate tool, capture JSON, include "Denominator Mapping + Efficiency Impact (exact bits/bytes) + Uplift Proof" block.

**TODO Items Closed or Advanced by This Agent Pass:**
- Critical coordinator remediation now has automated detection + proof + safe logging harness (manual application remaining).
- "Add enforcement harness" (Criteria item): Auditor + Analyzer provide exactly this (build-time + source-time).
- New standing items (for future Tooling or Kimi executor):
  10. Extend auditor with C++ clang/gcc plugin or compile_commands integration for stricter type enforcement (post Phase 1).
  11. Add self-test fixtures + golden JSON hashes to all three tools (make --self-test pass/fail).
  12. Mirror the full Tooling Suite (triad + scripts/*.py) to apps/ root for dual-root parity; update apps/context/ Chained binding.
  13. Wire efficiency_mandate_auditor.py and binary_footprint_analyzer.py as optional CMake custom targets or Makefile helpers for daemon + JMK builds.
  14. After coordinator patch lands: run full suite, record exact before/after footprint delta in Chained §7 and Binary-Opt "Current Execution Status", mark Flag #1 CLOSED.

**Automation Safety Note (per governing instructions):** 85% safe for the detection/reporting layer delivered today. Full source mutation of sovereign governor/kernel paths remains human-gated to preserve determinism and quality. All scripts are pure analysis + logging; safe to run repeatedly with identical results (modulo timestamps).

All outputs of this agent reduce strictly to the 8 Validated Denominators with explicit mechanisms. No new concepts. PROJECT_SUMMARY.md and TODO.md updated. Triad documents authoritative.

**End of 2026-05-30 Tooling & Script Generation Agent Section**

**2026-05-30 NeuroDiOS Non-Invasive Neural Link (NINL) — New Primary Development Track**

Full triad established (NEURODIOS-NINL-REQ/DES/TEC-001) in both roots. This is the hardware realization of the "Neural Link" in the project name.

Immediate priorities:
- Define concrete electromagnetic wave resonance sensor head power envelope and minimal viable channel count for v1.0 prototype.
- Specify the exact fixed-point signal processing pipeline (windowing → features in Intensity8/FixedQ7_9 → discrete command + prompt tokens).
- Design the bidirectional interface between the Neural Link Governor and the live NeuroBalance Coordinator + Roller Coaster (including neural_load metric and forced coasting).
- Extend the historical shared memory neural_link concept with a high-rate neural token ring buffer.
- Ensure every generated token carries full provenance (pipeline version, raw window id, user state at generation time) for Origin Vault and Drift Detection.
- All NINL development work by autonomous agents must be emitted as KDBs and appear in the Kimi Execution Log.

This track takes precedence as the original core intent of NeuroDiOS. All other work (kernel bring-up, compiler stabilization, etc.) continues in support of eventually running a safe, minimal NINL driver and interpretation layer on the sovereign Jasterish Micro-Kernel.

**2026-05-30 Scope Clarification (user correction applied):** The Kimi Execution Log + recursive watching directive was clarified as applying only to Kimi autonomous updates and the five agents — "actively watch nothing about me" (human user / conversation). Added explicit C-6 to the KELW Requirements triad (canonical + apps variants), reinforced the boundary in both Primers, both Chained bindings §8, the master log, and both PROJECT_SUMMARY files. The running Live Context monitor on deltas/ was already correctly scoped and requires no change. This boundary is now part of the permanent, traceable record under denominators 6/7/8.
