# Requirements Specification: AetherOS Jasterish C Compiler — Full-OS Toolchain Requirements

**Document ID:** AETHER-JSTARC-REQ-001  
**Version:** 1.0  
**Date:** 2026-09-07  
**Status:** Baseline for review  
**Origin:** First wave under the emulated-environment build landscape  
**Governing Context:** 8 Validated Denominators + Efficiency Mandate + First-Principles Operating Doctrine

---

## 1. Purpose and Scope

### 1.1 Purpose

This document fixes the requirements for a C-hosted Jasterish compiler — working name **JStar-C** — sufficient to build the full AetherOS operating system from Jasterish source to deterministic ELF64 binaries. Terms defined on first use: a **quark** is an irreducible Jasterish language primitive (THING — exists; ACTION — changes; LINK — connects); a **spark** is one machine-level operation formed when quarks combine in valid patterns fixed by the swarm task specifications; **ELF64** is the 64-bit Executable and Linkable Format of all emitted artifacts; the **trusted computing base (TCB)** is the set of components whose failure compromises every artifact produced.

The C-hosted path supersedes the Rust-hosted track (`apps/src/jstar`) as the path of least resistance to a self-hosting, kernel-capable toolchain. This document states requirements only — no design decisions, no algorithms, no code. The Design Specification and Technical Specification follow in the required triad per the spec-triad convention.

- FR-GOAL-01: JStar-C must compile the complete Jasterish language as frozen by the 2026-09-07 swarm task specifications; no subset may be silently accepted as complete. [Swarm task specs]
- FR-GOAL-02: JStar-C must be sufficient to build bootable AetherOS images for x86-64 and AArch64 targets from Jasterish kernel sources. [NEURODIOS-REQ-001 lineage; JMK spec triad]
- FR-GOAL-03: JStar-C must be implemented in auditable C11, freestanding-capable, with a strictly smaller TCB than the superseded Rust track. [Decision record, Section 1.4]
- FR-GOAL-04: JStar-C must support a staged self-hosting ladder terminating in a byte-identical fixpoint; the ladder stages, gates, and golden artifacts are specified in the Self-Hosting Ladder chapter of this specification. [NEURODIOS-BIN-OPT-001 success metrics]
- FR-GOAL-05: This document must govern requirements only; all design and technical decisions are delegated to the Design Specification and Technical Specification of the JStar-C triad. [Spec-triad convention]

### 1.2 Scope

| In scope | Out of scope |
|----------|--------------|
| Language frontend: lexer, tokenizer, parser, semantic validation | Jasterish language redesign — semantics are frozen by the 2026-09-07 swarm task specifications |
| Intermediate representation and x86-64 / AArch64 code generation | AetherOS kernel internal design — owned by the JMK spec triad |
| ELF64 binary emission with post-emit self-validation | Packaging or deployment beyond the emulated environment |
| Freestanding runtime and AetherOS kernel integration | Hosted userland distribution channels |
| Self-hosting ladder, determinism, provenance, drift gating | Hardware bring-up on physical boards (board profiles are noted as stubs) |
| Telemetry hooks (aetherProbe) and verification gates | Telemetry plane internals beyond the compiler-side contract |

- FR-SCOPE-01: Every in-scope row of the table above must be covered by at least one numbered requirement in the corresponding chapter of this specification. [This document]
- FR-SCOPE-02: No out-of-scope item may be absorbed into JStar-C work without an AMEND-NNN amendment; silent scope absorption is classified as an Adaptation Offset violation. [Denominator 5]
- FR-SCOPE-03: The Jasterish language semantics fixed by the swarm task specifications (TOKENIZER_TASK, PARSER_TASK, CODEGEN_TASK, INTEGRATION_TASK) must be restated normatively in this specification's language chapters, not redefined. [Swarm task specs]

### 1.3 Strategic Context (Full-OS Ambition)

JStar-C is not a prototype tool; it is the toolchain on which the full AetherOS operating system rests. The compiler must ultimately compile the kernel's own Jasterish sources — kernel, HAL, architecture-specific sources, and the compiler's own self-host source — into bootable images under the JMK specifications. Prior Rust-track self-hosting attempts produced structurally defective binaries (missing section headers, truncated `.data`, extreme size variance between nominally successful builds), recorded by the Binary Optimization Plan as direct violations of determinism, Origin Vault, and minimal-footprint principles. The C path is adopted under the challenge-assumption discipline of the First-Principles Operating Doctrine: the kernel target is C-adjacent, freestanding C11 toolchains exist on all three node topologies of the Tri-Plane Heterogeneous Compute Fabric (NUC / M1 / Orin), no cargo/LLVM dependency chain is required, and the resulting TCB is materially smaller.

### 1.4 Decision Record (Rust Track vs C Track)

The six core goals are defined as: **Usefulness** — the tool does the required work; **Reliability** — predictable behavior under repetition and stress; **Understanding** — a competent engineer can fully explain the tool's behavior; **Adaptability** — correction and extension without destabilizing dependents; **Sustainability** — dependencies and resource costs remain viable long-term; **Simplicity** — no mechanism beyond what the task requires.

| Core goal | Rust track (apps/src/jstar) | C track (JStar-C) | Basis |
|-----------|------------------------------|-------------------|-------|
| Usefulness | Produces ELF64 but with structural defects (e_shoff=0, truncated .data) | Must emit spec-valid ELF64; defects are remediated by numbered requirements | NEURODIOS-BIN-OPT-001 defect list |
| Reliability | Extreme size variance between "successful" self-host builds | Deterministic byte-identical output mandated and gate-enforced | Denominators 1, 8 |
| Understanding | cargo/LLVM-era dependency chain obscures build provenance | C11 source, no external toolchain runtime, fully auditable | Efficiency Mandate; minimal TCB |
| Adaptability | Rust track fixes constrained by ecosystem toolchain versions | Freestanding C11 available on all three node topologies | TP-HCF node matrix |
| Sustainability | Requires sustained Rust toolchain availability on every node | System C compiler suffices; smaller recurring dependency cost | Emulated-environment build landscape |
| Simplicity | Larger language runtime and build system in the TCB | Minimal C runtime; TCB reduction is explicit and measurable | Core-goal doctrine |

- FR-DEC-01: The Rust implementation at `apps/src/jstar` must be preserved as executable reference semantics; it must not be deleted, and behavioral divergences between tracks must be resolved against the swarm task specifications, not against the Rust implementation. [Baseline preservation rule]
- FR-DEC-02: Any future re-evaluation of the C-over-Rust decision must be recorded as an AMEND-NNN amendment with written rationale and denominator mapping before adoption. [First-Principles Operating Doctrine]

### 1.5 Normative References and Governing Documents

**Provenance note (binding).** The historical documents `2026-05-29-NeuroDiOS-Validated-Denominators.md`, `2026-05-29-NeuroDiOS-Efficiency-Minimal-Footprint-Layer.md`, and `2026-05-30-Binary-Optimization-Plan.md` were removed from the engine repository HEAD in commit `a492a29` (2026-09-03, recorded rationale: "contained confidential documentation"). They are referenced here strictly as **lineage artifacts** by original document ID and date, never as live repository paths. The canonical text of the 8 Validated Denominators lives in NEURODIOS-CSOT-KIMI-001 §2.

| Doc ID | Title | Date | What it binds |
|--------|-------|------|----------------|
| NEURODIOS-CSOT-KIMI-001 v1.3.0 | Chained Source of Truth (Kimi Binding) | 2026-05-30 | Canonical definitions of the 8 Validated Denominators; rule that every artifact maps to one or more denominators and declares Efficiency impact |
| NEURODIOS-VD-001 v1.0.0 | Validated Denominators | 2026-05-29 | Lineage artifact — denominator validation criteria; the legacy five-denominator language is historical and must not be reintroduced |
| NEURODIOS-EFF-001 v1.0 | Efficiency / Minimal-Footprint Layer | 2026-05-29 | Lineage artifact — minimal-type discipline, Compute Footprint sub-budget, token/context footprint |
| NEURODIOS-BIN-OPT-001 v1.1 | Binary Optimization Plan | 2026-05-30 | Lineage artifact — defect list, four-phase remediation plan, provenance manifest and drift-check contracts, acceptance metrics |
| FPOD-2026-09-07 | First-Principles Operating Doctrine | 2026-09-07 | Spec-first workflow, deterministic builds, hash-everything provenance, action cataloging, consensus before destructive actions, honest uncertainty |
| Telemetry Plane specification | aetherProbe Telemetry Plane | 2026-09-07 | Compiler-side instrumentation contract (metrics, traces, structured logs; SEB ring with stderr JSONL degradation) |
| Swarm task specifications | `compiler_requirements.md` (TOKENIZER/PARSER/CODEGEN/INTEGRATION) | 2026-09-07 | Frozen Jasterish language semantics — the sole ground truth for language behavior |
| NEURODIOS-REQ-001 + JMK spec triad | Jasterish Micro-Kernel Requirements / Design / Technical | 2026-05-28 | Kernel interfaces, load addresses, boot verification flow; kernel internal design ownership |

#### 1.5.1 Precedence Order

Conflict resolution order, highest first:

1. **8 Validated Denominators** (NEURODIOS-CSOT-KIMI-001 §2)
2. **Efficiency Mandate** (NEURODIOS-EFF-001)
3. **This document** (AETHER-JSTARC-REQ-001)
4. **Implementation choices** (design and technical volumes, code)

- FR-REF-01: All normative references in this specification must be cited by document ID and date, never by mutable repository path. [Provenance note]
- FR-REF-02: No requirement in this document may contradict a higher-precedence source; a detected conflict must be escalated through the AMEND-NNN procedure, not resolved silently. [First-Principles Operating Doctrine]
- FR-REF-03: Where the swarm task specifications and any implementation disagree on language behavior, the swarm task specifications must prevail. [FR-SCOPE-03]
- FR-REF-04: The legacy "five Validated Denominators" formulation must never be reintroduced outside historical citation; the binding set is exactly eight. [NEURODIOS-VD-001 criteria enforcement]

### 1.6 Governing Constraints

#### 1.6.1 The 8 Validated Denominators (Verbatim) Applied to a Compiler

The following denominators are quoted verbatim from NEURODIOS-CSOT-KIMI-001 v1.3.0, §2 (2026-05-30):

> "The following denominators are the immutable physics/governance layer for NeuroDiOS. Every artifact must explicitly map to one or more denominators and declare its Efficiency impact.
>
> 1. **Fluctuation Dynamics** — Rate of change, acceleration, and duration of any measurable state.
> 2. **Budget / Resource Accounting** — Depletable, quantifiable, enforceable units of capacity.
> 3. **Contrast Differential** — Measurable difference between two states or phases.
> 4. **Controlled Oscillation** — Deliberate, bounded, periodic movement between high-tension and low-tension states.
> 5. **Adaptation Offset** — Bounded compensatory actions in response to detected states or trajectories.
> 6. **Primitive Traceability / Atomic Dependency Mapping** — First-class dependency and provenance tracking.
> 7. **Origin Vault (Deterministic Provenance & State History)** — Immutable source-of-truth and state history.
> 8. **Drift Detection** — Operational detection of deviation from declared truth."

