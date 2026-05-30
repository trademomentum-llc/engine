# Design Specification: NeuroDiOS — Jasterish Micro-Kernel Foundation

**Document ID:** NEURODIOS-DS-001  
**Version:** 1.0.0  
**Date:** 2026-05-28  
**Predecessor:** NEURODIOS-REQ-001

---

## 1. Design Philosophy

The NeuroDiOS kernel is deliberately a **true micro-kernel** implemented in Jasterish:

- The privileged kernel is minimal by design.
- Most policy, device drivers, and services live in isolated user-space processes.
- All inter-process and user/kernel communication occurs through a clean, auditable IPC mechanism.
- The natural-language nature of JStar is leveraged for readability and to reduce the cognitive distance between high-level neurodivergent system design and low-level implementation.

This directly supports the larger NeuroDiOS vision: a deterministic, self-healing, neuro-aligned operating system where the kernel itself participates in (or at least does not fight) the morphogenetic and bounded-cognition principles of the higher layers.

---

## 2. Current Architecture (from Integrated Sources)

The kernel follows a classic layered micro-kernel structure with these major components (all written in JStar):

**Boot Layer (boot.jstr)**
- Multiboot2 header and entry point
- Early serial console (COM1)
- GDT setup for long mode
- Panic / halt primitives

**Memory Layer (memory.jstr)**
- Physical Memory Manager (bitmap frame allocator)
- Virtual Memory Manager (4-level x86-64 page tables, higher-half mapping)
- Kernel heap (buddy-style allocator)

**Process & Scheduler Layer (process.jstr)**
- Process Control Block (PCB) table (256 slots)
- Round-robin scheduler with time-slice preemption
- Context switch machinery
- Process creation / termination lifecycle

**IPC Layer (ipc.jstr)**
- Synchronous and asynchronous message passing
- Bounded 64-byte messages
- Send / Receive / Notify / Broadcast primitives

**System Call Layer (syscall.jstr)**
- int 0x80 gateway
- 11 core syscalls (exit, fork, yield, IPC primitives, sleep, brk, etc.)

**Driver Layer (drivers.jstr)**
- 8259 PIC remapping
- PIT timer (1 kHz)
- PS/2 keyboard with scan code translation

**Kernel Main (kernel.jstr)**
- Ordered initialization
- Creation of the init process (PID 1) that runs the kernel shell

---

## 3. Key Data Structure Design

(See the integrated SPEC.md for the detailed original definitions. The following are the stabilized versions that future Design work should respect or explicitly evolve.)

**PCB (Process Control Block)**
- pid, state, rsp, rip, cr3 (page table root), priority, time_slice
- Parent, exit_code
- IPC message buffer + pending state

**Message**
- source, dest, type, size, payload (48 bytes in current design)

**Memory Block Header (Heap)**
- Size (sign indicates free/used), prev offset

These structures are intentionally simple and JStar-friendly (arrays, small records).

---

## 4. Expansion Design Principles (NeuroDiOS Evolution)

When growing this into the full NeuroDiOS kernel:

- **JStar-Native Abstractions**: Scheduling, IPC, and memory primitives should expose concepts that map cleanly to TokenVector / morphlex thinking (bounded contexts, explicit cost, deterministic outcomes where possible).
- **User-Space Services**: The existing nnos daemons (MorphogeneticMaintainer, ThreatIntelligenceManager, etc.) become ordinary processes that communicate via the kernel IPC.
- **Validation Layer Integration**: The Jasterish validation runtime (from the nnos specs) can eventually guard critical kernel transitions or user-space service calls.
- **Morphogenetic Participation**: The kernel should eventually expose hooks or observation points so that higher-level repair logic can influence scheduling weights, memory pressure responses, or process priority in a controlled way.
- **Determinism First**: Any new scheduler or allocator policy must be accompanied by a clear reproducibility story under controlled workloads.

---

## 5. Component Interaction Model

```
User Processes (Ring 3)
        ⇅ IPC (send/recv via syscall)
Kernel (Ring 0)
  - Syscall dispatcher
  - IPC subsystem
  - Scheduler
  - VMM / PMM
  - Drivers (minimal)
Hardware
```

Services that would traditionally be "in the kernel" (file systems, networking stacks, complex device drivers) are designed to live in user space from day one.

---

## 6. Build and Tooling Design

- Current: Custom Makefile + linker.ld targeting bare-metal x86-64 ELF.
- Future: Tight integration with the Jasterish toolchain from `apps/`.
- The kernel should be buildable as part of the overall NeuroDiOS workspace once Jasterish self-hosting and validation mature.

---

**End of Design Specification (Initial Version)**

This document captures the current architecture from the integrated sources and sets the philosophical direction for NeuroDiOS evolution. The Technical Specification (exact syscall numbers and calling conventions, full PCB and message layouts, memory layout constants, IDT/ISR design, build system details, and testing harness) follows as the third document in the required triad.

Further expansion work will refine these designs as the kernel is brought up on real hardware and the higher-level NeuroDiOS daemons are ported on top of it.