#!/bin/bash
# build_jmk_docker.sh
# Builds the Jasterish Micro-Kernel inside a Docker container.
# This resolves the cross-compilation gap on non-Linux hosts.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
JMK_DIR="${SCRIPT_DIR}/../neurodios/jasterish-microkernel"
APPS_JSTAR="${SCRIPT_DIR}/../../../../apps/target/release/jstar"

echo "=== JMK Docker Build ==="
echo "JMK source: $JMK_DIR"

# Check for Docker
if ! command -v docker &> /dev/null; then
    echo "ERROR: Docker is required but not installed."
    exit 1
fi

# Build the builder image
docker build -f "$JMK_DIR/Dockerfile.build" -t jmk-builder "$JMK_DIR"

# Mount jstar from apps/ if available, otherwise assume it's in PATH
JSTAR_MOUNT=""
if [ -f "$APPS_JSTAR" ]; then
    echo "Using jstar from: $APPS_JSTAR"
    JSTAR_MOUNT="-v $APPS_JSTAR:/usr/local/bin/jstar"
else
    echo "WARNING: jstar not found at $APPS_JSTAR"
    echo "Ensure jstar is in your PATH inside the container."
fi

# Run the build
docker run --rm \
    $JSTAR_MOUNT \
    -v "$JMK_DIR:/src" \
    -w /src \
    jmk-builder \
    make JASTERISH_COMPILER=/usr/local/bin/jstar build

echo "=== Build complete ==="
echo "Artifacts in: $JMK_DIR"
ls -la "$JMK_DIR"/jmk.* 2>/dev/null || true
