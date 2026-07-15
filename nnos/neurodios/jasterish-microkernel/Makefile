# ============================================================================
# Makefile -- Jasterish Micro-Kernel Build System
# ============================================================================
#
# This Makefile compiles the JMK (Jasterish Micro-Kernel) from Jasterish
# source files (.jstr) into a bootable kernel binary.
#
# Build Chain:
#   1. Jasterish Compiler (morphlex jstar): .jstr -> kernel.elf
#      The compiler concatenates all --include files with the main input and
#      emits a single native ELF binary directly. It does NOT produce per-file
#      .o objects and does NOT support --format elf.
#   2. Object Copy (objcopy):                 kernel.elf -> kernel.bin
#
# Tools Required:
#   - morphlex jstar   : Jasterish compiler
#   - $(ARCH)-elf-objcopy : Object file conversion utility
#   - qemu-system-$(ARCH) : QEMU emulator (for run/debug targets)
#   - gdb            : GNU debugger (for debug target)
#   - grub-mkrescue  : GRUB2 rescue image creator (for ISO target)
#
# Usage:
#   make              -- Build kernel.bin
#   make all          -- Full build (kernel.bin + kernel.elf)
#   make run          -- Build and launch in QEMU
#   make iso          -- Create bootable ISO with GRUB2
#   make debug        -- Launch in QEMU with GDB server
#   make test         -- Run kernel self-test in QEMU
#   make clean        -- Remove all build artifacts
#   make count        -- Count total lines of Jasterish source code
# ============================================================================

# ----------------------------------------------------------------------------
# Configuration
# ----------------------------------------------------------------------------

# Target architecture
ARCH            := x86_64
TARGET          := $(ARCH)-elf

# Kernel name
KERNEL_NAME     := jmk
KERNEL_ELF      := $(KERNEL_NAME).elf
KERNEL_BIN      := $(KERNEL_NAME).bin
KERNEL_ISO      := $(KERNEL_NAME).iso

# Jasterish compiler.
# The real compiler on this machine is the morphlex JStar frontend.
JSTAR           := /Users/nnos/Projects/apps/target/debug/morphlex jstar compile

# Cross-compiler toolchain (adjust prefix if using a cross-compiler)
ifeq ($(ARCH),aarch64)
  CROSS_PREFIX  := $(or $(CROSS_COMPILE),aarch64-elf-)
else
  CROSS_PREFIX  := $(or $(CROSS_COMPILE),x86_64-elf-)
endif
CC              := $(CROSS_PREFIX)gcc
LD              := $(CROSS_PREFIX)ld
OBJCOPY         := $(CROSS_PREFIX)objcopy
OBJDUMP         := $(CROSS_PREFIX)objdump

# Emulator and debugger
GDB             := gdb

# GRUB2 tools
GRUB_MKRESCUE   := grub-mkrescue

# ----------------------------------------------------------------------------
# Source Files
# ----------------------------------------------------------------------------
# All Jasterish source files for the kernel.
# Order matters: boot must be first (contains _start entry point).
# Architecture-specific sources live under arch/$(ARCH)/;
# portable kernel services live under common/.
#
# NOTE: morphlex compiles the FIRST file as the main input and concatenates
# all remaining files via --include. Do not add files that do not exist.

ifeq ($(ARCH),aarch64)
  JSTR_SRCS := arch/aarch64/boot.jstr \
               arch/aarch64/memory_arch.jstr \
               arch/aarch64/uart.jstr \
               arch/aarch64/gic.jstr \
               arch/aarch64/timer.jstr \
               arch/aarch64/exceptions.jstr \
               arch/aarch64/fdt.jstr \
               arch/aarch64/pixel_hal.jstr \
               arch/aarch64/hal.jstr \
               common/elf.jstr \
               common/process.jstr \
               common/ipc.jstr \
               common/syscall.jstr \
               common/vfs.jstr \
               common/disk.jstr \
               common/kernel.jstr
  LINKER_SCRIPT := arch/aarch64/linker.ld
  QEMU := qemu-system-aarch64
  QEMU_MACHINE := virt
  QEMU_CPU := cortex-a72
else
  JSTR_SRCS := arch/x86_64/boot.jstr \
               arch/x86_64/idt.jstr \
               arch/x86_64/memory_arch.jstr \
               arch/x86_64/drivers.jstr \
               common/elf.jstr \
               common/process.jstr \
               common/ipc.jstr \
               common/syscall.jstr \
               common/vfs.jstr \
               common/disk.jstr \
               common/kernel.jstr
  LINKER_SCRIPT := arch/x86_64/linker.ld
  QEMU := qemu-system-x86_64
  QEMU_MACHINE := q35
  QEMU_CPU := qemu64
endif

# Main input file for morphlex (must contain _start).
MAIN_SRC        := $(firstword $(JSTR_SRCS))

# All remaining files are passed as --include.
INCLUDE_SRCS    := $(wordlist 2, $(words $(JSTR_SRCS)), $(JSTR_SRCS))

