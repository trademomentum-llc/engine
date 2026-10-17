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

# security-scan must still scan symlinked source files (review PRRT_kwDOSsdSEM6o55Zt)
mkdir -p "$test_dir/scanproj"
printf 'x = eval(user_input)\n' > "$test_dir/scan-target.py"
ln -s "$test_dir/scan-target.py" "$test_dir/scanproj/link.py"
if ! (cd "$test_dir" && "$engine_bin" run security-scan "$test_dir/scanproj" 2>/dev/null) | grep -q '(1 issues'; then
    echo 'symlinked source file was not scanned' >&2
    exit 1
fi
echo 'security-scan symlink regression: pass'

# code-check (and every recipe reader) must read symlinked sources (PRRT_kwDOSsdSEM6tcdNG)
mkdir -p "$test_dir/ccproj"
printf 'def f():\n    return 1\n' > "$test_dir/cc-target.py"
ln -s "$test_dir/cc-target.py" "$test_dir/ccproj/link.py"
cc_out="$(cd "$test_dir" && "$engine_bin" run code-check "$test_dir/ccproj" 2>&1 || true)"
if ! printf '%s\n' "$cc_out" | grep -Eq '(^|[^0-9])1 files?|files[^0-9]*1'; then
    echo "code-check did not analyze the symlinked source: $cc_out" >&2
    exit 1
fi
echo 'code-check symlink regression: pass'

# over-long project roots fail explicitly instead of truncating child paths (PRRT_kwDOSsdSEM6tcdNJ)
long_root="$test_dir"
while [ "${#long_root}" -lt 900 ]; do long_root="$long_root/dddddddddddddddddddddddddddddddddddddddd"; done
mkdir -p "$long_root"
if (cd "$test_dir" && "$engine_bin" run security-scan "$long_root" >/dev/null 2>&1); then
    echo 'over-long project root was accepted' >&2
    exit 1
fi
echo 'long-path regression: pass'

# an empty (voided) canonical marker must not shadow a valid legacy marker (PRRT_kwDOSsdSEM6o5gC8 / tcdNO / tcdNS)
printf 'legacy\n' > "$test_dir/legacy-real"
ln -s "$test_dir/legacy-real" "$test_dir/legacy-link"
"$engine_bin" seal "$test_dir/legacy-link"
# Turn the canonical marker into a legacy (as-passed spelling) marker and
# leave an empty canonical marker behind, as a voided rollback would.
mv "$test_dir/legacy-real.sealed" "$test_dir/legacy-link.sealed"
: > "$test_dir/legacy-real.sealed"
"$engine_bin" verify "$test_dir/legacy-link"
if "$engine_bin" verify "$test_dir/legacy-real" 2>/dev/null; then
    echo 'empty canonical marker verified without a legacy marker' >&2
    exit 1
fi
echo 'empty-marker regression: pass'
