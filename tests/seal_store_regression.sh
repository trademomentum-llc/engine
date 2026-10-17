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

# fail closed: an invalid canonical marker must not fall back to the legacy marker (PRRT_kwDOSsdSEM6tcoqD)
for kind in fifo symlink unreadable dir; do
    rm -rf "$test_dir/legacy-real.sealed"
    case "$kind" in
        fifo) mkfifo "$test_dir/legacy-real.sealed" ;;
        symlink) ln -s "$test_dir/legacy-link.sealed" "$test_dir/legacy-real.sealed" ;;
        unreadable) printf 'x\n' > "$test_dir/legacy-real.sealed"; chmod 000 "$test_dir/legacy-real.sealed" ;;
        dir) mkdir "$test_dir/legacy-real.sealed" ;;
    esac
    if [ "$kind" = unreadable ] && [ -r "$test_dir/legacy-real.sealed" ]; then
        continue  # running as root: permission bits do not block reads
    fi
    if timeout 5 "$engine_bin" verify "$test_dir/legacy-link" 2>/dev/null; then
        echo "invalid canonical marker ($kind) fell back to the legacy marker" >&2
        exit 1
    fi
done
rm -rf "$test_dir/legacy-real.sealed"
"$engine_bin" verify "$test_dir/legacy-link"
echo 'fail-closed marker regression: pass'

# rollback must still truncate and report incomplete recovery when its size check fails (PRRT_kwDOSsdSEM6tivXV)
repo_root="$(cd "$(dirname "$0")/.." && pwd)"
fault_bin="$test_dir/engine-fault"
cc -std=c11 -O2 -DSEAL_FAULT_INJECTION -I "$repo_root/include" -o "$fault_bin" \
    $(sed -n '/^SRC/,/^TARGET/p' "$repo_root/Makefile" | grep -o '[a-z_/]*\.c' | sed "s|^|$repo_root/|")
printf 'rollback-fstat\n' > "$test_dir/rf"
"$fault_bin" seal "$test_dir/rf"
rf_before="$(cat "$test_dir/rf")"
rf_err="$(SEAL_FAULT_AFTER_APPEND=1 SEAL_FAULT_ROLLBACK_FSTAT=1 timeout 3 "$fault_bin" amend "$test_dir/rf" 'should-not-land' 2>&1 || true)"
if [ "$(cat "$test_dir/rf")" != "$rf_before" ]; then
    echo 'rollback skipped truncation after a failed size check' >&2
    exit 1
fi
if ! printf '%s\n' "$rf_err" | grep -q 'recovery incomplete'; then
    echo "rollback did not report incomplete recovery: $rf_err" >&2
    exit 1
fi
echo 'rollback size-check regression: pass'

# a shrink by another writer during amend must be reported as incomplete recovery (PRRT_kwDOSsdSEM6tjKJi)
printf 'shrink-check-original\n' > "$test_dir/sh"
"$fault_bin" seal "$test_dir/sh"
sh_err="$(SEAL_FAULT_AFTER_APPEND=1 SEAL_FAULT_SHRINK_BEFORE_ROLLBACK=1 timeout 3 "$fault_bin" amend "$test_dir/sh" 'x' 2>&1 || true)"
if ! printf '%s\n' "$sh_err" | grep -q 'recovery incomplete'; then
    echo "shrunk file during rollback was not flagged: $sh_err" >&2
    exit 1
fi
if [ "$(tr -d '\000' < "$test_dir/sh" | wc -c)" != "$(wc -c < "$test_dir/sh")" ]; then
    echo 'rollback zero-extended a shrunk file' >&2
    exit 1
fi
echo 'rollback shrink regression: pass'

# a failed reseal must leave the existing valid marker intact (PRRT_kwDOSsdSEM6tjKJl)
printf 'reseal-check\n' > "$test_dir/rs"
"$fault_bin" seal "$test_dir/rs"
rs_marker_before="$(cat "$test_dir/rs.sealed")"
if SEAL_FAULT_DATA_FCHMOD=1 "$fault_bin" seal "$test_dir/rs" 2>/dev/null; then
    echo 'reseal succeeded despite a failed data chmod' >&2
    exit 1
fi
if [ "$(cat "$test_dir/rs.sealed")" != "$rs_marker_before" ]; then
    echo 'failed reseal modified the existing marker' >&2
    exit 1
fi
if ls "$test_dir"/rs.sealed.tmp.* >/dev/null 2>&1; then
    echo 'failed reseal left a temporary marker behind' >&2
    exit 1
fi
"$engine_bin" verify "$test_dir/rs"
# a failed first seal must not publish any marker
printf 'first-seal\n' > "$test_dir/fs"
if SEAL_FAULT_DATA_FCHMOD=1 "$fault_bin" seal "$test_dir/fs" 2>/dev/null; then exit 1; fi
if [ -e "$test_dir/fs.sealed" ] || "$engine_bin" verify "$test_dir/fs" 2>/dev/null; then
    echo 'failed first seal left a verifiable marker' >&2
    exit 1
fi
echo 'reseal marker-preservation regression: pass'

# verify must work through an execute-only (search-only) parent directory (PRRT_kwDOSsdSEM6tjKJn)
mkdir -p "$test_dir/xonly/inner"
printf 'x-only\n' > "$test_dir/xonly/inner/f"
"$engine_bin" seal "$test_dir/xonly/inner/f"
chmod 111 "$test_dir/xonly"
if ! "$engine_bin" verify "$test_dir/xonly/inner/f"; then
    chmod 755 "$test_dir/xonly"
    echo 'verify rejected a sealed file under an execute-only directory' >&2
    exit 1
fi
chmod 755 "$test_dir/xonly"
echo 'execute-only directory regression: pass'
