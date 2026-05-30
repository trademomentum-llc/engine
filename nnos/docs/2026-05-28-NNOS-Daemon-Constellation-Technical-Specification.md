# Technical Specification: NNOS Daemon Constellation + Jasterish Validation/Morph Port

**Document ID:** NNOS-TECH-001  
**Version:** 1.0.0  
**Date:** 2026-05-28  
**Predecessors:** NNOS-REQ-001, NNOS-DS-001

---

## 1. Build System Requirements

A single top-level CMakeLists.txt is required with the following targets:

- lsa_boot_dcn, lsa_boot_hcn, lsa_boot_epn (role-specific entry points)
- SystemIntegrityDaemon, ThreatIntelligenceManager, MorphogeneticMaintainer
- Validation runtime module (JStar-linked)
- Morphogenetic repair engine (JStar port)

All targets must compile to static or position-independent binaries suitable for systemd/Podman rootless.

---

## 2. Jasterish Validation Layer API (Technical Contract)

The validation runtime exposes (via FFI or direct link):

```c
int nnos_validate_action(
    const char* action_name,
    const uint8_t* state_blob,
    size_t state_len,
    uint8_t* decision_out,   // 0 = allow, 1 = deny, 2 = escalate
    char* reason_buf,
    size_t reason_buf_len
);
```

Input state_blob is the current shared-state snapshot (canonical serialized form).
Decision must be deterministic given identical input + versioned NDPL rules.

---

## 3. Shared-State Wire Format (v1 Baseline)

Append-only log + snapshot.

Log entry:
- 8-byte timestamp (ns)
- 4-byte type
- 4-byte length
- payload
- 32-byte BLAKE3 commitment (over previous entry + this payload)

Snapshot is the folded result of the log up to a checkpoint.

Serialization: Simple TLV (Type-Length-Value) with explicit version prefix. Little-endian.

---

## 4. Bootstrap & Deployment Artifacts

Required (currently missing or incomplete):
- CMakeLists.txt (see Build section)
- systemd units or quadlet files for each daemon
- supervisord.conf for containerized runs
- Dockerfiles (multi-stage, minimal base)
- Podman quadlet definitions for rootless

Bootstrap registry stub must be populated from the 180-pattern NDPL once finalized.

---

## 5. Determinism & Measurement

- scripts/benchmark_determinism.sh must hash and time a canonical command against a fixed fixture.
- All JStar-emitted validation/morph code must pass the apps/ self-host preflight before use in NNOS daemons.
- Linux-only Jasterish ladder (with compiler.jstr present) is prerequisite for full port validation.

---

## 6. Error & Failure Modes

- Validation failure → escalate to MorphogeneticMaintainer + human notification.
- Device sync conflict → last-writer-wins with provenance annotation (never silent overwrite).
- Budget exhaustion → hard stop of new tasks + warning emission.

---

**End of Technical Specification**

This completes the full triad for the NNOS Daemon Constellation + Jasterish Port (engine/nnos/). The three documents are now present and should be linked from PROJECT_SUMMARY.md, START_HERE.md, and TODO.md.