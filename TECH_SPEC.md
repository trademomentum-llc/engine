# Confidential — Engine PR #8 technical verification

## 1. Implementation

1. `src/seal.c`: retain the verified data descriptor; check every append operation; mark rollback necessary before writes; restore timestamps after truncation; restore POSIX mode before Linux immutability; report recovery errors.
2. `src/store.c`: nonblocking read open, `fstat` regular-file gate, checked stream close.
3. Tests: exercise normal seal/verify/amend and hostile marker/store object types in an isolated temporary directory.

## 2. Gates

| Gate | Command | Status |
| --- | --- | --- |
| Main C build | `make` | Passes with 34 existing path-truncation warnings |
| Sanitized C build | `make CFLAGS='-std=c11 -I include -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer' LDFLAGS='-fsanitize=address,undefined'` | Passes; focused tests pass with `ASAN_OPTIONS=detect_leaks=0` (local LeakSanitizer cannot inspect processes) |
| Telemetry | `make -C telemetry test` | Passes, 7 cases |
| Python | `PYTHONPATH=nnos python3 -m unittest nnos.neurobalance.test_coordinator_efficiency` | Passes, 7 cases |
| NNOS CMake | `cmake -S nnos -B build` | Blocked locally: CMake unavailable |
| Hosted OSSAR | GitHub PR workflow | Pending external result |

## Appendix A. Go/no-go

Commit only after the available local gates pass and the remote head remains at the inspected SHA. No assertion that the next Copilot review will be finding-free.
