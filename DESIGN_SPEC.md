# Confidential — Engine PR #8 repair design

## 1. First principles

The primitives are a data inode, a marker inode, held descriptors, mutable directory entries, timestamps, permission bits, and optional Linux immutable flags. A path lookup and a later unlink are separate operations. An append can partially succeed before an error is reported.

## 2. State transitions

| State | Action | Failure response |
| --- | --- | --- |
| Verified | Hold data descriptor and capture size, timestamps, mode, and marker bytes | Leave all objects untouched |
| Writable | Clear optional immutable flag; change mode on held descriptor | Restore original mode and flag |
| Appended | Write and flush through checked stream | Truncate held data descriptor and restore timestamp |
| Re-sealed | Rewrite held marker descriptor | Restore saved marker bytes and mode |
| Protected | Restore data mode before immutable flag | Report failure if recovery itself fails |

## 3. Security boundaries

Never delete a marker by pathname after a separate identity check. Fail closed on nonregular objects before buffered reads. Descriptor checks reduce replacement risk, while second-resolution size and mtime remain weak against deliberate modification; a cryptographic digest and atomic journaled protocol require a separate design.

## Appendix A. Risk

| Risk | Likelihood × impact | Gate |
| --- | --- | --- |
| Partial append after I/O error | Medium × high | Fault-path test and checked writes |
| Attacker-controlled directory replacement | Medium × high | No checked-then-unlink cleanup |
| Unsupported immutable ioctl | Medium × medium | Best-effort on initial seal; restore state on amend |
