# Phase 2 Subsystems Additions — NeuroDiOS Jasterish Micro-Kernel

**Document ID:** NEURODIOS-JMK-PHASE2-001  
**Version:** 1.0.0  
**Date:** 2026-05-28  
**Status:** Additions to the main triad (2026-05-28-NeuroDiOS-Jasterish-Micro-Kernel-*.md)

---

## 1. Overview

Following the integration of the significantly expanded kernel sources from the Apps folder (post JStar-2 codegen fix), this document records the completed Phase 2 subsystems that extend the original v1.0 micro-kernel.

These additions close critical gaps toward a bootable, higher-half, multi-user-space kernel with basic storage and execution capabilities.

All changes are documented in the source `EXPANSION_PLAN.md` (integrated at `neurodios/jasterish-microkernel/EXPANSION_PLAN.md`).

---

## 2. Summary of Additions

| Area                        | New/Improved Modules          | Key Capabilities                              | Syscall / Shell Impact      |
|-----------------------------|-------------------------------|-----------------------------------------------|-----------------------------|
| Exception & Interrupt Handling | idt.jstr                    | Full IDT (256 gates), exception handlers (0-31), IRQ routing, int 0x80 syscall stub | System call entry stabilized |
| Higher-Half Kernel & TSS    | (integrated into boot + kernel) | Multiboot info preservation, high-half kernel mapping, TSS for ring-3, `enter_user_mode` | User-mode transition support |
| Storage & VFS               | disk.jstr, vfs.jstr           | ATA PIO (LBA28), RAMFS (16×1KB files), persistence via disk sectors | sys_open/read/write/close/list + ls/cat/sync |
| Program Loading             | elf.jstr                      | ELF64 validation, LOAD segment mapping, BSS zeroing | sys_exec                    |
| Process Improvements        | (process.jstr updates)        | Copy-on-Write fork                            | sys_fork now CoW            |
| Shell & Usability           | (kernel.jstr + syscall updates) | Additional commands (echo, etc.)              | +3 shell commands           |

**Statistics (from EXPANSION_PLAN):**
- Source files: 7 → 11 (+4)
- Total lines: ~7,500 → ~13,000 (+5,500)
- Syscalls: 11 → 17 (+6)
- Shell commands: 5 → 8 (+3)

---

## 3. Design Implications for NeuroDiOS

These subsystems move the kernel from a minimal demonstration toward a practical foundation capable of hosting user-space services (including the future evolution of the original nnos daemons).

Particular alignment with NeuroDiOS goals:
- Deterministic intent preserved in the JStar implementation.
- Clean separation of privileged code (still minimal) from user-space (VFS, executables, etc.).
- Foundation for future morphogenetic/self-healing services via proper process and storage primitives.
- Path toward running the JStar validation layer and higher-level neurodivergent daemons as user processes.

---

## 4. Technical Notes

- All new modules were added to the Makefile `JSTR_SRCS` list.
- Build produces a single higher-half ELF (currently using one PT_LOAD segment for simplicity).
- The data_fixups + linker patching path (now confirmed stable after the JStar-2 codegen fix) is used for all absolute .data references.
- CoW fork and ELF loading are foundational for any future multi-process user-space environment on NeuroDiOS.

---

## 5. References

- Main triad: `2026-05-28-NeuroDiOS-Jasterish-Micro-Kernel-Requirements.md`, `-Design-Specification.md`, `-Technical-Specification.md`
- Source EXPANSION_PLAN: `jasterish-microkernel/EXPANSION_PLAN.md`
- Kernel sources: `neurodios/jasterish-microkernel/`

---

**End of Phase 2 Additions Note**

This document serves as an official extension to the primary NeuroDiOS Jasterish Micro-Kernel triad. It will be incorporated into future revisions of the main Design and Technical Specifications.