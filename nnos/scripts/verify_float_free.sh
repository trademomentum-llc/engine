#!/bin/bash
# verify_float_free.sh — Regression gate for Efficiency Mandate in C/C++
# Exits 0 if no float/double/cmath usage found in src/ and include/
# Exits 1 if any found, printing offending lines.
set -euo pipefail

cd "$(dirname "$0")/.."

echo "Scanning src/ and include/ for float/double/cmath/cfloat usage..."

# Grep for float/double/cmath, excluding comments and __attribute__ markers
violations=$(grep -rn 'float\|double\|#include <cmath>\|#include <cfloat>\|#include <math.h>' src/ include/ | grep -v '//.*float\|//.*double\|__attribute__' || true)

if [[ -n "$violations" ]]; then
    echo "FAIL: Efficiency Mandate violation detected — float/double/cmath usage found:"
    echo "$violations"
    exit 1
else
    echo "PASS: No float/double/cmath usage in src/ or include/."
    exit 0
fi