# Build includes the linker script as a dependency even though the current
# morphlex compiler emits the ELF directly; the script is staged for future
# post-link layout control or for a compiler revision that accepts it.
LINKER_DEPS     := $(LINKER_SCRIPT)

# ----------------------------------------------------------------------------
# Compiler Flags
# ----------------------------------------------------------------------------

# Jasterish compiler flags
#   --target $(ARCH)    : Generate target machine code (x86_64 or aarch64)
JSTARFLAGS      := --target $(ARCH)

# Include flags for morphlex: every source after the main input.
JSTAR_INCLUDES  := $(foreach f,$(INCLUDE_SRCS),--include $(f))

# Linker flags
#   -T $(LINKER_SCRIPT) : Use our custom linker script
#   -nostdlib           : Do not link against standard libraries
#   -static             : Static linking only
#   -z max-page-size=4096 : Set maximum page size to 4KB
LDFLAGS         := -T $(LINKER_SCRIPT) \
                   -nostdlib \
                   -static \
                   -z max-page-size=4096 \
                   --no-relax

# Linker map file (for debugging memory layout)
LDMAP           := $(KERNEL_NAME).map

# ----------------------------------------------------------------------------
# QEMU Configuration
# ----------------------------------------------------------------------------

# QEMU machine settings
QEMU_MEM        := 512

# QEMU common flags
QEMU_BASE       := -machine $(QEMU_MACHINE) \
                   -cpu $(QEMU_CPU) \
                   -m $(QEMU_MEM) \
                   -serial stdio \
                   -no-reboot \
                   -no-shutdown

# QEMU display (disable for headless operation)
QEMU_DISPLAY    := -display none

# QEMU debug flags (for GDB remote debugging)
QEMU_DEBUG      := -s -S

# QEMU Multiboot2 boot (direct kernel loading)
QEMU_KERNEL     := -kernel $(KERNEL_BIN)

# QEMU ISO boot
QEMU_ISO_BOOT   := -cdrom $(KERNEL_ISO) -boot d

# Combined QEMU run flags
# run: interactive (display enabled); test: headless capture
QEMU_RUN_FLAGS  := $(QEMU_BASE) $(QEMU_KERNEL)
QEMU_TEST_FLAGS := $(QEMU_BASE) $(QEMU_DISPLAY) $(QEMU_KERNEL)
QEMU_DEBUG_FLAGS:= $(QEMU_BASE) $(QEMU_DISPLAY) $(QEMU_KERNEL) $(QEMU_DEBUG)
QEMU_ISO_FLAGS  := $(QEMU_BASE) $(QEMU_DISPLAY) $(QEMU_ISO_BOOT)

# ----------------------------------------------------------------------------
# GDB Configuration
# ----------------------------------------------------------------------------

GDB_INIT        := .gdbinit
GDB_TARGET      := target remote localhost:1234
GDB_SYMBOLS     := file $(KERNEL_ELF)

# ----------------------------------------------------------------------------
# Default Target
# ----------------------------------------------------------------------------

.PHONY: all build run iso debug test selftest clean count help

# Default: build the kernel binary
all: build

# Full build: ELF + binary
build: $(KERNEL_BIN) $(KERNEL_ELF)
	@echo ""
	@echo "========================================"
	@echo "  Build Complete: $(KERNEL_NAME)"
	@echo "========================================"
	@echo "  ELF:   $(KERNEL_ELF)"
	@echo "  BIN:   $(KERNEL_BIN)"
	@echo "  Size:  $$(wc -c < $(KERNEL_BIN)) bytes"
	@echo "========================================"

# ----------------------------------------------------------------------------
# Compilation Rules
# ----------------------------------------------------------------------------

# Rule: compile all Jasterish sources together into the kernel ELF.
# morphlex concatenates --include files before the main input and emits the
# final native binary directly.
$(KERNEL_ELF): $(JSTR_SRCS) $(LINKER_DEPS)
	@echo "  JSTAR   $(MAIN_SRC) + $(words $(INCLUDE_SRCS)) include(s)"
	@$(JSTAR) $(JSTARFLAGS) --input $(MAIN_SRC) $(JSTAR_INCLUDES) --output $@

# Rule: Convert ELF to raw binary (for direct kernel loading)
$(KERNEL_BIN): $(KERNEL_ELF)
	@echo "  OBJCOPY $@"
	@$(OBJCOPY) -O binary $< $@

# ----------------------------------------------------------------------------
# QEMU Targets
# ----------------------------------------------------------------------------

# Run the kernel in QEMU (normal mode)
run: build
	@echo "========================================"
	@echo "  Launching JMK in QEMU..."
	@echo "========================================"
	@$(QEMU) $(QEMU_RUN_FLAGS)

# Run with ISO (GRUB2 bootloader)
run-iso: iso
	@echo "========================================"
	@echo "  Launching JMK ISO in QEMU..."
	@echo "========================================"
	@$(QEMU) $(QEMU_ISO_FLAGS)

# ----------------------------------------------------------------------------
# Debug Targets
# ----------------------------------------------------------------------------

