# Jasterish Codegen Phase Diagnosis Requirements

| Field | Value |
| --- | --- |
| Document ID | JSTAR-CG-DIAG-REQ-001 |
| Version | 1.0.0 |
| Date | 2026-03-21 |
| Status | Draft |
| Related Specs | JSTAR-EXT-ENG-REQ-001, VL-JSTAR-PORT-REQ-001 |
| Classification | Internal |
| Encoding | UTF-8 without BOM |

## 1. Purpose

Diagnose the current Jasterish self-host bootstrap failure using the real compiler tree in `apps/`, not the speculative chat transcript. The immediate goal is to separate verified defects from assumed causes so the bootstrap ladder can be reproduced on the NUC Linux target.

## 2. Verified Findings

1. The real codegen bug found so far is indexed access over pointer-valued stack slots. `LoadIndexed`, `StoreIndexed`, `ArrayLoad`, `ArrayStore`, and hash-addressed paths need to distinguish direct inline storage from slots that hold pointer values.
2. `data_fixups` are already applied in [`apps/src/jstar/linker.rs`](/Users/nexus1/Projects/apps/src/jstar/linker.rs). That is not the live failure mode in this checkout.
3. The self-host ladder in [`apps/src/jstar/mod.rs`](/Users/nexus1/Projects/apps/src/jstar/mod.rs) is Linux-only and the fixpoint test is still marked `#[ignore]`.
4. The current checkout does not contain `jstar/compiler.jstr`, so the bootstrap ladder cannot be executed end to end from this workspace today.

## 3. Functional Requirements

1. Keep regression coverage for pointer-vs-inline-storage indexing in codegen.
2. Add a repository-local bootstrap check that fails fast on missing `compiler.jstr`, missing `cargo`, or a non-Linux host.
3. Preserve a deterministic portable verification path that can run on macOS and Linux: codegen tests must still pass before any Linux-only self-host attempt.
4. When `compiler.jstr` is restored on Linux, run the existing T-diagram fixpoint path before adding new bootstrap theories.

## 4. Non-Functional Requirements

1. Diagnosis must stay evidence-based: only document causes backed by the checked-in source.
2. Bootstrap validation should be reproducible from a single script in the repo.
3. Any future self-host run must emit enough information to compare stage outputs and isolate divergence quickly.

## 5. Immediate Next Steps

1. Supply `jstar/compiler.jstr` or the real path to the self-host source.
2. Run the bootstrap check script on Linux.
3. If the fixpoint still diverges after the pointer-base fix, diff the stage binaries and inspect the first differing code section instead of broadening the diagnosis blindly.