| # | Denominator | Compiler implication for JStar-C | Binding requirement |
|---|-------------|-----------------------------------|---------------------|
| 1 | Fluctuation Dynamics | Build variance (size, timing) is a health signal; extreme variance between "successful" builds is a defect | FR-DEN-01 |
| 2 | Budget / Resource Accounting | Minimal resources in compiler and emitted code; footprint is a budgeted resource | FR-DEN-02 |
| 3 | Contrast Differential | Minimal-vs-bloated build contrast measured and reported (56–69% MinSizeRel precedent) | FR-DEN-03 |
| 4 | Controlled Oscillation | Compiler stable across stress/relief cycles; an unstable T-diagram blocks system-level uplift | FR-DEN-04 |
| 5 | Adaptation Offset | Smallest effective correction; bounded remediations, never scope creep | FR-DEN-05 |
| 6 | Primitive Traceability / Atomic Dependency Mapping | Emitted binaries declare and validate atomic primitive dependencies at build time | FR-DEN-06 |
| 7 | Origin Vault | Every binary linked by manifest to source commit, primitive map, compiler version; reproducible bytes | FR-DEN-07 |
| 8 | Drift Detection | Builds machine-compared to golden baselines (SHA256 + size + section layout) with exit-code gating | FR-DEN-08 |

- FR-DEN-01: JStar-C must produce byte-identical output for identical input on the same toolchain, and build-to-build size variance beyond declared tolerances must fail the verification gate. [Denominator 1]
- FR-DEN-02: Compiler hot paths and generated code must apply the smallest-safe-integer discipline of Section 1.6.2; Compute Footprint must be tracked as an explicit sub-resource. [Denominator 2]
- FR-DEN-03: A minimal-footprint build profile must exist for JStar-C itself, and its size contrast against the default build must be recorded and tracked. [Denominator 3]
- FR-DEN-04: The self-hosting ladder must remain continuously executable; a broken stage must block dependent work rather than propagate. [Denominator 4]
- FR-DEN-05: Defect remediations under this specification must be scoped to the identified defect; each remediation must name the defect it resolves. [Denominator 5]
- FR-DEN-06: Every emitted artifact must carry or reference an atomic primitive dependency declaration validated at build time. [Denominator 6]
- FR-DEN-07: Every emitted artifact must carry a provenance manifest with the fields fixed in the Determinism, Provenance and Drift chapter (schema version, build date, git commit, git branch, build host, compiler, primitive map reference, per-binary name/SHA256/size/type). [Denominator 7]
- FR-DEN-08: Every build must be checked against golden baselines using SHA256, size, and section-layout comparison, with exit codes 0 (no drift), 1 (drift detected), and 2 (baseline missing) honored as gate semantics. [Denominator 8]

#### 1.6.2 Efficiency Mandate

The governing rule is quoted verbatim from NEURODIOS-EFF-001 v1.0 (2026-05-29):

> "**Objective:** Every action, primitive evaluation, mechanism, and context load must use the absolute minimum computational and token resources required for correctness and determinism."
>
> Governance rule: "What is the smallest safe integer type (or fixed-point representation) that can express this behavior deterministically? Use that — never more."

The canonical minimal types (live enforcement artifact `minimal_types.py`) binding on compiler data paths:

| Type | Definition | Compiler application |
|------|------------|----------------------|
| `Intensity8` | Normalized intensity/capacity/load in [0, 255] (u8) | Capacity and load accounting in compiler and emitted code |
| `Delta8` | Signed velocity/delta in [-128, 127] | Change-rate values in telemetry and control paths |
| `Intensity16` / `Delta16` | Higher precision where accumulation demands it (i16) | Accumulated counters (e.g., accumulated debt equivalents) |
| `FixedQ7_9` | Q7.9 fixed point, range ~ -256 to +256 at ~0.002 precision (scaled by 2^9, clamped to [-32768, 32767]) | Fractional values in deterministic control and budget calculations |
| `compute_action_footprint()` | Estimated compute cost in abstract units (base 1; +2 for Fluctuation/Drift actions; intensity cost `(intensity >> 5) + 1`) | Footprint accounting on compiler actions; cheaper equivalent actions preferred |

- FR-EFF-01: No float or double may appear in compiler hot paths or in deterministic emission paths; FP32 is permitted only with documented physics/precision justification. [NEURODIOS-EFF-001; AGENTS.md §4]
- FR-EFF-02: Generated code must prefer 8-bit and 16-bit immediates and registers for control flow and bounds checks wherever the value range permits. [NEURODIOS-BIN-OPT-001 Phase 2, item 4]
- FR-EFF-03: Every compiler action subject to cataloging must carry a compute-footprint declaration consistent with `compute_action_footprint()` semantics. [minimal_types.py]
- FR-EFF-04: Every new `.jstr` source file compiled under this toolchain should carry a compute-footprint budget declaration in its header as a matter of source hygiene; for sources accepted by the toolchain this declaration is mandatory and enforced per FR-RT-10. [AGENTS.md §5; enforcement strength resolved 2026-09-07 against FR-RT-10]
- FR-EFF-05: The compiler must be buildable in a minimal-footprint profile whose size is documented and tracked against the default build. [FR-DEN-03]

#### 1.6.3 First-Principles Operating Doctrine Inheritance

| Doctrine element | Compiler obligation | Requirement |
|------------------|---------------------|-------------|
| Spec-first | No implementation work begins without a governing requirement ID in this specification | FR-DOC-01 |
| Deterministic builds | Byte-identical rebuilds on any host; no timestamps, host paths, or uninitialized padding in output | FR-DOC-02 |
| Hash-everything provenance | Every artifact and every build input is hashed; hashes recorded in the provenance manifest | FR-DOC-03 |
| Action cataloging | Build actions recorded as temporal action events compilable to markdown notebooks | FR-DOC-04 |
| Consensus before destructive actions | Deletion or supersession of any baseline artifact requires recorded consensus (AMEND-NNN) | FR-DOC-05 |
| Honest uncertainty | Partial compliance must be declared with requirement IDs and gate evidence; no unqualified conformance claims | FR-DOC-06 |

- FR-DOC-01: Every code change to JStar-C must cite at least one requirement ID from this specification; uncited work is non-conforming. [Spec-first]
- FR-DOC-02: Rebuilds on any supported host must produce byte-identical artifacts; sources of nondeterminism (timestamps, absolute paths, uninitialized bytes) must be excluded by construction. [Deterministic builds]
- FR-DOC-03: All build inputs and outputs must be SHA256-hashed and recorded; unhashed artifacts must not pass the verification gate. [Hash-everything]
- FR-DOC-04: Build actions must be emitted as temporal action events conforming to `action_event.schema.json`, compilable to markdown notebooks per the 2026-09-07 notebook compiler contract. [Action cataloging]
- FR-DOC-05: No baseline artifact listed in §2 may be deleted, replaced, or superseded without a recorded AMEND-NNN amendment adopted by consensus. [Consensus before destructive actions]
- FR-DOC-06: Conformance statements must cite requirement IDs and passing gate evidence; partial or unverified compliance must be stated as such, with the open items enumerated. [Honest uncertainty]

---

## 2. Current Baseline (Brownfield — Preserved, Superseded, Remediated)

JStar-C is brownfield work. The following artifacts exist, are binding, and define the remediation floor for this specification.

| Artifact | Role | Status under JStar-C |
|----------|------|----------------------|
| `apps/src/jstar` (Rust reference implementation) | Executable language semantics; T-diagram fixpoint precedent | Preserved as reference; superseded as the production path |
| `NeuroDiOS/compiler/jstar/jstar_c.c` (existing C port) | First C-path attempt; mirrors Rust phases (Tokenize → Parse → Codegen → Link) | Baseline to remediate; defects listed below are binding inputs to this specification |
| aetherProbe telemetry SDK (Layer 1) | Metrics, traces, and structured logs contract | Preserved; compiler instrumentation hooks specified in the Telemetry chapter |
| Swarm task specifications (`compiler_requirements.md`) | Frozen Jasterish language semantics (TOKENIZER/PARSER/CODEGEN/INTEGRATION tasks) | Binding; restated normatively, never redefined |

Known defects of the existing C port (`jstar_c.c`), each remediated by a numbered requirement of this specification:

- FR-BASE-01: Emitted ELF64 binaries must carry a valid section header table — non-zero `e_shoff`, correct `e_shentsize`, `e_shnum`, and `e_shstrndx` — replacing the baseline's hardcoded zero fields. [NEURODIOS-BIN-OPT-001 Phase 1; ELF emission chapter]
- FR-BASE-02: The baseline's single `PT_LOAD` segment with `PF_R|PF_W|PF_X` must be superseded by separated RX text and RW data segments, or the exception must be explicitly justified and documented per bootstrap stage. [NEURODIOS-BIN-OPT-001]
- FR-BASE-03: The hardcoded entry address `0x400078` must be replaced by the layout conventions fixed in the ELF emission chapter (userland `0x400000` lineage; kernel load addresses per JMK specifications). [INTEGRATION_TASK lineage]
- FR-BASE-04: Stubbed codegen paths (ADD/SUB/MUL/DIV/STORE/LOAD/IF/WHILE marked "simplified: just skip for now") must be fully implemented; no emitted program may contain a silently skipped construct. [Swarm CODEGEN_TASK]
- FR-BASE-05: The kernel-level Jasterish constructs `array` and `for ... from ... to`, recorded as missing on 2026-07-04, must be implemented; language coverage must be complete against the frozen semantics. This requirement mandates the capability and its constraints — counted iteration is While-equivalent and arrays are capacity-declared contiguous regions — while the concrete English surface syntax and any AST extension node are explicitly deferred to AMEND-001 in the Design Specification volume. [Binding maintenance log]
- FR-BASE-06: Full `.data` emission (string literals and globals) must be guaranteed, terminating the truncated/absent `.data` lineage that produced 0-byte and 146-byte self-host outputs. [NEURODIOS-BIN-OPT-001 artifact evidence]
- FR-BASE-07: Fixed buffer capacities inherited from the baseline (`MAX_TEXT 65536`, `MAX_DATA 2097152`, `MAX_TOKENS 32768`, `MAX_VARS 512`) must be declared and capacity-checked, with exact diagnostics on overflow — never assumed. [Semantic validation chapter]
- FR-BASE-08: The positive invariant of the Rust track — byte-identical ELF output for identical input (`test_elf_determinism`, T-diagram fixpoint across the self-host ladder jstar1 → jstar2 → jstar3 → jstar4 → jstar5, canonical invariant jstar4 == jstar5 byte-identical, lineage stage sizes 123,259 B (jstar2) and 70,925 B (jstar3–jstar5) accepted as the baseline pending governed amendment) — must be preserved and gate-enforced in JStar-C. [Baseline preservation rule]

---

*This section-file covers the front matter and Sections 1–2 (Purpose and Scope — including Normative References and Governing Constraints — and Current Baseline). Subsequent chapters (Language Model onward) and the fixed document tail (Non-Functional Requirements → Constraints and Assumptions → Success Criteria → end sentinel) are contributed by companion section-files under the same skeleton.*

---

## 3. Language Model and Definitions

This section fixes the normative language model of Jasterish as implemented by JStar-C. The semantics restated here are frozen by the 2026-09-07 swarm task specifications (`morehlex-deterministic/compiler_requirements.md`); JStar-C must implement them exactly and must not extend, reinterpret, or weaken them except where this document explicitly remediates a known defect.

### 3.1 Quarks and Sparks

