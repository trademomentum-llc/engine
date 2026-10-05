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
# A sealed file whose marker is swapped for a FIFO must make amend fail
# promptly (no blocking open), and the data file must be left untouched.
printf 'fifo-marker\n' > "$test_dir/fm"
"$engine_bin" seal "$test_dir/fm"
before="$(cat "$test_dir/fm")"
chmod u+w "$test_dir/fm.sealed" "$test_dir"
rm -f "$test_dir/fm.sealed"
mkfifo "$test_dir/fm.sealed"
if timeout 3 "$engine_bin" amend "$test_dir/fm" 'should-not-land' 2>/dev/null; then
    echo 'amend accepted a FIFO marker' >&2
    exit 1
fi
rc=0
timeout 3 "$engine_bin" amend "$test_dir/fm" 'should-not-land' 2>/dev/null || rc=$?
if [ "$rc" -eq 124 ]; then
    echo 'amend blocked on a FIFO marker' >&2
    exit 1
fi
if [ "$(cat "$test_dir/fm")" != "$before" ]; then
    echo 'amend modified data despite failing' >&2
    exit 1
fi
echo 'seal regression: pass'
