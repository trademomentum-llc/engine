# Requirements Specification: NeuroDiOS — Jasterish Micro-Kernel Foundation

**Document ID:** NEURODIOS-REQ-001  
**Version:** 1.0.0  
**Date:** 2026-05-28  
**Origin:** Kimi_Agent_Jasterish Micro-Kernel Build (integrated from ~/Downloads)  
**Governing Context:** Highest-value project evolution — engine/nnos becoming NeuroDiOS, with this micro-kernel as the deterministic, Jasterish-native foundation.

---

## 1. Purpose and Scope

NeuroDiOS (Neurodivergent Neural-Link Operating System, evolving from nnos) requires a minimal, deterministic, bare-metal kernel written in Jasterish (JStar) — the system-level language in which natural language tokens compile directly to x86-64 machine code.

This Requirements Specification defines the mandatory properties of the Jasterish Micro-Kernel (JMK) as the foundational layer for NeuroDiOS. It captures the current implemented state (from the integrated Kimi sources) and the required expansion path to a fully operational kernel.

The kernel follows strict micro-kernel philosophy: minimal privileged code, with higher-level services (file systems, device drivers beyond basics, user-space daemons) running in isolated processes communicating exclusively via IPC.

---

## 2. Vision and Alignment with Persona

NeuroDiOS treats the brain as active computational substrate. The kernel must therefore be:

- **Deterministic** — Fixed inputs produce identical outputs (no hidden RNG, stable scheduling where possible, reproducible IPC).
- **Neuro-aligned** — Token-vector (morphlex/JStar) friendly data paths, low cognitive load for kernel developers using natural language syntax, support for bounded context switching and masking-prevention primitives at the process level.
- **Self-healing / Morphogenetic** — Future kernel services should be able to participate in repair cycles (detect drift, restore, optimize).
- **Post-quantum ready** — Cryptographic foundations (when added) must prefer PQC primitives.
- **Minimal trusted computing base** — True to micro-kernel design.

This work evolves the existing nnos daemon constellation (SystemIntegrityDaemon, MorphogeneticMaintainer, etc.) into user-space services running atop this kernel.

---

## 3. Current Implemented Capabilities (Baseline — Must Be Preserved and Stabilized)

From the integrated Jasterish Micro-Kernel sources:

**Core Subsystems (all in JStar):**
- Boot: Multiboot2, GDT, COM1 serial, early panic.
- Memory: Bitmap PMM, 4-level x86-64 VMM, buddy heap.
- Processes: 256 PCB slots, round-robin scheduler with preemption (PIT-driven), context switching.
- IPC: 64-byte message passing (send/receive/notify/broadcast).
- Syscalls: 11 syscalls via int 0x80 (exit, fork, yield, send/recv, sleep, getpid, puts, brk, kill, getticks).
- Drivers: 8259 PIC, PIT @1000Hz, PS/2 keyboard.
- Init/Shell: PID 1 interactive kernel shell.

**Non-Functional (Current State):**
- ~8,150 lines of Jasterish across 7 modules.
- Higher-half kernel mapping.
- Deterministic intent (natural language syntax + explicit data structures).

These form the Minimum Viable Kernel that must remain functional during expansion.

---

## 4. Functional Requirements (Expansion to Fully Operational NeuroDiOS Kernel)

### 4.1 Boot and Hardware Abstraction (Strengthen)
- FR-BOOT-01: Reliable Multiboot2 + direct framebuffer or serial console.
- FR-BOOT-02: Clean transition to long mode with identity + higher-half mappings.
- FR-BOOT-03: Early deterministic panic with register + stack dump in JStar.

### 4.2 Memory Management (Production Grade)
- FR-MEM-01: Stable physical page allocator with poisoning / guard pages where feasible.
- FR-MEM-02: Full virtual memory with user/kernel separation, copy-on-write potential, and per-process address spaces.
- FR-MEM-03: Kernel heap that can grow dynamically with proper accounting.
- FR-MEM-04: Future integration points for neuro-memory models (bounded working sets, context-switch cost tracking).

### 4.3 Process and Scheduling (Neurodivergent-Aware)
- FR-PROC-01: Robust PCB management, fork/exec semantics, proper zombie reaping.
- FR-PROC-02: Preemptive scheduler with configurable quanta + priority classes.
- FR-PROC-03: Explicit support for bounded context switching and task-limit primitives (directly supporting nnos-style burnout/masking prevention at kernel level).
- FR-PROC-04: Process hierarchy with proper signal / termination semantics.

### 4.4 IPC and Isolation (Core Micro-Kernel Contract)
- FR-IPC-01: Reliable, bounded message passing with back-pressure or queuing limits.
- FR-IPC-02: Capability-style or token-based access control for future services.
- FR-IPC-03: Notification and broadcast primitives usable by deterministic user-space daemons.

### 4.5 System Call Interface and User/Kernel Boundary
- FR-SYS-01: Clean, auditable syscall table with JStar-native wrappers.
- FR-SYS-02: Safe argument validation and copy-in/copy-out.
- FR-SYS-03: Extensible for future NeuroDiOS-specific calls (physiology feedback, context budget queries, etc.).

### 4.6 Drivers and Hardware (Minimal but Expandable)
- FR-DRV-01: Timer, interrupt controller, basic console/keyboard.
- FR-DRV-02: Clear path for user-space drivers (via IPC + IOMMU-like mechanisms when hardware supports it).

### 4.7 Determinism and Observability (Persona Alignment)
- FR-DET-01: Where scheduling is involved, provide reproducible behavior under controlled inputs.
- FR-DET-02: Kernel tracing / event logging that can feed higher-level neuro-analysis daemons.
- FR-DET-03: Build reproducibility (deterministic Jasterish compilation pipeline).

---

## 5. Non-Functional Requirements

- **Size & Complexity**: Kernel remains minimal; most policy and services live in user space.
- **Performance**: Low-overhead context switch and IPC suitable for real-time neuro feedback loops.
- **Safety**: Strong isolation between user processes and between user/kernel.
- **Evolvability**: The kernel must be incrementally replaceable or serviceable as Jasterish matures (self-hosting, better codegen, validation layer from nnos specs).
- **Integration**: Seamless hosting of the existing nnos daemons (once ported) and future NeuroDiOS user-space components.

---

## 6. Constraints and Assumptions

- Primary development language: Jasterish (JStar) via the toolchain in apps/.
- Target architecture (initial): x86-64 bare metal (QEMU/GRUB for early testing).
- The kernel will eventually host the nnos/NeuroDiOS daemon constellation as user-space processes.
- Jasterish compiler maturity (self-hosting, codegen stability, validation) is a critical dependency (see apps/ Jasterish triad).

---

## 7. Success Criteria

- The integrated Jasterish Micro-Kernel boots, runs its init shell, and demonstrates IPC + scheduling reliably on QEMU.
- Clear, documented path to host the existing nnos daemons.
- Full triad of Requirements + Design + Technical Specifications maintained and kept current.
- All new kernel work referenced from engine/nnos/ PROJECT_SUMMARY.md and TODO.md.

---

**End of Requirements Specification**

This document was created per the governing persona mandate for every new functional system/module. The Design Specification and Technical Specification for the NeuroDiOS Jasterish Micro-Kernel (and its evolution) follow in the required triad.

Next immediate work: Integration of the sources, creation of the Design + Technical specs, and heavy updates to nnos governance documents.