**Quark** — the atomic semantic primitive of Jasterish. Every source construct is composed of quarks. Three quark kinds exist, and no others:

| Quark kind | Definition | Lexical realization |
|---|---|---|
| THING | that which exists (a named entity, a value, a type) | NOUN, NUMBER, TYPE tokens |
| ACTION | that which changes (an operation applied to things) | DECL, VERB, CONDITION tokens |
| LINK | that which connects (binds an action to its operand things) | PREP, RELATION, auxiliary tokens such as `is`, `into`, `to`, `then` |

**Spark** — one machine-level operation, formed when quarks touch in a valid pattern. A spark is the unit of meaning: a source sentence either matches a spark pattern exactly or it has no defined meaning. The set of valid spark patterns is fixed by the swarm specifications and restated in Section 5.1; the minimum normative set is binding on every conforming implementation.

**Sentence** — a THING-ACTION-LINK-THING pattern expressed in natural English surface syntax, terminated by a period. Example: `Store five into the counter.` Parenthesized comments are ignored entirely and carry no meaning.

- FR-LANG-01: JStar-C must implement exactly three quark kinds — THING, ACTION, LINK — with the definitions above; the compiler must not introduce additional primitive kinds at the language-model level.
- FR-LANG-02: JStar-C must treat a spark as the sole unit of machine-level meaning; a sentence that matches no valid spark pattern must be rejected, never assigned an inferred or approximate meaning.
- FR-LANG-03: JStar-C must define a sentence as a period-terminated THING-ACTION-LINK-THING pattern in natural English surface form; input outside sentence boundaries must produce defined error behavior per §4 and §5.
- FR-LANG-04: JStar-C must ignore parenthesized comments completely; comment content must not influence the token stream, the AST, the IR, or any emitted artifact.

### 3.2 Lexical Definitions

**Token** — the output unit of lexical analysis. Every token carries a class, a surface form, an optional resolved value, and a source position (line and column). Eight token classes exist:

| Class | Definition | Examples |
|---|---|---|
| DECL | declaration verb introducing a named entity | `declare` (and `array`, `for` heads per §3.4) |
| NOUN | a named thing: identifier or reference | `counter`, `result`, `tss_stack` |
| VERB | an action verb other than declaration | `store`, `add`, `halt`, `call`, `return` |
| PREP | a linking preposition | `into`, `to`, `from`, `at` |
| NUMBER | a number word carrying a defined integer value | `five`→5, `ten`→10, `fifteen`→15 |
| TYPE | a type word | `byte`, `long` |
| CONDITION | a conditional head | `if`, `while` |
| RELATION | a comparison phrase | `is equal to`, `is greater than`, `is less than` |

Compound RELATION phrases (`is equal to`, `is greater than`, `is less than`) are atomic: they are consumed as one token and must never be split into constituent words. Number words carry their integer values as part of the token; the vocabulary of recognized number words is externalized per §4.3.

- FR-LANG-05: JStar-C must define and classify exactly the eight token classes in the table above; every token in the emitted token stream must carry exactly one of these classes or be an error token per FR-LEX-07.
- FR-LANG-06: JStar-C must resolve the compound phrases `is equal to`, `is greater than`, and `is less than` as single atomic RELATION tokens; partial matches must not be reinterpreted as separate tokens.
- FR-LANG-07: JStar-C must attach the resolved integer value to every NUMBER token at lexical time; at minimum the values `five`→5, `ten`→10, and `fifteen`→15 must be defined by the externalized vocabulary.

### 3.3 AST Node Inventory

**AST** (abstract syntax tree) — the structured output of parsing. The node inventory is fixed by the swarm specifications; each node has a fixed arity and fixed semantics:

| Node | Arity / fields | Semantics |
|---|---|---|
| Declare(name, type) | 2 | introduces a named entity of the given type into scope |
| Assign(dest, value) | 2 | stores a constant value into a named destination |
| Op(op, left, right, dest) | 4 | applies a binary arithmetic operation, result into dest |
| Compare(left, relation, right) | 3 | evaluates a RELATION between two operands, yielding a condition truth value |
| If(condition, then, else) | 3 | executes the then-branch when the condition holds, else-branch otherwise |
| While(condition, body) | 2 | repeatedly executes the body while the condition holds |
| Call(name, args) | 2 (args a list) | invokes a named procedure with an argument list |
| Return(value) | 1 | yields a value from the current procedure |
| Halt() | 0 | terminates execution immediately |

- FR-LANG-08: JStar-C must represent parsed programs using exactly the nine AST node types in the table above, each with the stated arity and field semantics; no additional node types may appear in the AST of a valid program.
- FR-LANG-09: JStar-C must preserve source position (line, column) on every AST node, inherited from the first token of the sentence that produced it.

### 3.4 Kernel-Level Construct Remediation

The existing `jstar_c.c` prototype lacks two constructs that the AetherOS kernel sources require: indexed arrays (`array`) and counted iteration (`for...from...to`). Their absence is a known defect; this document remediates it normatively.

- FR-LANG-10: JStar-C must support the array declaration construct (`array`), defining a fixed-size contiguous region of elements addressable by integer index, with capacity declared at declaration time.
- FR-LANG-11: JStar-C must support the counted-loop construct `for...from...to`, defining While-equivalent iteration over a closed integer range with a defined loop variable, lower bound, and upper bound.
- FR-LANG-12: The array and for-loop constructs must integrate into the existing quark/spark model: each must have at least one declared spark pattern (§5.1) and exactly one AST representation drawn from §3.3 or a normatively declared extension node deferred to AMEND-001 in the Design Specification volume.
- FR-LANG-13: JStar-C must not change the semantics of any frozen construct (§3.1–§3.3) in order to accommodate the remediated constructs.

### 3.5 FR-LANG Summary

| ID | Requirement (abbreviated) | Mandatory |
|---|---|---|
| FR-LANG-01 | Exactly three quark kinds | yes |
| FR-LANG-02 | Spark is sole unit of meaning; unmatched sentences rejected | yes |
| FR-LANG-03 | Period-terminated sentence definition | yes |
| FR-LANG-04 | Parenthesized comments fully ignored | yes |
| FR-LANG-05 | Exactly eight token classes | yes |
| FR-LANG-06 | Atomic compound RELATION phrases | yes |
| FR-LANG-07 | NUMBER tokens carry resolved values | yes |
| FR-LANG-08 | Exactly nine AST node types, fixed arity | yes |
| FR-LANG-09 | Source position on every AST node | yes |
| FR-LANG-10 | Array construct supported | yes |
| FR-LANG-11 | `for...from...to` construct supported | yes |
| FR-LANG-12 | Remediated constructs integrated into spark/AST model | yes |
| FR-LANG-13 | Frozen semantics unchanged by remediation | yes |

---

## 4. Lexer and Tokenizer Requirements

The **lexer** (tokenizer) is the pipeline stage that converts Jasterish source text into the token stream. All lexical behavior must be defined; no input may produce undefined behavior.

### 4.1 Functional Requirements

- FR-LEX-01: JStar-C must tokenize Jasterish source as period-terminated English sentences, classifying every token into exactly one of the eight classes defined in §3.2.
- FR-LEX-02: JStar-C must resolve compound RELATION phrases atomically during tokenization, before any pattern recognition.
- FR-LEX-03: JStar-C must strip parenthesized comments during tokenization such that they produce no tokens and do not disturb the line/column accounting of surrounding text.
- FR-LEX-04: JStar-C must emit the token stream as JSON, with every token record carrying its class, surface form, resolved value where applicable, line, and column.
- FR-LEX-05: Token classification must be total over the source character stream: every non-whitespace, non-comment input must yield a classified token or an error token.

### 4.2 Malformed-Input Hardening

The prototype silently skips unrecognized input. That behavior is prohibited in JStar-C.

- FR-LEX-06: JStar-C must never silently skip input. Every byte of source must be accounted for in the token stream or in a diagnostic.
- FR-LEX-07: On unrecognized or malformed input, JStar-C must emit an error token carrying the exact line and column of the offending input, plus a diagnostic message; lexing must continue after the offending unit where a resumption point exists.
- FR-LEX-08: JStar-C must enforce the declared input capacity of 262144 bytes (§6.2): source exceeding the declared capacity must be rejected with an exact diagnostic naming the capacity and the observed size, never truncated silently.
- FR-LEX-09: JStar-C must define behavior for unterminated sentences (end of input without a period): the partial sentence must produce an error token at its start position and must not enter pattern recognition.

### 4.3 Externalized, Versioned Vocabulary

**Vocabulary table** — the machine-readable mapping from surface words to token classes and values (verbs, prepositions, number words, type words, relation phrases, condition words).

- FR-LEX-10: JStar-C must load its vocabulary from an externalized, versioned vocabulary table; no recognized word may be defined solely in compiled-in source.
- FR-LEX-11: The vocabulary table must carry an explicit format version; JStar-C must reject a vocabulary table whose version it does not support, with a diagnostic naming the encountered and supported versions.
- FR-LEX-12: The vocabulary version used for a compilation must be recorded in the compilation's provenance data.

### 4.4 Determinism

- FR-LEX-13: For identical source input and identical vocabulary table version, JStar-C must produce a byte-identical token stream JSON, independent of host platform, locale, timezone, or environment.
- FR-LEX-14: Tokenization must not consult locale-sensitive functions, wall-clock time, or host-specific character classification; all classification must derive from the versioned vocabulary and fixed byte-level rules.

### 4.5 FR-LEX Summary

| ID | Requirement (abbreviated) | Mandatory |
|---|---|---|
| FR-LEX-01 | Total classification into eight classes | yes |
| FR-LEX-02 | Atomic compound phrase resolution | yes |
| FR-LEX-03 | Comment stripping with intact position accounting | yes |
| FR-LEX-04 | JSON token stream with line/column | yes |
| FR-LEX-05 | Total coverage: classified or error token | yes |
| FR-LEX-06 | No silent skipping of input | yes |
| FR-LEX-07 | Error tokens with exact position, continued lexing | yes |
| FR-LEX-08 | 262144-byte input capacity enforced with diagnostic | yes |
| FR-LEX-09 | Unterminated sentence produces error token | yes |
| FR-LEX-10 | Externalized versioned vocabulary table | yes |
| FR-LEX-11 | Unsupported vocabulary version rejected | yes |
| FR-LEX-12 | Vocabulary version recorded in provenance | yes |
| FR-LEX-13 | Byte-identical token stream across platform/locale/timezone | yes |
| FR-LEX-14 | No locale/time/host-sensitive classification | yes |

---

## 5. Parser Requirements

The **parser** is the pipeline stage that recognizes spark patterns in the token stream and constructs the AST. Pattern recognition is closed: only declared patterns are valid.

### 5.1 Spark-Pattern Recognition

The normative minimum spark pattern set is fixed by the swarm PARSER_TASK specification and restated here bindingly:

| # | Spark pattern (token sequence) | AST produced |
|---|---|---|
| SP-1 | DECL + NOUN + `is` + TYPE | Declare(name, type) |
| SP-2 | `Store` + NUMBER + `into` + NOUN | Assign(dest, value) |
| SP-3 | `Add` + NUMBER + `to` + NOUN | Op(add, noun, number, noun) |
| SP-4 | `If` + NOUN + `is` + RELATION + NUMBER + `then` + `halt` | If(Compare(noun, relation, number), Halt(), —) |
| SP-5 | array declaration head (remediation, §3.4) | Declare with array capacity |
| SP-6 | `for` … `from` … `to` loop head (remediation, §3.4) | While-equivalent counted iteration per FR-LANG-11 |

