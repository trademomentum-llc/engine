# Pixel 9 Pro Memory Map (Research Notes)

> This is a living document for Tensor G4 bring-up. Addresses are best-guess until verified from a device DTB or debug UART.

## Known/Assumed Regions

| Region | Base | Size | Notes |
|--------|------|------|-------|
| DRAM | 0x8000_0000 | 8-16 GB | Typical ARM64 Android load address |
| UART (SBSA) | TBD | 4 KB | Likely SBSA generic UART; base unknown |
| GICv3 Distributor | TBD | 64 KB | GIC-500 or GIC-600 |
| GICv3 Redistributor | TBD | 128 KB per CPU | Should be discoverable from DTB |
| ARM Generic Timer | CPU-local | - | CNTP/CNTV |
| UFS HCI | TBD | 4 KB | Samsung/ARM UFS controller |
| PMIC | TBD | - | Google Titan M2 or Samsung PMIC |

## Open Questions

1. What is the exact base address of the debug UART?
2. Is the bootloader locked? Can we chainload a custom kernel?
3. Which GIC version is present on Tensor G4?
4. What is the DRAM size and layout per SKU?
5. Is the framebuffer accessible without signed firmware?

## Next Steps

- Obtain or derive a Pixel 9 Pro DTB.
- Verify UART output with early `arch_putc` probes.
- Bring up ARM Generic Timer and GIC before scheduler.
