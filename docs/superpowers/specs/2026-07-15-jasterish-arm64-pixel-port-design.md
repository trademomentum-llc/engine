# Design Specification: Jasterish Micro-Kernel ARM64 Port + Pixel 9 Pro Path

**Document ID:** NEURODIOS-DS-ARM64-001  
**Version:** 1.0.0  
**Date:** 2026-07-15  
**Author:** Kimi Code (finishing-a-development-branch + brainstorming skills)  
**Predecessor:** NEURODIOS-REQ-001, NEURODIOS-DS-001, NNOS Gap Analysis & Roadmap  

---

## 1. Purpose and Scope

This document specifies the first implementation cycle for bringing the Jasterish Micro-Kernel (JMK) from x86-64-only to a multi-architecture kernel capable of targeting ARM64, with a documented path toward booting on a Google Pixel 9 Pro (Tensor G4, ARMv9).

### 1.1 In Scope

- Add an AArch64 machine-code backend to the JStar compiler (`apps/src/jstar/`).
- Restructure `nnos/neurodios/jasterish-microkernel/` so architecture-specific code is isolated from common kernel services.
- Implement AArch64 boot, exception vectors, MMU, GIC, timer, and UART in Jasterish.
- Bring the kernel up on QEMU ARM64 `virt` machine.
- Provide a documented path (DTB parser, HAL stubs, memory-map placeholder) for the Pixel 9 Pro Tensor G4 HAL.

### 1.2 Explicitly Out of Scope (Deferred)

- Telemetry/observability pipeline (separate follow-on cycle).
- Local AI inference stack: Qwen, vLLM, Ollama, HuggingFace, LangChain, Qdrant, Firecrawl, Cohere/OpenAI embeddings, Weights & Biases (separate follow-on cycle).
- Full Pixel 9 Pro device drivers: display, touch, UFS, PMIC, modem, cellular.
- Production-ready power management.

### 1.3 Success Criteria

1. `apps/src/jstar/` can compile a Jasterish program to both x86-64 and AArch64 machine code.
2. The micro-kernel builds for AArch64 and boots to an interactive shell in QEMU `virt`.
3. The existing x86-64 QEMU path continues to work without regressions.
4. Pixel 9 Pro HAL has a documented memory map and stubbed driver interfaces.

---

## 2. Background and Constraints

### 2.1 Current State

- The Jasterish micro-kernel is x86-64 bare metal, ~13,000 lines across 11 modules.
- It is structurally complete (boot, memory, process, IPC, syscall, VFS, ELF, disk, drivers) but not yet booting in QEMU.
- The JStar compiler (`apps/src/jstar/codegen.rs`) emits only x86-64 machine code via direct byte emission.
- No ARM64 code, device tree, or Pixel-specific documentation exists in the repository.

### 2.2 Target Hardware

- **Google Pixel 9 Pro**: ARM64 SoC (Google Tensor G4, ARMv9).
- **QEMU stepping stone**: `qemu-system-aarch64 -machine virt`, a stable, well-documented ARM64 platform with PL011 UART, GICv2, ARM Generic Timer, and virtio-mmio.

### 2.3 Key Constraints

- JStar compiler must remain dependency-free (no LLVM/Cranelift) to preserve self-hosting path.
- Kernel must remain deterministic and integer-only (no floating point in kernel path).
- Pixel 9 Pro register-level documentation is not public; real hardware bring-up will require reverse engineering or derived DTB and is explicitly deferred beyond this cycle.

---

## 3. Design Approach

### 3.1 Selected Approach: Multi-Architecture Abstraction + QEMU First

This approach adds an AArch64 backend to the compiler, isolates architecture-specific code behind a HAL, proves correctness on QEMU `virt`, and only then stubs out the Pixel 9 Pro HAL.

**Why this approach:**
- Validates the compiler backend on stable hardware before dealing with closed Pixel details.
- Produces a portable kernel that can target both x86-64 and ARM64.
- Minimizes risk by isolating compiler, kernel, and hardware concerns.

### 3.2 Rejected Alternatives

- **Pixel-first ARM64 port**: Too risky; debugging compiler, kernel port, and Pixel hardware simultaneously makes failure attribution hard.
- **C/Assembly ARM64 kernel**: Breaks the Jasterish-as-foundation mandate and largely discards existing work.

---

## 4. Compiler Backend Design

### 4.1 Architecture Abstraction

Introduce a target-architecture abstraction in the JStar compiler while keeping the IR unchanged.

```rust
pub enum Arch {
    X86_64,
    Aarch64,
}
```

- Split `apps/src/jstar/codegen.rs` into:
  - `apps/src/jstar/codegen/mod.rs` — shared `MachineCode` struct, target selector, public `generate(arch: Arch, program: &IrProgram)` entry.
  - `apps/src/jstar/codegen/x86_64.rs` — relocated existing x86-64 emitter.
  - `apps/src/jstar/codegen/aarch64.rs` — new AArch64 emitter.
