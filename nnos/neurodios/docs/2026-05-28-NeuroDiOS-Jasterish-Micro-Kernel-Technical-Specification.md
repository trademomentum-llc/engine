# Technical Specification: NeuroDiOS — Jasterish Micro-Kernel Foundation

**Document ID:** NEURODIOS-TECH-001  
**Version:** 0.9 (Initial Integration Version)  
**Date:** 2026-05-28  
**Predecessors:** NEURODIOS-REQ-001, NEURODIOS-DS-001

---

## 1. Scope of This Document

This Technical Specification captures the concrete, implemented details of the integrated Jasterish Micro-Kernel as it existed at the time of import (May 2026), plus the minimal extensions and conventions needed to make it a governed part of the NeuroDiOS workspace.

It is intentionally an "as-integrated" snapshot. Full production-grade Technical Specification work (complete IDT/ISR tables, exact syscall calling conventions with register usage, formal memory layout constants, build system evolution, testing harness, etc.) will be expanded in subsequent revisions as the kernel is exercised and ported.

---

## 2. Source Layout (Post-Integration)

Location in workspace:
`engine/nnos/neurodios/jasterish-microkernel/`

Core modules (all `.jstr`):
- boot.jstr          (~828 LOC)
- memory.jstr        (~1,453 LOC)
- process.jstr       (~2,394 LOC)
- ipc.jstr           (~636 LOC)
- syscall.jstr       (~791 LOC)
- drivers.jstr       (~703 LOC)
- kernel.jstr        (~742 LOC)

Build artifacts:
- Makefile
- linker.ld
- .gitignore

Supporting documentation (original):
- README.md (inside the kernel dir)
- plan.md (top level of the Kimi drop)
- SPEC.md (top level of the Kimi drop) — contains the original detailed data structure definitions.

---

## 3. Current Concrete Interfaces (Extracted from Sources)

### 3.1 System Calls (int 0x80)

Current implemented set (from kernel sources and SPEC):
- exit
- fork
- yield
- send / recv (IPC)
- sleep
- getpid
- puts (debug output)
- brk
- kill
- getticks

**Note:** Exact register passing convention (which registers hold syscall number and arguments) must be documented from the actual syscall.jstr handler before any user-space code is written against this kernel.

### 3.2 IPC Message Format (Current)

From the integrated SPEC.md:
- source PID
- dest PID
- type
- size
- 48-byte payload

Total on-wire message size in current design is small and fixed (good for a micro-kernel).

### 3.3 Memory Layout (Higher-Half Kernel)

From the integrated sources (higher-half design):
- Kernel mapped physical memory window
- Kernel code/data in higher half
- User space in lower canonical addresses

Exact virtual addresses and the layout of the initial page tables are defined in memory.jstr and boot.jstr.

### 3.4 Scheduler

Round-robin with time slicing driven by PIT timer (currently 1 kHz in drivers.jstr).

PCB table size: 256 entries (hard limit in current design).

---

## 4. Build & Boot Environment (Current)

- Multiboot2 compatible (intended for GRUB or direct QEMU -kernel loading).
- Custom linker script (linker.ld) that places kernel in higher half.
- Makefile that assembles/compiles the JStar sources via the Jasterish toolchain and links with the provided ld script.

**Critical Dependency:** The Jasterish compiler and runtime from `apps/` (the JStar self-hosting work). The kernel cannot be built or evolved without a working JStar toolchain.

---

## 5. Immediate Technical Gaps to Address (Post-Integration)

1. **Toolchain Integration** — Formalize how the kernel build consumes the Jasterish artifacts from apps/.
2. **Testing Harness** — QEMU launch scripts + automated smoke tests (boot to shell prompt, basic fork + IPC test).
3. **Documentation of Exact ABIs** — Syscall register usage, context switch register save/restore layout, page table entry formats as actually implemented.
4. **Stability** — The scheduler and memory manager need heavy exercising; known JStar codegen limitations (see apps/ T-Diagram work) must be respected or worked around.
5. **NeuroDiOS Extensions** — Define the first set of NeuroDiOS-specific syscalls or IPC message types (context budget queries, physiology feedback channels, etc.).

---

## 6. Relationship to Existing nnos Work

The daemons and services described in the nnos specs (SystemIntegrityDaemon, MorphogeneticMaintainer, ThreatIntelligenceManager, etc.) are designed to run as user-space processes on top of this kernel once the kernel is stable and the JStar validation layer is available.

The kernel itself should eventually expose the minimal hooks needed for those higher layers to observe and (in controlled ways) influence scheduling and memory behavior in service of neurodivergent safety goals.

---

**End of Initial Technical Specification**

This document, together with the Requirements and Design Specifications, forms the initial mandated triad for the NeuroDiOS Jasterish Micro-Kernel Foundation.

Subsequent revisions will expand the concrete ABIs, memory layout constants, build system, and testing requirements as the kernel is brought up and the broader NeuroDiOS system is layered on top of it.