Note (SP-5/SP-6): this specification mandates the capability and its constraints — counted iteration is While-equivalent, and arrays are capacity-declared contiguous regions; the concrete English surface syntax and any AST extension node for these remediated constructs are explicitly deferred to AMEND-001 in the Design Specification volume.

- FR-PARSE-01: JStar-C must recognize every spark pattern in the normative table above and produce exactly the AST node(s) stated for each.
- FR-PARSE-02: JStar-C must reject any token sequence that matches no declared spark pattern; pattern matching must be exact on token class, keyword identity, and arity.
- FR-PARSE-03: The spark pattern set must be extensible only through the versioned vocabulary/pattern tables (§4.3); adding a pattern must increment the table version and must not alter the meaning of existing patterns.
- FR-PARSE-04: Where two patterns could match a token prefix, JStar-C must apply a single documented longest-match rule; ambiguity resolution must be deterministic and identical across runs.

### 5.2 AST Construction and Well-Formedness

- FR-PARSE-05: JStar-C must construct AST nodes in deterministic source order; identical token streams must yield structurally identical ASTs, including node ordering and position annotations.
- FR-PARSE-06: JStar-C must enforce declared-before-use: a NOUN referenced by any spark must have been introduced by a preceding Declare; violations must be rejected with the position of the first offending use.
- FR-PARSE-07: JStar-C must enforce single-definition: a name declared twice in the same scope must be rejected with positions of both declarations.
- FR-PARSE-08: JStar-C must enforce parse-level type consistency: operands of Op and Compare must be type-compatible with the operation per the versioned vocabulary's type rules; violations must be rejected with the sentence position.
- FR-PARSE-09: JStar-C must preserve token line/column positions on every constructed AST node per FR-LANG-09.

### 5.3 Error Handling and Recovery

The prototype's failure behavior (silent acceptance, panic paths, or undiagnosed skipping) is prohibited.

- FR-PARSE-10: Every parse error must carry the sentence's line and column and the list of expected spark patterns that could have matched at the failure point.
- FR-PARSE-11: JStar-C must contain no panic paths in parsing; all malformed input must terminate in a structured diagnostic, not a crash, abort, or undefined exit.
- FR-PARSE-12: On a failed sentence, JStar-C must recover by skipping to the next period (skip-to-period recovery) and must resume parsing at the following sentence; recovery must never silently accept the failed sentence or any fragment of it into the AST.
- FR-PARSE-13: If any parse error has been emitted, JStar-C must not proceed to IR generation for the affected program; a compilation containing parse errors must fail as a whole while still reporting all recoverable errors found.

### 5.4 FR-PARSE Summary

| ID | Requirement (abbreviated) | Mandatory |
|---|---|---|
| FR-PARSE-01 | Recognize all normative spark patterns | yes |
| FR-PARSE-02 | Exact, closed pattern matching; reject unmatched | yes |
| FR-PARSE-03 | Extensibility only via versioned tables | yes |
| FR-PARSE-04 | Deterministic documented longest-match rule | yes |
| FR-PARSE-05 | Deterministic AST construction order | yes |
| FR-PARSE-06 | Declared-before-use enforced | yes |
| FR-PARSE-07 | Single-definition enforced | yes |
| FR-PARSE-08 | Parse-level type consistency enforced | yes |
| FR-PARSE-09 | Position preservation on AST nodes | yes |
| FR-PARSE-10 | Errors carry line/column + expected-pattern list | yes |
| FR-PARSE-11 | No panic paths | yes |
| FR-PARSE-12 | Skip-to-period recovery without silent acceptance | yes |
| FR-PARSE-13 | Any parse error fails the compilation | yes |

---

## 6. Semantic Analysis and Validation Layer Requirements

The **validation layer** is the pipeline stage that establishes static guarantees about a parsed program before lowering. Per the 2026-05-28 spec-triad lineage, validation is a program-level concern: the layer validates programs, not individual compiler IR instructions.

### 6.1 Validation-Layer Architecture

- FR-SEM-01: JStar-C must implement validation as a distinct, separately invocable runtime module that accepts a complete parsed program and accepts or rejects it as a whole.
- FR-SEM-02: The validation module must operate on programs (AST plus symbol context), not on every emitted IR instruction; per-instruction validation is out of scope for this layer.
- FR-SEM-03: Validation passes must execute in a fixed, documented, deterministic order; identical programs must produce identical validation outcomes and identical diagnostic sequences.
- FR-SEM-04: Validation must be invocable independently of code generation, such that a program can be validated without being compiled.

### 6.2 Declared Buffer Capacities

**Capacity declaration** — a compile-time-stated maximum size for a compiler global region, checked against actual use. The corrected buffer lineage from `compiler.jstr` is normative; capacities are declared and checked, never assumed:

| Region | Capacity (bytes) | Role |
|---|---|---|
| input | 262144 | source input buffer |
| output | 262144 | emitted-binary output buffer |
| text | 262144 | emitted `.text` section buffer |
| datasec | 2097152 | string-literal / `.data` section buffer |

- FR-SEM-05: JStar-C must declare the four capacities in the table above as normative limits and must check every use of the corresponding region against its declared capacity.
- FR-SEM-06: A program or compilation exceeding any declared capacity must be rejected with an exact diagnostic naming the region, the declared capacity, and the required size; silent truncation or overflow is prohibited.
- FR-SEM-07: JStar-C must not treat any capacity as an implementation assumption; the capacity values must appear in a versioned declaration consumable by verification tooling.

### 6.3 Static Guarantees

- FR-SEM-08: JStar-C must construct the symbol table deterministically: iteration order, lookup results, and diagnostics derived from the symbol table must be identical across runs, platforms, and environments.
- FR-SEM-09: JStar-C must validate array accesses against declared array capacities at validation time wherever the index is statically known; statically unprovable accesses must be flagged with a defined diagnostic class.
- FR-SEM-10: JStar-C must validate `for...from...to` loop bounds: the lower bound, upper bound, and loop variable must be defined integer-valued entities, and a statically known empty or inverted range must produce a diagnostic.
- FR-SEM-11: JStar-C must reject programs that exceed declared capacities (regions, symbol count, nesting depth where declared) before any IR is emitted; partial output for a rejected program is prohibited.
- FR-SEM-12: Every validation diagnostic must carry a source position (line, column) traceable through AST node positions to the originating sentence.

### 6.4 FR-SEM Summary

| ID | Requirement (abbreviated) | Mandatory |
|---|---|---|
| FR-SEM-01 | Validation as distinct runtime module | yes |
| FR-SEM-02 | Program-level, not per-IR-instruction, validation | yes |
| FR-SEM-03 | Fixed deterministic pass order | yes |
| FR-SEM-04 | Validation invocable without codegen | yes |
| FR-SEM-05 | Four buffer capacities declared and checked | yes |
| FR-SEM-06 | Capacity overflow rejected with exact diagnostic | yes |
| FR-SEM-07 | Capacities in versioned machine-consumable declaration | yes |
| FR-SEM-08 | Deterministic symbol table | yes |
| FR-SEM-09 | Static array-bounds validation | yes |
| FR-SEM-10 | For-loop bound validation | yes |
| FR-SEM-11 | Capacity-exceeding programs rejected pre-IR | yes |
| FR-SEM-12 | Diagnostics carry source positions | yes |

---

## 7. Intermediate Representation Requirements

The **IR** (intermediate representation) is the defined form between validated AST and target code generation. The IR is a specified artifact of the compiler, not an implementation accident.

### 7.1 IR Properties

- FR-IR-01: JStar-C must define the IR as an explicit, documented format with a fixed instruction inventory; every IR construct must appear in the format definition.
- FR-IR-02: The IR must be serializable: JStar-C must be able to write an IR instance to a byte stream and re-read it into a structurally identical instance (round-trip fidelity).
- FR-IR-03: The IR format must be versioned; the format version must be embedded in every serialized IR instance, and readers must reject unsupported versions with a diagnostic naming encountered and supported versions.
- FR-IR-04: JStar-C must provide a canonical text dump of the IR suitable for golden-diff testing: identical programs must produce byte-identical text dumps across runs, platforms, locales, and timezones.
- FR-IR-05: The IR must be target-independent: no target-specific information (register names, instruction encodings, ABI details) may appear in the IR.

### 7.2 Lowering Rules

**Lowering** — the translation of a validated AST into IR, and of IR into target code.

- FR-IR-06: Every AST node type defined in §3.3, including the remediated array and `for...from...to` constructs, must have exactly one lowering path into IR per target; alternative or conditional lowering paths for the same node type on the same target are prohibited.
- FR-IR-07: Target code generation must not mutate the IR; the IR instance produced by lowering must remain unchanged by every backend.
- FR-IR-08: Lowering must be deterministic: identical validated ASTs must produce byte-identical serialized IR instances on any host.
- FR-IR-09: The mapping from AST node type to IR lowering path must be documented in a form consumable by verification tooling, such that coverage of every AST node type can be checked mechanically.
- FR-IR-10: IR instances must carry provenance sufficient to trace each IR instruction to the AST node and source position it was lowered from.

### 7.3 FR-IR Summary

| ID | Requirement (abbreviated) | Mandatory |
|---|---|---|
| FR-IR-01 | Defined IR with fixed instruction inventory | yes |
| FR-IR-02 | Serializable with round-trip fidelity | yes |
| FR-IR-03 | Versioned format; unsupported versions rejected | yes |
| FR-IR-04 | Canonical byte-identical text dump for golden-diff | yes |
| FR-IR-05 | Target-independent IR | yes |
| FR-IR-06 | Exactly one lowering path per AST node per target | yes |
| FR-IR-07 | No target-dependent IR mutation | yes |
| FR-IR-08 | Deterministic lowering | yes |
| FR-IR-09 | Machine-checkable AST-to-lowering coverage map | yes |
| FR-IR-10 | IR-to-source provenance on every instruction | yes |

---

## 8. x86-64 Code Generation Requirements

This section fixes the mandatory properties of the x86-64 machine-code emission path. The encoding subset below is the minimum required instruction repertoire, restated normatively from the swarm CODEGEN_TASK specification; every emitted byte stream must be disassemblable by `objdump -d` against these exact encodings.

### 8.1 Encoding Requirements (Minimal Fixed Subset)

- FR-X64-01: The compiler must emit the mov-immediate-to-register operation for a 32-bit immediate into any 64-bit general register exclusively as the byte sequence `48 C7 C0+reg imm32le` (REX.W prefix, opcode `C7 /0`, ModRM `11 000 reg`, little-endian imm32).
- FR-X64-02: The compiler must emit the add-immediate operation for a 32-bit immediate to any 64-bit general register exclusively as `48 81 C0+reg imm32le` (REX.W, opcode `81 /0`, ModRM `11 000 reg`).
- FR-X64-03: The compiler must emit the subtract-immediate operation for a 32-bit immediate from any 64-bit general register exclusively as `48 81 E8+reg imm32le` (REX.W, opcode `81 /5`, ModRM `11 101 reg`).
- FR-X64-04: The compiler must emit the register-compare operation `cmp dst, src` exclusively as `48 39 C0+src*8+dst` (REX.W, opcode `39 /r`, ModRM `11 src dst`), with flags consumption defined for the subsequent conditional branch.
- FR-X64-05: The compiler must emit every conditional jump-on-equal as `0F 84 rel32le`, with the rel32 displacement computed from the end of the instruction to the resolved target address at fixup time.
- FR-X64-06: The compiler must emit every unconditional jump as `E9 rel32le`, with the rel32 displacement computed identically to FR-X64-05.
- FR-X64-07: The compiler must emit function return exclusively as `C3` (near `ret`).
- FR-X64-08: The compiler must emit program halt exclusively as `F4` (`hlt`).
- FR-X64-09: The compiler must not emit any instruction encoding outside its documented encoding table for the minimal subset; every encoding added beyond FR-X64-01 through FR-X64-08 must be recorded in the same byte-format form before first use.
- FR-X64-10: The compiler must resolve all branch and data displacements through an explicit fixup table populated at emission time; byte-pattern rescanning of emitted code to locate patch sites must not occur.