- Add `--target aarch64` CLI flag (default `x86_64`).

### 4.2 AArch64 Emitter (MVP)

The AArch64 emitter follows the same IR → machine-code pipeline as x86-64 but emits A64 instructions.

**Calling convention:** AAPCS64
- Integer/pointer arguments: `x0-x7`.
- Return value: `x0`.
- Callee-saved: `x19-x29`, `x30` (link register).
- Stack pointer (`sp`) must remain 16-byte aligned.

**Instruction categories to support:**
- Integer arithmetic: `add`, `sub`, `madd`/`msub`, `sdiv`, `udiv`.
- Bitwise and shifts: `and`, `orr`, `eor`, `lsl`, `lsr`, `asr`.
- Loads/stores: `ldr`, `str`, `ldrb`, `strb`, `ldp`, `stp`.
- Branches: `b`, `bl`, `cbz`/`cbnz`, conditional `b.cond`.
- Moves and immediates: `mov`, `movz`, `movn`, `movk`.
- System registers: `mrs`, `msr` for kernel use.

**Out of scope for MVP:**
- SIMD/NEON.
- Floating point.
- Advanced register allocation (keep simple linear mapping).
- Linker relaxation.

### 4.3 Compiler Testing

Add unit tests in `apps/src/jstar/codegen/` that:
1. Compile a small JStar program for both architectures.
2. Emit an ELF64 object file (`--format elf`).
3. Disassemble with `objdump -d` (x86-64) or `aarch64-linux-gnu-objdump -d` (AArch64) and verify key instructions are present.

Example test programs:
- Integer arithmetic and function call.
- Conditional branch and loop.
- Global data load/store.

---

## 5. Kernel Architecture Abstraction

### 5.1 Directory Restructuring

```
nnos/neurodios/jasterish-microkernel/
├── common/
│   ├── kernel.jstr      # kernel_main, panic, init shell
│   ├── process.jstr     # scheduler, PCB, fork/exec, context switch
│   ├── ipc.jstr         # message passing
│   ├── vfs.jstr         # RAMFS + VFS operations
│   ├── elf.jstr         # ELF loader
│   ├── syscall.jstr     # syscall table + dispatch
│   └── memory_common.jstr # page allocator policy, kernel heap
├── arch/
│   ├── x86_64/
│   │   ├── boot.jstr    # Multiboot2, long mode, GDT
│   │   ├── idt.jstr     # IDT, exceptions, IRQs
│   │   ├── memory_arch.jstr # x86-64 page tables, TLB shootdown
│   │   ├── drivers.jstr # PIC, PIT, PS/2, COM1
│   │   └── linker.ld
│   └── aarch64/
│       ├── boot.jstr    # EL drop, MMU enable, DTB pointer
│       ├── exceptions.jstr # vector table, EL1 handlers
│       ├── memory_arch.jstr # 4-level translation, TTBR0/TTBR1
│       ├── gic.jstr     # GICv2 distributor + CPU interface
│       ├── timer.jstr   # ARM Generic Timer (CNTP)
│       ├── uart.jstr    # PL011 + stubbed Pixel UART
│       ├── fdt.jstr     # flattened device tree parser
│       ├── hal.jstr     # HAL registration table
│       └── linker.ld
└── Makefile             # arch selection: make ARCH=aarch64
```

### 5.2 Hardware Abstraction Layer (HAL)

Each architecture exposes identical symbols so `common/` is architecture-agnostic:

| Symbol | Purpose |
|--------|---------|
| `arch_init(dtb_ptr)` | Early arch setup. `dtb_ptr` is the DTB physical address on ARM64; `0` on x86-64. |
| `arch_putc(c)` | Write one character to console. |
| `arch_getc()` | Read one character from console (blocking). |
| `arch_irq_enable()` | Enable interrupts at the CPU. |
| `arch_irq_disable()` | Disable interrupts at the CPU. |
| `arch_timer_init(freq)` | Initialize periodic timer. |
| `arch_timer_ack()` | Acknowledge timer interrupt. |
| `arch_mmu_map(vaddr, paddr, flags)` | Map a page. |
| `arch_mmu_unmap(vaddr)` | Unmap a page. |
| `arch_mmu_switch(page_table)` | Switch active page table. |
| `arch_panic()` | Register dump and halt. |

### 5.3 Common Code Changes

- Move scheduler, IPC, VFS, ELF, and syscalls to `common/`.
- Replace direct x86-64 register accesses (e.g., `inb`/`outb`, `cr3` reads/writes, IDT loads) with `arch_*` HAL calls.
- Keep process context-switch logic in `common/` but call `arch_save_context`/`arch_restore_context` helpers implemented per arch.
- Replace inline `cli`/`sti` with `arch_irq_disable()` / `arch_irq_enable()`.

---

