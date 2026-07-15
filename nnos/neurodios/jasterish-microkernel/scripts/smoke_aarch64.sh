#!/bin/bash
# AArch64 QEMU smoke test for the Jasterish Micro-Kernel.
# Builds the kernel and runs it headlessly in qemu-system-aarch64, then
# checks the serial output for a boot marker and a shell prompt.

set -euo pipefail

cd "$(dirname "$0")/.."

LOG_FILE="${JMK_SMOKE_LOG:-/tmp/jmk_aarch64_smoke.log}"
rm -f "$LOG_FILE"

echo "[smoke] Cleaning previous AArch64 build artifacts..."
make ARCH=aarch64 clean

echo "[smoke] Building AArch64 kernel..."
make ARCH=aarch64 build

echo "[smoke] Running kernel in QEMU (timeout 10s)..."
timeout 10 qemu-system-aarch64 \
  -machine virt \
  -cpu cortex-a72 \
  -m 512 \
  -serial stdio \
  -no-reboot \
  -no-shutdown \
  -kernel jmk.bin \
  -display none > "$LOG_FILE" 2>&1 || true

echo "[smoke] Checking serial output..."
BOOT_OK=0
PROMPT_OK=0

if grep -iq "BOOT" "$LOG_FILE"; then
    BOOT_OK=1
fi

# The common kernel prints the prompt as "JMK> "; accept case-insensitive match.
if grep -iq "JMK>" "$LOG_FILE"; then
    PROMPT_OK=1
fi

if [[ "$BOOT_OK" -eq 1 && "$PROMPT_OK" -eq 1 ]]; then
    echo "[PASS] AArch64 smoke test"
    exit 0
else
    echo "[FAIL] AArch64 smoke test"
    echo "---- captured output ----"
    cat "$LOG_FILE"
    echo "-------------------------"
    exit 1
fi