Table 8-1: Minimal x86-64 instruction encodings (fixed byte formats).

| Construct | Byte format (hex) | Fields | Verification vector |
|---|---|---|---|
| mov imm32 → reg | `48 C7 C0+reg imm32le` | REX.W=`48`; opcode=`C7`; ModRM reg field=`000`; reg code 0–7 in low ModRM bits | "Store five into the counter." → `48 C7 C3 05 00 00 00` (RBX) |
| add imm32 → reg | `48 81 C0+reg imm32le` | REX.W=`48`; opcode=`81`; ModRM reg field=`000` | "Add ten to the counter." → `48 81 C3 0A 00 00 00` (RBX) |
| sub imm32 → reg | `48 81 E8+reg imm32le` | REX.W=`48`; opcode=`81`; ModRM reg field=`101` | subtract 1 from RCX → `48 81 E9 01 00 00 00` |
| cmp dst, src | `48 39 C0+src*8+dst` | REX.W=`48`; opcode=`39`; ModRM=`11 src dst` | cmp RBX, RAX → `48 39 C3` |
| je rel32 | `0F 84 rel32le` | opcode=`0F 84`; displacement little-endian | forward branch over 4 bytes → `0F 84 04 00 00 00` |
| jmp rel32 | `E9 rel32le` | opcode=`E9`; displacement little-endian | back-branch −5 → `E9 FB FF FF FF` |
| ret | `C3` | — | function epilogue marker |
| hlt | `F4` | — | "then halt." terminal |

### 8.2 Register Model

- FR-X64-11: The compiler must assign the loop/counter variable role to RBX, the result role to RCX, the output role to RDX, and the accumulator role to RAX, with RAX exposed to source programs through the `it` pronoun idiom.
- FR-X64-12: The compiler must document its register allocation model as a versioned table; any generalization beyond the four fixed roles of FR-X64-11 must preserve those role assignments for the minimal subset.
- FR-X64-13: The compiler must define register-pressure policy and spill rules at the requirement level: when live values exceed allocatable registers, spilled values must be stack-resident with deterministic slot assignment independent of host platform.
- FR-X64-14: The compiler must prefer 8-bit and 16-bit immediates and narrow operations in control-flow and bounds-check paths wherever the value range permits, per the Efficiency Mandate smallest-safe-integer rule, and must record the chosen width in the emission log.

Table 8-2: Fixed register map (minimal subset).

| Register | Role | Source-level exposure |
|---|---|---|
| RAX | Accumulator | `it` pronoun; parameter and return carrier |
| RBX | Counter | loop/iteration variable |
| RCX | Result | computed-result staging |
| RDX | Output | output staging |
| RSP/RBP | Stack discipline | frame and stack maintenance only; never assigned to source variables |

### 8.3 Correctness Requirements

- FR-X64-15: Every instruction stream the compiler emits must disassemble cleanly with `objdump -d -m i386:x86-64` with zero `bad` or undefined encodings.
- FR-X64-16: Every language construct in the minimal subset must have a golden-vector test pairing a source sentence with its exact expected byte sequence per Table 8-1.
- FR-X64-17: The compiler must emit position-independent fixup records (offset, width, target symbol) for every relocation it performs, and the set of fixups must be reproducible across identical runs.
- FR-X64-18: The compiler must not emit cumulative re-patching of data fixups per function entry (the 2026-05-28 Rust-lineage defect class); each fixup must be applied exactly once.
- FR-X64-19: The x86-64 backend must target the System V AMD64 register conventions (RAX accumulator; RDI/RSI/RDX/RCX/R8/R9 argument order; RBP frame; RSP stack) at every boundary where emitted code interfaces with C objects.

---

## 9. AArch64 Backend Requirements

This section fixes the mandatory properties of the aarch64 emission path required to reproduce the existing JMK aarch64 kernel build and smoke test from identical Jasterish sources.

### 9.1 Target Requirements

- FR-A64-01: The compiler must provide an aarch64 emission backend selectable via `--target aarch64`, capable of compiling the same Jasterish source set accepted by the x86-64 backend.
- FR-A64-02: The compiler must emit aarch64 images with virtual-address base `0x40080000`, matching the QEMU `virt` machine `-kernel` load address; this compiler-emitted base must not be conflated with any linker-script address of Table 9-1.
- FR-A64-03: The compiler must support preservation of the DTB physical pointer received in register `x0` at `_start`, including emission of the callee-saved `x19` save sequence used by the kernel boot path.
- FR-A64-04: The compiler must correctly compile the EL2→EL1 exception-level drop sequence (HCR_EL2 RW=1 configuration, SPSR_EL2=`0x3C5`, ELR_EL2 address materialization, `eret`) expressed through inline assembly in source.
- FR-A64-05: The compiler must support the kernel's FDT-driven driver registry pattern: FDT parse for `arm,pl011`, `arm,cortex-a15-gic`, and `arm,gic-v3` compatibles with a hard-coded QEMU `virt` fallback when the DTB is absent or invalid.
- FR-A64-06: The compiler must adhere to AAPCS64 at every boundary where emitted code interfaces with C objects, including argument registers `x0`–`x7`, return in `x0`, and callee-saved preservation of `x19`–`x28`.
- FR-A64-07: The compiler must treat the aarch64 boot stack as a fixed physical address (`0x41000000` lineage) when so directed by source, consistent with the all-locals-stack-resident discipline of Section 11.

### 9.2 Address Layout (Non-Conflation Requirement)

- FR-A64-08: The compiler must keep compiler-emitted virtual-address bases distinct from staged kernel linker-script addresses; no requirement, diagnostic, or provenance record may substitute one for the other.

Table 9-1: Address layout — compiler-emitted bases vs. staged kernel linker scripts.

| Address role | x86_64 | aarch64 | Origin |
|---|---|---|---|
| Compiler-emitted vaddr base | `0x400000` | `0x40080000` (QEMU `virt` `-kernel` load addr) | Compiler emission contract |
| Staged physical load base | `0x00100000` | — (single higher-half origin) | Kernel linker scripts (staged) |
| Staged higher-half runtime base | `0xFFFFFFFF80000000` | `0xFFFF800000000000` | Kernel linker scripts (staged) |
| Boot stack (fixed physical) | per linker script (64 KiB) | `0x41000000` | Kernel boot sources |
| Boot protocol marker | Multiboot2 header, magic `0xE85250D6`, emitted in-image by `_start` | DTB pointer in `x0` at entry | Kernel boot sources |
| Entry symbol | `_start` | `_start` | Both backends |

### 9.3 Parity Requirements

- FR-A64-09: The compiler must enforce the feature-parity policy between x86-64 and aarch64 backends: identical Jasterish source must compile on both targets with equivalent semantics, differing only in emitted bytes and target-mandated sequences.
- FR-A64-10: The compiler must record per-architecture provenance for every emitted artifact, including target triple, so that x86-64 and aarch64 baselines are independently comparable.
- FR-A64-11: Any language construct supported on one backend must be supported on the other or be explicitly registered as an architecture-scoped construct (inline assembly, port I/O) in a versioned parity ledger; silent per-arch divergence must not occur.
- FR-A64-12: The aarch64 backend must reproduce the existing JMK aarch64 build-and-smoke outcome (serial `BOOT` marker and `JMK>` shell prompt under `qemu-system-aarch64 -machine virt -cpu cortex-a72`) as an integration acceptance vector.

---

## 10. ELF64 Binary Emission Requirements

This section fixes the structural, layout, and reproducibility properties of every ELF64 file the compiler emits. Each requirement remediates a proven defect of the Rust-jstar lineage (`apps/src/jstar/linker.rs`) and the existing `jstar_c.c` comparison path; the mapping is fixed in Table 10-2.

### 10.1 Structural Compliance

- FR-ELF-01: Every emitted binary must be a spec-valid ELF64 file with a non-zero section-header-table offset (`e_shoff > 0`).
- FR-ELF-02: Every emitted binary must carry a section header table with `e_shnum ≥ 3`, comprising at minimum the null section, `.text`, and `.data`, with `e_shentsize = 64` and a valid `e_shstrndx` naming all sections.
- FR-ELF-03: Every emitted binary must carry a complete, non-truncated `.data` section containing all globals and string literals referenced by the program; `.data` sizes recorded in section and program headers must equal the emitted byte count exactly.
- FR-ELF-04: Every emitted binary must use per-segment `PT_LOAD` program headers with correct flags — `.text` as `PF_R|PF_X`, `.data` as `PF_R|PF_W` — and must not emit a single `PF_R|PF_W|PF_X` segment.
- FR-ELF-05: Every emitted binary must be fully static: `ET_EXEC`, no `PT_INTERP`, no `.interp` section, no dynamic dependencies.
- FR-ELF-06: Every emitted binary must declare `e_machine = EM_X86_64` or `EM_AARCH64` matching the selected target, and `e_entry` equal to the linked virtual address of `_start`; a hardcoded entry offset (the `0x400078` defect class) must not occur.
- FR-ELF-07: Every emitted binary must use 4096-byte (`0x1000`) alignment for all loadable segments (`p_align`), with file offsets and virtual addresses congruent modulo `p_align`.
- FR-ELF-08: Every emitted binary must be directly consumable by the documented header-strip flow without post-processing correction; the strip offset must be derived from the emitted program-header table as 64 + 56 × `e_phnum` bytes (the 64-byte ELF64 header plus all 56-byte program headers preceding `.text`/`.data`), so that W^X multi-`PT_LOAD` images per FR-ELF-04 strip correctly. The legacy single-program-header 120-byte constant (`dd skip=$((64+56))`) is permitted only as a documented transitional exception for the single-`PT_LOAD` minimal layout.
- FR-ELF-09: The compiler must support emitting the x86_64 Multiboot2-capable image in which `_start` embeds the in-image Multiboot2 header (magic `0xE85250D6`, architecture 0, header length 24, computed checksum) within the first 32 KiB of the image.
- FR-ELF-10: The compiler must emit relocation/fixup records as explicit 8-byte relocation slots resolved at emission; the emitted image must contain no residual unpatched fixup.

Table 10-1: ELF64 header field requirements.

