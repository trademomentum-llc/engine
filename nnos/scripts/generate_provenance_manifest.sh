#!/bin/bash
# generate_provenance_manifest.sh
# Phase 3, Item 7 of NEURODIOS-BIN-OPT-001
# Embeds or accompanies every binary with a manifest linking it to
# source commit, primitive map, and compiler version.

set -euo pipefail

BINARY_DIR="${1:-./build}"
OUTPUT_DIR="${2:-./build/provenance}"
PRIMITIVE_MAP="${3:-./stubs/primitive_map.json.stub}"

mkdir -p "$OUTPUT_DIR"

GIT_COMMIT=$(git rev-parse HEAD 2>/dev/null || echo "unknown")
GIT_BRANCH=$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo "unknown")
BUILD_HOST=$(uname -a)
BUILD_DATE=$(date -u +"%Y-%m-%dT%H:%M:%SZ")
COMPILER_VERSION=$(${CXX:-c++} --version | head -1)

# Write master manifest
cat > "$OUTPUT_DIR/manifest.json" <<EOF
{
  "schema_version": "1.0",
  "build_date": "$BUILD_DATE",
  "git_commit": "$GIT_COMMIT",
  "git_branch": "$GIT_BRANCH",
  "build_host": "$BUILD_HOST",
  "compiler": "$COMPILER_VERSION",
  "primitive_map_ref": "$PRIMITIVE_MAP",
  "binaries": [
EOF

FIRST=1
for bin in "$BINARY_DIR"/lsa_* "$BINARY_DIR"/*Daemon "$BINARY_DIR"/*Manager; do
    [ -f "$bin" ] || continue
    NAME=$(basename "$bin")
    SHA256=$(sha256sum "$bin" | awk '{print $1}')
    SIZE=$(stat -c%s "$bin" 2>/dev/null || stat -f%z "$bin")
    TYPE=$(file -b "$bin" | sed 's/"/\\"/g')

    if [ $FIRST -eq 1 ]; then
        FIRST=0
    else
        echo "," >> "$OUTPUT_DIR/manifest.json"
    fi

    cat >> "$OUTPUT_DIR/manifest.json" <<EOF
    {
      "name": "$NAME",
      "sha256": "$SHA256",
      "size_bytes": $SIZE,
      "file_type": "$TYPE"
    }
EOF
done

echo "" >> "$OUTPUT_DIR/manifest.json"
echo "  ]" >> "$OUTPUT_DIR/manifest.json"
echo "}" >> "$OUTPUT_DIR/manifest.json"

# Also write a flat sha256sums.txt for quick verification
jq -r '.binaries[] | .sha256 + "  " + .name' "$OUTPUT_DIR/manifest.json" > "$OUTPUT_DIR/sha256sums.txt"

echo "Provenance manifest written to $OUTPUT_DIR/manifest.json"
echo "SHA256 flat file written to $OUTPUT_DIR/sha256sums.txt"