# Launch QEMU with GDB server (waits for debugger connection)
debug: build
	@echo "========================================"
	@echo "  QEMU waiting for GDB on port 1234..."
	@echo "  Run: gdb -x $(GDB_INIT)"
	@echo "========================================"
	@$(QEMU) $(QEMU_DEBUG_FLAGS)

# Create GDB initialization script
gdbinit:
ifeq ($(ARCH),aarch64)
	@echo "set architecture aarch64" > $(GDB_INIT)
else
	@echo "set architecture i386:x86-64" > $(GDB_INIT)
endif
	@echo "$(GDB_SYMBOLS)" >> $(GDB_INIT)
	@echo "$(GDB_TARGET)" >> $(GDB_INIT)
	@echo "set disassemble-next-line on" >> $(GDB_INIT)
	@echo "layout asm" >> $(GDB_INIT)
	@echo "layout regs" >> $(GDB_INIT)
	@echo "Created $(GDB_INIT)"

# ----------------------------------------------------------------------------
# ISO Target (GRUB2 Bootable Image)
# ----------------------------------------------------------------------------

# Create a bootable ISO using GRUB2
iso: build
	@echo "========================================"
	@echo "  Creating bootable ISO..."
	@echo "========================================"
	@mkdir -p iso/boot/grub
	@cp $(KERNEL_BIN) iso/boot/$(KERNEL_NAME)
	@echo "menuentry \"Jasterish Micro-Kernel\" {" > iso/boot/grub/grub.cfg
	@echo "    multiboot2 /boot/$(KERNEL_NAME)" >> iso/boot/grub/grub.cfg
	@echo "    boot" >> iso/boot/grub/grub.cfg
	@echo "}" >> iso/boot/grub/grub.cfg
	@$(GRUB_MKRESCUE) -o $(KERNEL_ISO) iso/ 2>/dev/null
	@echo "  ISO: $(KERNEL_ISO)"
	@echo "========================================"

# ----------------------------------------------------------------------------
# Test Target
# ----------------------------------------------------------------------------

# Run kernel self-test in QEMU (non-interactive, capture output)
test: build
	@echo "========================================"
	@echo "  Running JMK self-test in QEMU..."
	@echo "========================================"
	@$(QEMU) $(QEMU_TEST_FLAGS) | tee test_output.log || true
	@echo ""
	@echo "Test output saved to: test_output.log"

# Quick line count of all Jasterish source files
count:
	@echo "========================================"
	@echo "  Source Code Statistics"
	@echo "========================================"
	@echo "  Files:  $$(echo $(JSTR_SRCS) | wc -w)"
	@echo "  Lines:  $$(cat $(JSTR_SRCS) 2>/dev/null | wc -l)"
	@echo "  Bytes:  $$(cat $(JSTR_SRCS) 2>/dev/null | wc -c)"
	@echo "========================================"
	@echo "  Per-file breakdown:"
	@echo ""
	@for f in $(JSTR_SRCS); do \
		printf "  %-20s %6d lines\n" "$$f" "$$(wc -l < $$f)"; \
	done
	@echo "========================================"

# ----------------------------------------------------------------------------
# Cleanup Target
# ----------------------------------------------------------------------------

clean:
	@echo "Cleaning build artifacts..."
	@rm -f $(KERNEL_ELF)
	@rm -f $(KERNEL_BIN)
	@rm -f $(KERNEL_ISO)
	@rm -f $(LDMAP)
	@rm -f $(GDB_INIT)
	@rm -rf iso/
	@rm -f test_output.log
	@echo "Clean complete."

# ----------------------------------------------------------------------------
# Help Target
# ----------------------------------------------------------------------------

help:
	@echo ""
	@echo "Jasterish Micro-Kernel -- Build System Help"
	@echo "==========================================="
	@echo ""
	@echo "Build Targets:"
	@echo "  make              Build kernel binary (default)"
	@echo "  make all          Full build (ELF + BIN)"
	@echo "  make build        Alias for 'make all'"
	@echo ""
	@echo "Run Targets:"
	@echo "  make run          Build and launch in QEMU (direct kernel boot)"
	@echo "  make run-iso      Build and launch ISO in QEMU (GRUB2 boot)"
	@echo "  make test         Run kernel in QEMU, capture output to log"
	@echo ""
	@echo "Debug Targets:"
	@echo "  make debug        Launch QEMU with GDB server (port 1234)"
	@echo "  make gdbinit      Create GDB initialization script"
	@echo ""
	@echo "Utility Targets:"
	@echo "  make iso          Create bootable ISO image with GRUB2"
	@echo "  make count        Show source code line counts"
	@echo "  make clean        Remove all build artifacts"
	@echo "  make help         Show this help message"
	@echo ""
	@echo "Configuration Variables:"
	@echo "  ARCH                 Target architecture (x86_64 | aarch64, default: x86_64)"
	@echo "  CROSS_COMPILE        Cross-compiler prefix (default: x86_64-elf- / aarch64-elf-)"
	@echo "  QEMU_MEM             QEMU RAM in MB (default: 512)"
	@echo ""
	@echo "Examples:"
	@echo "  make ARCH=x86_64"
	@echo "  make ARCH=aarch64"
	@echo ""
