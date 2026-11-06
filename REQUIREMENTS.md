# Confidential — Engine PR #8 hardening requirements

## 1. Scope and status

| Area | Requirement | Status |
| --- | --- | --- |
| Seal and amend | Failed amendments preserve the prior bytes, mtime, mode, and marker where recovery succeeds | Implemented; failure injection remains future work |
| Filesystem safety | Reject nonregular store inputs and avoid deleting a path after an identity check | Store gate implemented; existing code does not unlink in failure cleanup |
| Compatibility | Preserve existing marker parsing and command behavior | Normal path tested |
| Verification | Build C engine; run focused regression cases, telemetry tests, Python tests, and available scans | Local gates pass; hosted checks pending |

## 2. Acceptance

1. A normal seal, verify, and amend succeeds.
2. Missing, FIFO, and symlink marker or store inputs fail promptly.
3. A failed amendment returns failure and restores previous file protection and recorded metadata when filesystem recovery is possible.
4. A concurrent replacement path is never unlinked by cleanup based on a separate identity check.
5. Document unavailable platform toolchains and external CI status rather than claiming they passed.

## Appendix A. Boundary

This is a repair pass on PR #8. It does not certify the entire research platform or turn size and second-resolution mtime into a cryptographic integrity guarantee.
