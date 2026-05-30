# Jasterish Codegen Phase Fix Technical Specification

| Field | Value |
| --- | --- |
| Document ID | JSTAR-CG-FIX-TECH-001 |
| Version | 1.0.0 |
| Date | 2026-03-21 |
| Status | Draft |
| Related Specs | JSTAR-CG-DIAG-REQ-001 |
| Classification | Internal |
| Encoding | UTF-8 without BOM |

## 1. Implemented Changes

1. [`apps/src/jstar/codegen.rs`](/Users/nexus1/Projects/apps/src/jstar/codegen.rs) now tracks whether a virtual register names direct inline storage or a slot containing a pointer/scalar value.
2. Indexed access helpers load pointer-valued bases from the slot or global location before emitting SIB-based memory access.
3. Global direct-storage metadata is reconstructed when `MachineCode` generation initializes `global_vregs`.
4. Regression tests now cover:
   - direct-storage indexed loads
   - pointer-valued indexed loads
   - pointer-valued indexed stores

## 2. Bootstrap Check Script

[`apps/scripts/jstar_bootstrap_check.sh`](/Users/nexus1/Projects/apps/scripts/jstar_bootstrap_check.sh) provides the current deterministic entry point for bootstrap verification.

Behavior:

1. Runs `cargo test jstar::codegen -- --nocapture` on any host.
2. Fails fast if:
   - `cargo` is missing
   - the host is not Linux
   - `jstar/compiler.jstr` is not present
3. On a Linux host with `compiler.jstr` available, `RUN_FIXPOINT=1` escalates to `cargo test test_t_diagram_fixpoint -- --ignored --nocapture`.

## 3. Linux NUC Execution Sequence

[`apps/scripts/jstar_bootstrap_linux.sh`](/Users/nexus1/Projects/apps/scripts/jstar_bootstrap_linux.sh) is the repo-local runner for the NUC/Linux path.

Behavior:

1. Requires Linux and `cargo`.
2. Accepts `JSTAR_COMPILER_SRC` when the compiler source exists outside the canonical `apps/jstar/compiler.jstr` location.
3. Stages a temporary symlink into `apps/jstar/compiler.jstr` when needed so the existing Rust self-host tests can run unchanged.
4. Runs:
   - bootstrap preflight
   - `test_selfhost_arithmetic`
   - `test_selfhost_variable`
   - `test_selfhost_if_else`
   - `test_t_diagram_fixpoint`

## 4. Explicit Non-Fixes

1. No change was made to `data_fixups` handling because the linker already patches them in [`apps/src/jstar/linker.rs`](/Users/nexus1/Projects/apps/src/jstar/linker.rs).
2. No end-to-end self-host run was claimed from this macOS workspace because the required source file is absent and the relevant tests are Linux-only.

## 5. Remaining Work

1. Restore `compiler.jstr` to the checkout or parameterize the script with the real compiler source path.
2. Run the bootstrap ladder on Linux and capture stage hashes once the source is present.
3. Only after that, investigate any remaining divergence in ABI handling, stack probing, or call patching.
