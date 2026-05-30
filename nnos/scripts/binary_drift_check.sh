#!/bin/bash
# binary_drift_check.sh
# Phase 3, Item 9 of NEURODIOS-BIN-OPT-001
# Compares current build artifacts against a golden baseline manifest.
# Exit 0 = no drift, Exit 1 = drift detected.

set -euo pipefail

BASELINE_MANIFEST="${1:-./build/provenance/manifest.json}"
CURRENT_DIR="${2:-./build}"

if [ ! -f "$BASELINE_MANIFEST" ]; then
    echo "ERROR: Baseline manifest not found: $BASELINE_MANIFEST"
    echo "Run generate_provenance_manifest.sh first to establish a baseline."
    exit 2
fi

DRIFT_COUNT=0
echo "=== Binary Drift Detection ==="
echo "Baseline: $BASELINE_MANIFEST"
echo ""

# Extract expected hashes from baseline
while IFS= read -r line; do
    NAME=$(echo "$line" | jq -r '.name')
    EXPECTED_SHA=$(echo "$line" | jq -r '.sha256')
    EXPECTED_SIZE=$(echo "$line" | jq -r '.size_bytes')

    BINARY_PATH="$CURRENT_DIR/$NAME"
    if [ ! -f "$BINARY_PATH" ]; then
        echo "MISSING  $NAME (expected $EXPECTED_SHA)"
        DRIFT_COUNT=$((DRIFT_COUNT + 1))
        continue
    fi

    CURRENT_SHA=$(sha256sum "$BINARY_PATH" | awk '{print $1}')
    CURRENT_SIZE=$(stat -c%s "$BINARY_PATH" 2>/dev/null || stat -f%z "$BINARY_PATH")

    if [ "$CURRENT_SHA" != "$EXPECTED_SHA" ]; then
        echo "DRIFT    $NAME"
        echo "  expected: $EXPECTED_SHA ($EXPECTED_SIZE bytes)"
        echo "  actual:   $CURRENT_SHA ($CURRENT_SIZE bytes)"
        DRIFT_COUNT=$((DRIFT_COUNT + 1))
    else
        echo "OK       $NAME ($CURRENT_SIZE bytes)"
    fi
done < <(jq -c '.binaries[]' "$BASELINE_MANIFEST")

echo ""
if [ $DRIFT_COUNT -eq 0 ]; then
    echo "RESULT: No drift detected. All binaries match baseline."
    exit 0
else
    echo "RESULT: $DRIFT_COUNT binary(s) drifted from baseline."
    exit 1
fi