| Field | Required value | Defect remediated |
|---|---|---|
| `e_type` | `ET_EXEC` | — |
| `e_machine` | `EM_X86_64` or `EM_AARCH64`, matching the selected target | arm64-only build variance |
| `e_entry` | vaddr of `_start`; never a hardcoded constant | hardcoded `0x400078` |
| `e_shoff` | `> 0`, pointing to a valid section header table | `e_shoff = 0` (no section headers) |
| `e_shentsize` | 64 | zeroed in defective lineage |
| `e_shnum` | `≥ 3` (null + `.text` + `.data` minimum) | zeroed in defective lineage |
| `e_shstrndx` | valid index naming all sections | zeroed in defective lineage |
| `PT_LOAD` flags | per-segment; no single RWX segment | single `PF_R|PF_W|PF_X` segment |
| `p_align` | `0x1000` | — |
| `PT_INTERP` / `.interp` | absent (static only) | — |

### 10.2 Post-Emit Self-Validation

- FR-ELF-11: The compiler must parse its own emitted output header after every emission and must fail the build with a non-zero exit status if any requirement of FR-ELF-01 through FR-ELF-07 is violated (assert `e_shoff != 0` as the minimum check).
- FR-ELF-12: The self-validation of FR-ELF-11 must run on every build, including bootstrap-stage builds, and its verdict must appear in the build record.
- FR-ELF-13: The compiler must reject as an emission defect any output whose size deviates from the golden size envelope for its stage (the extreme 3.8M / 67K / 0B / 146B variance class), unless the deviation is declared and gated per Section 12.

### 10.3 Reproducibility

- FR-ELF-14: Identical source input with an identical toolchain must produce byte-identical ELF64 output on any host, in any locale, at any time.
- FR-ELF-15: Emitted binaries must contain no timestamps, no host paths, no host identifiers, and no uninitialized padding bytes; every padding byte must be zero.
- FR-ELF-16: Every emitted artifact must be registrable in the provenance-manifest enumeration scheme of FR-DEN-07 (per-binary `name`, `sha256`, `size_bytes`, `file_type`) and must be comparable by the drift gate under the exit-code contract of FR-DEN-08.

Table 10-2: Defect-remediation mapping (each defect class must not recur).

| # | Proven defect (lineage) | Remediating requirement(s) |
|---|---|---|
| D1 | Missing section headers (`e_shoff = 0`, zeroed shdr fields) | FR-ELF-01, FR-ELF-02, FR-ELF-11 |
| D2 | Truncated or absent `.data` for globals/string literals | FR-ELF-03 |
| D3 | Extreme size variance between "successful" builds (3.8M vs 67K vs 0B/146B outputs) | FR-ELF-13, FR-HOST-08, FR-HOST-11 |
| D4 | No provenance linkage between binaries and sources | FR-ELF-16, FR-HOST-12 |
| D5 | No multi-architecture discipline (arm64-only build variance) | FR-A64-09, FR-A64-10, FR-ELF-06 |
| D6 | No stripping / minimal-footprint / smallest-safe-integer discipline in emitted code | FR-X64-14, FR-RT-08 |
| D7 | Single RWX `PT_LOAD` segment | FR-ELF-04 |
| D8 | Hardcoded entry `0x400078` | FR-ELF-06 |
| D9 | Cumulative `data_fixups` re-patching per function entry | FR-X64-18 |
| D10 | Use-before-declaration (`string_data_len`) causing 1-byte stage divergence | FR-ELF-14, FR-HOST-09 |

---

## 11. Freestanding Runtime Requirements

This section fixes the freestanding discipline binding both the compiler and the programs it emits for kernel-context use.

### 11.1 Freestanding Discipline

- FR-RT-01: The compiler and all compiled programs must link freestanding with `-nostdlib -static`; no libc dependency may appear anywhere in the build or link chain.
- FR-RT-02: Neither the compiler nor any emitted program must use `float` or `double` in any path; all arithmetic must be integer, and fractional quantities must use the declared fixed-point discipline.
- FR-RT-03: All local variables in emitted programs must be stack-resident; the compiler must honor the declared memory budgets of Table 11-1 without exceeding them.
- FR-RT-04: The compiler must support the kernel-mode language surface: inline assembly passthrough, direct register read/write (`store … into register`, `store register … into`), port I/O verbs, labels and jumps, and the `it`-register calling idiom for parameter and return passing.
- FR-RT-05: The compiler's emitted kernel-mode code must require no dynamic allocation beyond the declared static capacities; all buffers must be fixed-capacity and capacity-checked.
- FR-RT-06: The compiler must declare and check fixed capacities for its own buffers at the lineage values: input source buffer 262144 bytes, output ELF buffer 262144 bytes, emitted `.text` buffer 262144 bytes, and `.data` string-literal buffer 2097152 bytes.

Table 11-1: Declared memory budgets (freestanding images).

| Region | Budget | Binding |
|---|---|---|
| Kernel stack | 64 KiB | Must not be exceeded by emitted stack frames |
| Per-process stack | 8 KiB (8192 B) | Must be honored by per-process code |
| Kernel heap | 1 MiB (1048576 B) | Static allocation only within budget |
| Compiler input buffer | 262144 B | Capacity-declared and checked |
| Compiler output buffer | 262144 B | Capacity-declared and checked |
| Compiler `.text` buffer | 262144 B | Capacity-declared and checked |
| Compiler `.data` buffer | 2097152 B | Capacity-declared and checked |

### 11.2 Minimal Runtime Services

- FR-RT-07: The toolchain must provide a documented minimal runtime — entry stub, exit/halt service, and memory primitives — as versioned C source with a recorded hash.
- FR-RT-08: A release-minimal build profile must exist for the compiler itself, per the minimal-footprint profile mandate of FR-DEN-03, with its size documented and tracked against the golden baseline.
- FR-RT-09: The minimal runtime must define program termination semantics: top-level return from `_start` must produce the documented exit/halt sequence on each target.
- FR-RT-10: Every new Jasterish source file accepted by the toolchain must carry a compute-footprint budget declaration in its header, and the compiler must surface violations as diagnostics rather than silently accepting undeclared files.
- FR-RT-11: Buffer discipline must be enforced as capacity checks with exact diagnostics on overflow; silent truncation of any declared buffer must not occur.

---

## 12. Self-Hosting Ladder Requirements

This section fixes the staged self-hosting ladder, its preflight gates, and its golden-artifact invariants. The canonical release invariant is **jstar4 == jstar5 byte-identical**, enforced by `apps/scripts/verify_jstar_canonical_baseline.sh`.

### 12.1 Ladder Definition

- FR-HOST-01: The toolchain must define the self-host ladder as the ordered stage sequence jstar1 → jstar2 → jstar3 → jstar4 → jstar5, where jstar1 is the Rust-lineage reference bootstrap compiler compiling `compiler.jstr` and each subsequent stage is the preceding stage compiling the identical `compiler.jstr` source.
- FR-HOST-02: The toolchain must treat jstar4 == jstar5 (byte-identical SHA256) as the canonical release fixpoint invariant; any ladder re-execution in which `jstar4_sha256 != jstar5_sha256` must fail the release gate.
- FR-HOST-03: The JStar-C compiler must be able to compile the toolchain's own full `compiler.jstr` source (212,894 bytes) as its self-host source; this capability is a precondition for ladder participation.
- FR-HOST-04: Every stage output must be hashed (SHA256) and sized at production time, and each hash must be recorded against the golden manifest of Table 12-1 lineage (`apps/security/provenance/jstar-canonical-baseline.json`).

Table 12-1: Ladder stages, lineage sizes, and gates.

| Stage | Definition | Lineage size (bytes) | Gate |
|---|---|---|---|
| jstar1 | Rust-lineage reference compiler compiles `compiler.jstr` | 3,931,061 (historical artifact) | Reference only; not a promotion candidate |
| jstar2 | jstar1 compiles `compiler.jstr` | 123,259 | SHA256 + size vs golden manifest |
| jstar3 | jstar2 compiles `compiler.jstr` | 70,925 | SHA256 + size vs golden manifest; smoke tests (tokenize/compile/execute) |
| jstar4 | jstar3 compiles `compiler.jstr` | 70,925 | SHA256 + size vs golden manifest |
| jstar5 | jstar4 compiles `compiler.jstr` | 70,925 | Canonical invariant: `jstar4_sha256 == jstar5_sha256` |

### 12.2 Bootstrap Preflight

- FR-HOST-05: The preflight gate equivalent to `apps/scripts/jstar_bootstrap_check.sh` must run to a green verdict before any bootstrap attempt; a bootstrap begun without a passing preflight is invalid and its artifacts must be discarded.
- FR-HOST-06: The presence of the complete `compiler.jstr` source (212,894 bytes) must be verified as a preflight precondition; any bootstrap attempted against a partial or absent source must fail before compilation begins.
- FR-HOST-07: Live rebuilds of the ladder must be quarantined behind explicit opt-in: `RUN_FIXPOINT=1` on a Linux host only for fixpoint verification, and `ALLOW_FORENSIC_BOOTSTRAP_REBUILD=1` for forensic rebuild traces; neither path may run by default.
- FR-HOST-08: Stage promotion must follow the documented harness sequence — build the release candidate, compile `compiler.jstr` with it, compare the output data hash to the known stable value, and pass smoke tests — and any step failure must block promotion.
- FR-HOST-09: The ladder must not reintroduce the known divergence classes: cumulative per-function fixup re-patching, and use-before-declaration of `.data` length state (the 1-byte stage-divergence defect at `0x1c8b`).

### 12.3 Golden Artifacts and Drift

- FR-HOST-10: A golden SHA256 and size must be recorded per ladder stage; re-execution of the ladder from the same source and toolchain must reproduce every golden value exactly.
- FR-HOST-11: Any drift between a re-executed stage artifact and its golden value — in hash, size, or section layout — must fail the verification gate under the drift-check exit-code contract of FR-DEN-08.
- FR-HOST-12: Every stage artifact must carry provenance linkage (compiler version, target triple, source hashes, section-layout hash) sufficient to reproduce its manifest entry without recompilation.
- FR-HOST-13: The C compiler's own ladder integration must preserve the lineage stage sizes as the accepted baseline until a governed amendment (per the document's amendment procedure) records new golden values with written rationale.

---

## 13. AetherOS Kernel Integration Requirements

The compiler must be able to compile the AetherOS Jasterish Micro-Kernel (JMK) itself — the kernel's own Jasterish sources and the toolchain's self-host source — into bootable images, and must interoperate with the existing C-side build, link, and boot-verification machinery. These requirements bind the compiler to the kernel's concrete module inventory, its ABI expectations, and its emulated boot gates.

### 13.1 Kernel Source Coverage (Compilation Scope)

