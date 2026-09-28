#!/usr/bin/env bash
set -euo pipefail

engine_bin="$(cd "$(dirname "$0")/.." && pwd)/engine"
test_dir="$(mktemp -d)"
trap 'chmod -R u+rwX "$test_dir" 2>/dev/null || true; rm -rf "$test_dir"' EXIT

printf 'original\n' > "$test_dir/data"
"$engine_bin" seal "$test_dir/data"
"$engine_bin" verify "$test_dir/data"
"$engine_bin" amend "$test_dir/data" 'append-check'
"$engine_bin" verify "$test_dir/data"
grep -q 'append-check' "$test_dir/data"

chmod u+w "$test_dir/data"
printf 'changed!\n' > "$test_dir/data"
if "$engine_bin" verify "$test_dir/data" 2>/dev/null; then
    echo 'tampered data was accepted' >&2
    exit 1
fi

mkfifo "$test_dir/no-writer"
if timeout 3 "$engine_bin" seal "$test_dir/no-writer" 2>/dev/null; then
    echo 'FIFO was sealed' >&2
    exit 1
fi
echo 'seal regression: pass'
