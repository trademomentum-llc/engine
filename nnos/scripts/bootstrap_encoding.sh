#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="${1:-.}"
SCANNED=0
NORMALIZED=0

if [ ! -d "${ROOT_DIR}" ]; then
    printf 'error: directory not found: %s\n' "${ROOT_DIR}" >&2
    exit 1
fi

if ! command -v perl >/dev/null 2>&1; then
    printf 'error: perl is required to normalize file encodings\n' >&2
    exit 1
fi

normalize_file() {
    local path="$1"
    local tmp

    tmp="$(mktemp "${TMPDIR:-/tmp}/nnos-encoding.XXXXXX")"
    perl -0pe 's/^\xEF\xBB\xBF//; s/\r\n/\n/g; s/\r/\n/g' "${path}" > "${tmp}"

    if ! cmp -s "${path}" "${tmp}"; then
        mv "${tmp}" "${path}"
        NORMALIZED=$((NORMALIZED + 1))
        printf 'normalized %s\n' "${path}"
    else
        rm -f "${tmp}"
    fi
}

while IFS= read -r -d '' path; do
    SCANNED=$((SCANNED + 1))
    normalize_file "${path}"
done < <(
    find "${ROOT_DIR}" -type f \
        \( -name '*.c' -o -name '*.cc' -o -name '*.cpp' -o -name '*.cmake' \
        -o -name '*.h' -o -name '*.hpp' -o -name '*.ini' -o -name '*.json' \
        -o -name '*.md' -o -name '*.service' -o -name '*.sh' -o -name '*.txt' \
        -o -name '*.yaml' -o -name '*.yml' \) \
        -print0
)

printf 'scanned=%d normalized=%d\n' "${SCANNED}" "${NORMALIZED}"