- FR-KERN-01: The compiler must compile the complete x86_64 kernel source set — 11 modules comprising 4 architecture modules (`arch/x86_64/boot.jstr`, `arch/x86_64/idt.jstr`, `arch/x86_64/memory_arch.jstr`, `arch/x86_64/drivers.jstr`) and 7 common modules (`common/elf.jstr`, `common/process.jstr`, `common/ipc.jstr`, `common/syscall.jstr`, `common/vfs.jstr`, `common/disk.jstr`, `common/kernel.jstr`) — into a single bootable image.
- FR-KERN-02: The compiler must compile the complete aarch64 kernel source set — 16 sources comprising 9 architecture modules (`arch/aarch64/{boot,memory_arch,uart,gic,timer,exceptions,fdt,driver_registry,hal}.jstr`, including the `fdt.jstr` DTB parser and `hal.jstr` HAL top-level) and the same 7 common modules — into a single bootable image.
- FR-KERN-03: The compiler must compile the `pixel_hal.jstr` lineage and all successor HAL profiles with no source-level special casing beyond declared board profiles.
- FR-KERN-04: The compiler must compile the toolchain's own self-host source `compiler.jstr` byte-for-byte within the self-hosting ladder, satisfying the fixpoint invariants defined in the ladder requirements.
- FR-KERN-05: The compiler must accept the kernel's concatenated-compile invocation model — first source as `--input` (containing `_start`), remaining sources via repeated `--include` — and emit one native ELF binary directly, without intermediate per-file object output.
- FR-KERN-06: The compiler must support inline assembly passthrough (`inline "..."` — `lgdt`, `msr hcr_el2`, `wfe`, `cli/hlt/sti`, `eret`, `retfq` and equivalents), direct register access (`store ... into register`, `load ... into register`), and I/O port verbs (`store ... into port`, `load from port`), which are pervasive across the kernel sources.
- FR-KERN-07: The compiler must support the full kernel-level language surface evidenced by the sources: `global` scalars and `global byte NAME SIZE` buffers, `array N name`, functions with `with long` parameters, `call f arg`, labels with `jump`, `if`/`while` blocks, arithmetic/bitwise/shift verbs, indexed `store`/`load`, `print "literal"`, and `return it`.

| Architecture | Modules | Arch sources | Common sources | Notable lineage |
|---|---|---|---|---|
| x86_64 | 11 | 4 (`boot`, `idt`, `memory_arch`, `drivers`) | 7 (`elf`, `process`, `ipc`, `syscall`, `vfs`, `disk`, `kernel`) | Multiboot2 in-image header; COM1 serial; `int 0x80` syscall stub |
| aarch64 | 16 | 9 (`boot`, `memory_arch`, `uart`, `gic`, `timer`, `exceptions`, `fdt`, `driver_registry`, `hal`) | 7 (same common set) | DTB in `x0`; FDT parser with QEMU `virt` fallback; `pixel_hal.jstr` lineage |
| Toolchain | 1 | — | — | `compiler.jstr` self-host source (bootstrap ladder input) |

### 13.2 System Interface (Defined ABI)

- FR-KERN-08: The compiler must define and document a formal ABI between compiled Jasterish and C kernel objects, covering at minimum calling convention, symbol naming, and section placement, versioned alongside the compiler.
- FR-KERN-09: The compiler must implement the in-kernel Jasterish accumulator convention — parameters and return values passed via the `it` pronoun (`Parameter c is passed in register it`; `return it`) — as the operative calling convention for compiled Jasterish functions.
- FR-KERN-10: The compiler must honor the host-side SysV AMD64 convention for x86_64 output (rax accumulator; rdi/rsi/rdx/rcx/r8/r9 arguments; rbp frame; rsp stack) and AAPCS64 adherence where compiled Jasterish interfaces with the C runtime on aarch64.
- FR-KERN-11: The compiler project must produce, as a deliverable of this work, a formal definition of the syscall register ABI: the JMK technical specification leaves exact syscall register usage undocumented, and the README's RAX=number, RDI/RSI/RDX=arguments convention via `int 0x80` is the operative contract until that formal definition is ratified.
- FR-KERN-12: The compiler must support the image-layout constants the kernel depends on: emitted vaddr base `0x400000` for x86_64 and `0x40080000` for aarch64 QEMU `virt` `-kernel` loading, higher-half runtime mappings `0xFFFFFFFF80000000` (x86_64) and `0xFFFF800000000000` (aarch64), and entry symbol `_start` on both architectures.
- FR-KERN-13: The compiler must compile the aarch64 boot path correctly end-to-end: DTB physical address received in `x0` and preserved (callee-saved `x19` discipline), FDT parsing for `arm,pl011`, `arm,cortex-a15-gic`, and `arm,gic-v3` with fallback to hard-coded QEMU `virt` defaults when the DTB is absent or invalid, and the EL2-to-EL1 drop sequence.
- FR-KERN-14: The compiler must compile the x86_64 boot path carrying the in-image Multiboot2 header (magic `0xE85250D6`, architecture 0, header length 24, computed checksum) so that the GRUB2 `grub-mkrescue` ISO path (`multiboot2 /boot/jmk`) boots compiler output unmodified.

| ABI point | Requirement level | Source of obligation |
|---|---|---|
| Calling convention | Jasterish `it` accumulator in-kernel; SysV AMD64 host-side; AAPCS64 at C runtime boundary | `boot.jstr` serial_putc lineage; self-hosting tech spec §2; FR-003 |
| Symbol naming | Defined, stable, collision-free across concatenated sources and C objects | JMK build model (`--input` + `--include`) |
| Section placement | `.text` RX / `.data` RW separation; linker-script constants honored | `arch/*/linker.ld` staged layouts |
| Syscall registers | Formally defined by this project; README RAX/RDI/RSI/RDX operative interim | JMK tech spec §3.1 gap; `jmk/README.md` |

### 13.3 Boot Verification (Emulated Acceptance)

- FR-KERN-15: The compiler's kernel output must pass QEMU boot-to-shell verification per the `scripts/smoke_aarch64.sh` pattern: `make test` on x86_64 and the aarch64 smoke flow must yield serial output containing `BOOT` (first output of `_start`) and the `JMK>` shell prompt; absence of either marker is a FAIL.
- FR-KERN-16: The compiler's output must boot under the documented QEMU invocations — `qemu-system-x86_64 -machine q35 -cpu qemu64 -m 512 -serial stdio -no-reboot -no-shutdown -kernel jmk.bin` and `qemu-system-aarch64 -machine virt -cpu cortex-a72` equivalents — with `jmk.bin` produced by the header-strip step whose skip offset is derived from the emitted program-header table (64 + 56 × `e_phnum`) per FR-ELF-08; the legacy 64-byte ELF header + single 56-byte program header form is honored only as the documented transitional exception.
- FR-KERN-17: The compiler's output must remain compatible with the GDB debug path (`make debug`, port 1234) and the GRUB2 ISO path without source modification.
- FR-KERN-18: The compiler should support board profiles beyond QEMU `virt` — Pixel/Tensor G4 stubs noted as future profiles — with each profile declared, capacity-checked, and provenance-recorded rather than hard-coded.

---

## 14. Determinism, Provenance and Drift Requirements

Non-functional requirements, house style. Governing lineage: the 8 Validated Denominators (Budget/Resource Accounting, Primitive Traceability, Origin Vault, Drift Detection), the Efficiency Mandate, and the Binary Optimization Plan phases 1–3.

### 14.1 Determinism and Provenance (Deterministic Provenance and State History)

- **Reproducible Build**: A build is reproducible when identical source, identical toolchain, and identical declared inputs produce byte-identical output artifacts on any host, measured per the `benchmark_determinism.sh` pattern of repeated clean builds with hash comparison.
- **Deterministic Measurement**: The project must ship a determinism benchmark that executes repeated clean builds from canonical state captures and reports hash equality, size variance, and build-count statistics as machine-readable results.
- **No Environmental Leakage**: Emitted binaries must contain no timestamps, no host paths, no locale- or timezone-dependent bytes, and no uninitialized padding.
- **Provenance Manifest**: Every produced artifact must embed or be accompanied by a provenance manifest per the `generate_provenance_manifest.sh` schema, carried in a `.note` section or a sidecar file; absence of provenance is a build failure, not a warning.
- **Artifact Registration**: All compiler-produced artifacts must be registered in the provenance enumeration scheme so that they are hashed and tracked alongside daemon and kernel binaries.
- **Section-Layout Hashing**: Provenance must include a section-layout hash in addition to the whole-binary SHA-256, so that structural drift is distinguishable from content drift.
- **Footprint Budget Tracking**: Each build variant must record its size, and the `minsize` versus `release` differential must be tracked as a first-class budget metric per the Contrast Differential denominator.

| Manifest field | Content | Lineage |
|---|---|---|
| `schema_version` | Manifest schema version (`"1.0"` lineage) | `generate_provenance_manifest.sh` |
| `build_date` | UTC ISO-8601 build timestamp (recorded, never embedded in binaries) | same |
| `git_commit` / `git_branch` | Source revision and branch at build time | same |
| `build_host` | Host identification string | same |
| `compiler` | Compiler identity and version line | same |
| `primitive_map_ref` | Reference to the atomic primitive dependency map | Primitive Traceability denominator |
| `binaries[]` | Per-binary `{name, sha256, size_bytes, file_type}` | same |

### 14.2 Drift Detection (Operational Detection of Deviation)

- **Drift Gate Integration**: Every build must be machine-comparable against golden baselines via `binary_drift_check.sh` integration, comparing SHA-256, size, and section layout per artifact.
- **Exit-Code Contract**: The drift check must preserve the established semantics — exit 0 for no drift, exit 1 for drift detected (including missing baselined files), exit 2 for missing baseline with instruction to establish one first.
- **Multi-Root Baselines**: Baselines must be maintained per artifact root, so compiler, kernel, and daemon trees each carry their own golden manifests.
- **Multi-Arch Baselines**: Separate baselines must exist per architecture — x86_64 at `-march=x86-64-v2` (NUC profile) and aarch64 at `-march=armv8.2-a+crc+crypto` (Orin profile) — with no cross-architecture comparison permitted.
- **Baseline Amendment**: Baseline updates must follow the consensus-before-destructive-action doctrine; a baseline change without written rationale is itself drift.
- **Variance as Defect**: Extreme size or hash variance between nominally successful builds must be treated as a defect per the Fluctuation Dynamics denominator, not tolerated as noise.

| Drift check property | Contract | Governing source |
|---|---|---|
| Compared metrics | SHA-256 + size + section layout, per binary | Binary Optimization Plan, Drift Detection hook |
| Exit 0 | No drift | `binary_drift_check.sh` |
| Exit 1 | Drift detected; missing file counts as drift | same |
| Exit 2 | Baseline missing; establish baseline first | same |
| x86_64 baseline | `-march=x86-64-v2` (NUC) | TP-HCF toolchain file |
| aarch64 baseline | `-march=armv8.2-a+crc+crypto` (Orin) | TP-HCF toolchain file |

---

## 15. Security and Post-Quantum Requirements

Non-functional requirements, house style. Security posture is inherited from the Trident security plane; post-quantum compliance is owned by the multi-layer PQC project, which this document references without prescribing algorithms.

- **No Network**: The compiler must perform no network access at build time or at runtime; all inputs are local files declared to the build.
- **Declared Inputs**: Every input to a compilation — sources, vocabulary tables, manifests, toolchain identities — must be explicitly declared and hashed; undeclared inputs are a build failure.
- **Auditable Implementation Language**: The compiler must be implemented in auditable C11 with a minimal trusted computing base and no external dependency chain beyond a freestanding-capable C toolchain.
- **Zero-Warning Policy**: The compiler must build with zero warnings under `-Wall -Wextra` on both gcc and clang; warnings are gate failures, not advisories.
- **Deterministic Security Behavior**: Security-relevant behavior (input rejection, capacity enforcement, error paths) must be deterministic and free of undefined behavior; no panic or silent-skip paths are permitted.
- **Signing Interface**: An artifact signing and attestation interface must be defined for all emitted binaries, specified at the interface level (what is signed, where the attestation lives, how verification is invoked) so that PQC standards per the Trident security plane can be adopted without requirements churn.
- **Algorithm Agnosticism**: This document must not prescribe specific PQC algorithms or parameter sets; algorithm selection is delegated to the multi-layer PQC project as the compliance counterpart.
- **Freestanding Integrity**: Kernel-context output must carry no libc dependency, preserving the `-nostdlib -static` discipline of the existing kernel build.