## 6. QEMU ARM64 `virt` Bring-Up

### 6.1 Boot Path

QEMU `-machine virt` loads the kernel at a known address. The AArch64 `_start`:

1. Save DTB pointer from `x0`.
2. Detect current exception level (`CurrentEL`).
3. If at EL2, drop to EL1 by configuring `hcr_el2`, `spsr_el2`, and `elr_el2`, then `eret`. If already at EL1, continue.
4. Set up EL1 stack.
5. Initialize early page tables and enable MMU.
6. Call `arch_init(dtb)`.
7. Jump to `kernel_main()`.

### 6.2 Memory Layout

```
0x0000_0000_0000_0000  user space (TTBR0)
0xFFFF_0000_0000_0000  kernel direct map
0xFFFF_8000_0000_0000  kernel text/data/bss (linked base)
0xFFFF_FFFF_FFFF_F000  early trampoline / temporary mappings
```

### 6.3 Devices

| Device | Base Address | Purpose |
|--------|--------------|---------|
| GICv2 Distributor | `0x0800_0000` | Interrupt distribution |
| GICv2 CPU Interface | `0x0801_0000` | Per-CPU interrupt control |
| PL011 UART | `0x0900_0000` | Console I/O |
| ARM Generic Timer | CPU-local | Scheduler tick |
| virtio-mmio block | DTB-discovered | Optional storage (not required for shell boot) |

### 6.4 Validation Gate

`make ARCH=aarch64 qemu` builds and runs the kernel in QEMU and boots to the init shell, accepting keyboard input via PL011.

---

## 7. Pixel 9 Pro HAL

### 7.1 Reality Check

The Pixel 9 Pro uses Google's Tensor G4 SoC. Detailed register-level documentation is not publicly available. This milestone will not boot on real hardware; it will create the structural path for future hardware bring-up.

### 7.2 Deliverables

- **FDT parser** (`arch/aarch64/fdt.jstr`): Walks the flattened device tree to extract memory reservations, CPU topology, and device nodes.
- **HAL registration table** (`arch/aarch64/hal.jstr`): Drivers register by compatible string (`"arm,sbsa-uart"`, `"arm,gic-v3"`, etc.).
- **Stubbed drivers** for Tensor G4 devices:
  - UART (SBSA-compatible until actual base address is known).
  - GICv3/v4 (distributor, redistributor, CPU interface).
  - ARM Generic Timer.
  - UFS HCI — stub only.
  - PMIC/reset — stub only.
- **Memory-map placeholder** in `docs/pixel-9-pro-memory-map.md` with known/derived base addresses and open questions.

### 7.3 Boot Path on Pixel

1. Device bootloader loads kernel and passes DTB in `x0`.
2. Kernel parses DTB to discover RAM, UART, GIC, timer.
3. Initialize common kernel services.
4. Future cycles: bring up display, touch, storage, cellular.

---

## 8. Testing and Validation

### 8.1 Compiler Backend Tests

- Unit tests compile small JStar programs for both x86-64 and AArch64.
- Verify emitted bytes via disassembly (`objdump -d` for ELF, `aarch64-linux-gnu-objdump` for AArch64).
- Cover arithmetic, function calls, branches, and global data access.

### 8.2 Kernel Regression Tests

- `make ARCH=x86_64 qemu` continues to boot to shell.
- `make ARCH=aarch64 qemu` boots to shell.

### 8.3 New AArch64 Smoke Test

Add `nnos/neurodios/jasterish-microkernel/scripts/smoke_aarch64.sh`:

1. Build AArch64 kernel.
2. Run `qemu-system-aarch64`.
3. Expect serial output containing `BOOT` and shell prompt.
4. Send a simple command and verify response.

### 8.4 CI

Extend `nnos/.github/workflows/ci.yml` with a job that:
1. Installs QEMU (`qemu-system-aarch64`) and an AArch64 toolchain.
2. Builds the JStar compiler.
3. Builds the AArch64 micro-kernel.
4. Runs the AArch64 smoke test.

---

## 9. Risks and Mitigations

| Risk | Mitigation |
|------|------------|
| AArch64 instruction encoding is error-prone. | Extensive unit tests + disassembly verification. |
| QEMU `virt` differs significantly from Pixel G4. | Complete QEMU bring-up first; Pixel HAL is stubbed and documented, not fully implemented. |
| Refactoring x86-64 kernel breaks existing path. | Keep x86-64 smoke test in CI and run it on every change. |
| JStar compiler IR is tightly coupled to x86-64. | Keep IR unchanged; only final emitter is arch-specific. |

---

## 10. Future Work (Out of Scope)

- Telemetry/observability pipeline for native services.
- Hybrid AI inference stack on Pixel + host.
- Full Pixel 9 Pro device driver implementation.
- Security hardening (PQC signatures, attestation).
- Jasterish validation runtime guard integration.

---

**End of Design Specification**
