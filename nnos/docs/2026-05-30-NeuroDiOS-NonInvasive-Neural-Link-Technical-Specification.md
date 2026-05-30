# NeuroDiOS Non-Invasive Neural Link (NINL) Subsystem — Technical Specification

**Document ID:** NEURODIOS-NINL-TEC-001  
**Version:** 1.0.0  
**Date:** 2026-05-30  
**Status:** Authoritative Baseline  
**Related:** NEURODIOS-NINL-REQ-001, NEURODIOS-NINL-DES-001

---

## 1. Scope

This Technical Specification defines the concrete interfaces, data formats, deterministic algorithms, minimal type usage, and safety mechanisms required to implement the Non-Invasive Neural Link Subsystem in a manner consistent with the Requirements and Design.

It is intentionally narrow for v1.0: receive-only, non-invasive electromagnetic resonance primary modality, deterministic integer/fixed-point heavy pipeline, strict integration with NeuroBalance and the Roller Coaster governor.

---

## 2. Hardware Interface (Sensor Head ↔ Host)

### 2.1 Physical / Link Layer

- Recommended initial transport: USB 2.0 High Speed or Bluetooth LE 5+ with isochronous channels.
- Minimum sustained sample rate: 500 Hz per channel (target 1–2 kHz for resonant modalities).
- Packet format: Little-endian, 24-bit or 32-bit signed samples, 8–64 channels depending on head density.
- Every packet carries:
  - 64-bit monotonic hardware sample counter
  - 32-bit wall-clock timestamp (microseconds since boot of sensor head)
  - CRC-32C
  - Pipeline version / firmware hash (8 bytes)

### 2.2 Power & Safety Envelope (Hardware Enforced)

The sensor head firmware SHALL enforce at the hardware level:
- Maximum average radiated power over any 1-second window.
- Maximum peak instantaneous power.
- Maximum continuous transmission time before mandatory quiet period.

These limits are not software-configurable beyond a factory calibration constant that is itself under Origin Vault.

---

## 3. Host-Side Data Formats

### 3.1 Raw Neural Window

```c
struct alignas(64) NeuralRawWindow {
    uint64_t window_id;
    uint64_t start_sample_counter;
    uint32_t sample_rate_hz;           // e.g. 1000
    uint16_t channel_count;
    uint16_t samples_per_channel;
    uint8_t  modality_id;              // 0 = EM Resonance primary, 1 = dry EEG, 2 = fNIRS, etc.
    uint8_t  reserved[3];
    // Followed by packed samples (int32_t or int24_t packed)
};
```

All subsequent processing operates on windows of this form or their fixed-point transformed equivalents.

### 3.2 Feature Vector (Minimal Representation)

After deterministic preprocessing, every window is reduced to a feature vector using only Intensity8, Delta8, and FixedQ7_9 types:

- Band energies (canonical + user resonant bands): FixedQ7_9
- Statistical moments (mean, variance, skewness proxy): Intensity8 / Delta8 after scaling
- Phase locking / cross-channel coherence: FixedQ7_9
- Signal quality metric: Intensity8

This keeps the hot interpretation path inside the Efficiency Mandate.

### 3.3 Command / Prompt Token

```c
struct NeuralIntentToken {
    uint64_t timestamp_us;
    uint8_t  token_type;           // 0 = discrete command, 1 = prompt fragment
    uint8_t  confidence;           // Intensity8
    uint16_t command_id;           // for discrete commands
    uint32_t prompt_hash_prefix;   // first 32 bits of the prompt content hash
    uint64_t pipeline_version_hash;
    uint64_t raw_window_id;
};
```

---

## 4. Deterministic Signal Processing Requirements

All stages after the initial physics-specific filtering must be implementable with integer and fixed-point arithmetic only.

Recommended approach for v1.0:
- Use FixedQ7_9 (signed 16-bit, 9 fractional bits, range ≈ ±64) for most amplitude and energy calculations.
- Use Intensity8 (0–255) for normalized loads and confidences.
- Where true floating point is required (e.g., initial FFT or resonant frequency solving), isolate it in a versioned, auditable module whose output is immediately requantized to fixed-point before any command generation.

Every stage must be accompanied by a small integer proof or comment showing the scaling math that preserves required precision.

---

## 5. Integration with NeuroBalance & Roller Coaster

The Neural Link Governor (user-space component) must expose at minimum these calls (or equivalent event interface):

```c
NeuralLinkStatus query_link_status();
bool request_session_start(uint32_t requested_duration_s, uint8_t max_bandwidth);
void report_neural_load(uint8_t neural_load_intensity8, uint8_t fluctuation_velocity);
void force_coast(uint32_t minimum_coast_seconds);
```

The NeuroBalance Coordinator is the sole authority on whether `request_session_start` may succeed and what parameters are allowed.

The Roller Coaster governor treats `neural_load_intensity8` as one of the primary high-bandwidth input channels and will schedule mandatory coasting periods accordingly.

---

## 6. Versioning & Provenance

- Every firmware image on the sensor head carries a 32-byte content hash.
- Every host pipeline version is identified by a 64-bit hash of its source + compiled artifact.
- Calibration artifacts are stored as versioned, signed, immutable blobs.
- All of the above are registered in the system Origin Vault.

---

## 7. Error & Failure Modes (Mandatory Handling)

- Loss of sensor head communication → immediate zero-output + user-visible alert + NeuroBalance notification.
- Signal quality collapse → automatic session suspension.
- NeuroBalance veto or high intervention tier → hardware power cut within one control loop (target < 50 ms).
- Pipeline version mismatch between sensor head and host → hard refusal to start session.

---

## 8. Minimal Implementation Path for v1.0

1. Single-channel or low-channel-count EM resonance prototype head with hard power limiting.
2. Host USB receiver + basic windowing + FixedQ7_9 bandpass energy extraction.
3. Small set of user-calibrated discrete command templates (e.g., 4–8 intentional mental gestures).
4. Direct injection of resulting tokens into the existing NeuroDiOS shared state.
5. Full bidirectional integration with the live NeuroBalance Coordinator and Roller Coaster governor.
6. Complete audit trail for every token under the Origin Vault.

Anything beyond this (high-density arrays, sophisticated learned models, bidirectional stimulation, kernel-level driver) is explicitly Phase 2+ and must re-demonstrate all safety invariants.

---

**End of Technical Specification (v1.0 Baseline)**

Together with the Requirements and Design Specification, this document completes the authoritative triad for the NeuroDiOS Non-Invasive Neural Link Subsystem. No hardware development, firmware, or production signal processing code should be written against this module until the triad has been reviewed, accepted, and cross-referenced from the Chained Source of Truth bindings.