| Security property | Requirement | Compliance counterpart |
|---|---|---|
| Network isolation | No network at build or runtime; localhost-only inter-process exceptions belong to the telemetry plane, not the compiler | Emulated-environment doctrine |
| Input hygiene | All inputs declared and hashed | Origin Vault denominator |
| Implementation auditability | C11, minimal TCB, zero-warning `-Wall -Wextra` | Verification & CI gates (Section 17) |
| Artifact attestation | Signing/attestation interface defined, algorithms unspecified | Multi-layer PQC project |

---

## 16. Telemetry and Knowledge-Base Integration Requirements

The compiler is a first-class citizen of the AetherOS telemetry plane and the temporal knowledge graph: its compile phases emit aetherProbe telemetry, and its build actions are cataloged as temporal action events compilable to markdown notebooks.

### 16.1 Compiler Instrumentation (aetherProbe)

- FR-TELEM-01: The compiler must emit aetherProbe telemetry per the Layer 1 SDK contract — metrics per compile phase (tokenize, parse, validate, codegen, link), traces per pipeline stage (`aether_trace_begin`/`aether_trace_end`), and structured logs at levels 1–5 (`aether_log`).
- FR-TELEM-02: The compiler must publish telemetry into the SEB ring when the ring is available, using `struct seb_event` semantics with JSON payloads of at most 1024 bytes.
- FR-TELEM-03: The compiler must degrade deterministically to stderr JSONL emission when the SEB ring is unavailable, preserving event content, ordering, and schema across both transports.
- FR-TELEM-04: Telemetry emission must not perturb compilation output: artifacts produced with telemetry enabled must be byte-identical to artifacts produced with it disabled.
- FR-TELEM-05: Telemetry must use no wall-clock reads inside the compiler's deterministic paths; timestamps are supplied at the emission boundary per the probe API (`seb_now`) and excluded from output bytes.

### 16.2 Action Cataloging and Notebooks (Temporal Knowledge)

- FR-TELEM-06: Every build action — compile, bootstrap stage, drift check, gate decision — must be recorded as a temporal action event conforming to `schemas/action_event.schema.json` (one JSON object per line: `action_id`, `recorded_at`, `valid_from`, `actor`, `layer`, `action`, `target`, `summary`, `rationale`, `inputs`, `outputs`, `hashes`, `replication_steps`, `supersedes`, `source`).
- FR-TELEM-07: Action events must carry artifact hashes (`sha256:` form) and exact replication steps sufficient to reproduce the action from the catalog alone.
- FR-TELEM-08: The recorded action stream must be compilable to dated markdown notebooks via the 2026-09-07 notebook compiler (`temporal_kg notebook`), with deterministic ordering by `recorded_at` then `action_id` and a trailing provenance section containing the input JSONL hash.
- FR-TELEM-09: The telemetry interface must not preclude wave-2 dependencies — OTLP emission via the collector path and the SEB_ALERT integrity bridge — while those dependencies remain out of scope for this document's gates.

| Signal class | Granularity | Transport | Contract source |
|---|---|---|---|
| Metrics | Per compile phase (counts, sizes, durations) | SEB ring; stderr JSONL fallback | aetherProbe Layer 1 SDK (`aether_metric`) |
| Traces | Per pipeline stage (begin/end spans) | SEB ring; stderr JSONL fallback | `aether_trace_begin`/`aether_trace_end` |
| Structured logs | Levels 1–5, JSON payload ≤1024 B | SEB ring; stderr JSONL fallback | `aether_log` |
| Action events | Per build action, JSONL, schema-validated | Action catalog file | `action_event.schema.json` |
| Notebooks | Per ISO date, deterministic markdown | Notebook compiler output | `temporal_kg notebook` (2026-09-07) |

---

## 17. Verification, Testing and CI Gate Requirements

Non-functional requirements, house style under the Verification & CI group. These requirements define the minimum test matrix and the continuous-integration gates through which every compiler change must pass; stage gates G0–G5 are governed by the First-Principles Operating Doctrine.

- **Golden Vectors**: Every language construct must have golden-vector tests pinning accepted source, expected token stream, expected AST, and expected emitted bytes.
- **Unit Levels**: Tokenizer, parser, and codegen must each carry an independent unit-test level with deterministic, ordered results and no reliance on integration fixtures.
- **Integration Pipeline**: An end-to-end pipeline test must drive source through ELF emission to objdump disassembly, asserting spec-valid output and clean disassembly per construct.
- **Boot Smoke**: QEMU boot smoke tests on both x86_64 and aarch64 must execute the compiled kernel to the `JMK>` shell prompt, including the basic fork + IPC exercise mandated post-integration.
- **Determinism Re-runs**: CI must execute repeated clean builds and assert byte-identical output; the self-host ladder re-execution must reproduce recorded golden SHA-256 and sizes per stage.
- **Zero-Warning Builds**: Both gcc and clang builds must compile with zero warnings under `-Wall -Wextra`; either toolchain emitting a warning fails the gate.
- **Static Analysis**: The build must remain compatible with the OSSAR static-analysis workflow already present in engine CI, with findings triaged before merge.
- **Drift Before Merge**: The drift check must run before any merge and report exit 0 against the applicable per-root, per-arch baseline; exit 1 blocks the merge, exit 2 blocks until a baseline is established.
- **Stage Gates**: Promotion through stage gates G0–G5 must follow the First-Principles Operating Doctrine, with each gate's entry and exit criteria recorded as action events per FR-TELEM-06.
- **Honest Uncertainty**: Verification evidence must use honest uncertainty language for partial compliance; a conformance claim without cited requirement IDs and passing gate evidence is non-compliant.

| Test level | Scope | Pass criterion | Automation anchor |
|---|---|---|---|
| Golden vector | One language construct | Byte-exact match to pinned vectors | Per-construct fixture set |
| Unit — tokenizer | Token classification, compound phrases, error tokens | Deterministic stream equality | TOKENIZER_TASK lineage |
| Unit — parser | Spark-pattern recognition, AST well-formedness | Deterministic AST equality | PARSER_TASK lineage |
| Unit — codegen | Instruction encodings per backend | Exact byte-format equality | CODEGEN_TASK lineage |
| Integration | Source → ELF → objdump | Spec-valid ELF64; clean disassembly | Pipeline harness |
| Boot smoke | QEMU x86_64 + aarch64 | Serial contains `BOOT` and `JMK>` | `smoke_aarch64.sh` pattern; `make test` |
| Determinism | Repeated clean builds | Byte-identical artifacts; ladder fixpoint | `benchmark_determinism.sh` pattern |
| Drift | Baseline comparison | Exit 0 | `binary_drift_check.sh` |

| Gate | Intent (per First-Principles Operating Doctrine) | Minimum evidence |
|---|---|---|
| G0 | Problem identified and decomposed to primitives | Written problem statement; denominator mapping |
| G1 | Specification ratified before implementation | Approved requirements; spec-first confirmation |
| G2 | Implementation complete against spec | Unit levels green; zero-warning builds |
| G3 | Integration verified | Pipeline test, boot smoke on both arches |
| G4 | Determinism and provenance established | Reproducible builds; manifests; ladder fixpoint |
| G5 | Release candidate accepted | Drift exit 0; all gates green; action catalog complete |

---

## Constraints and Assumptions

- **Precondition — compiler.jstr restoration**: The self-host ladder requirements assume the full `compiler.jstr` source is present in the checkout; the current-checkout caveat recorded in `nnos/TODO.md` is honored, and ladder execution before restoration is out of scope.
- **Assumption — historical triad lineage**: The canonical governing triad documents (Validated Denominators, Efficiency-Minimal-Footprint-Layer, Binary Optimization Plan) were removed from `engine` HEAD on 2026-09-03; this document cites them as lineage references from the last pre-deletion tree, and path reconciliation is a lead-level open item if they were moved to a private location.
- **Precedent — version pinning**: Version-pin amendments to this document follow the Jaeger-1.x-style pinning precedent — pinned versions with recorded SHA-256 of upstream artifacts — applied to toolchain and gate dependencies.
- **Dependency — telemetry wave 2**: OTLP emission through the collector path and the SEB_ALERT integrity bridge are telemetry-plane wave-2 dependencies; this document requires interface compatibility only, per FR-TELEM-09.
- **Open item — syscall register ABI**: The exact syscall register ABI is undocumented by the JMK technical specification; FR-KERN-11 obligates this project to produce its formal definition, with the README RAX/RDI/RSI/RDX convention operative in the interim.
- **Constraint — emulated environment**: All verification occurs in the emulated environment (QEMU); no Docker artifacts and no hardware-board gates are introduced by this document.
- **Constraint — header-strip offset derivation**: The kernel image header-strip step must derive its skip offset from the emitted program-header table (64 + 56 × `e_phnum`) per FR-ELF-08; the legacy 120-byte constant is retained only as a documented transitional exception, preserving the existing boot flow while W^X multi-`PT_LOAD` emission (FR-ELF-04) is the normative target.
- **Assumption — language semantics frozen**: Jasterish language semantics are frozen by the 2026-09-07 swarm task specifications; kernel-integration requirements may not expand the language surface beyond the evidenced kernel constructs.

---

## Success Criteria

- **Ladder fixpoint reproduced**: The self-host ladder re-execution reproduces the accepted release invariant `jstar4 == jstar5` (byte-identical SHA-256), verified against the canonical baseline manifest.
- **Kernel boots on both architectures**: The AetherOS kernel, compiled by JStar-C, boots to the `JMK>` shell prompt under QEMU on x86_64 (`make test`) and aarch64 (smoke pattern), with serial output containing both `BOOT` and `JMK>`.
- **Zero-warning builds**: gcc and clang builds complete with zero warnings under `-Wall -Wextra`.
- **Drift exit 0**: `binary_drift_check.sh` integration reports exit 0 against the applicable per-root, per-arch baselines on every merge candidate.
- **Provenance complete**: Every emitted artifact carries the manifest schema of §14.1 (embedded `.note` or sidecar), and registration in the enumeration scheme is demonstrated.
- **Full traceability**: Every functional requirement group is traced to a governing source and a verification method, per the matrix below.

| FR / NFR group | Governing source | Verification method |
|---|---|---|
| FR-KERN (§13) | JMK Makefile and module inventory; JMK spec triad; self-hosting tech spec | QEMU boot smoke (`BOOT` + `JMK>`) on both arches; GRUB2 ISO and GDB path checks |
| Determinism & Provenance (§14) | 8 Validated Denominators (#6/#7/#8); Binary Optimization Plan phases 1–3 | Determinism benchmark re-runs; manifest schema audit; drift exit 0 |
| Security & PQC (§15) | Trident security plane; emulated-environment doctrine | Network-isolation audit of build; zero-warning gate; attestation-interface review |
| FR-TELEM (§16) | SPEC.md telemetry plane contracts; `action_event.schema.json`; notebook compiler contract | SEB/JSONL transport tests; schema validation; notebook round-trip |
| Verification & CI (§17) | First-Principles Operating Doctrine; OSSAR workflow in engine CI | Gate-by-gate evidence review G0–G5; CI run records; drift-before-merge check |

---

**End of Requirements Specification**
