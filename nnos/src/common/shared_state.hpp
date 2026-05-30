#pragma once
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <memory>

namespace nnos {

// ============================================================================
// Shared-State TLV Wire Format (v1 Baseline)
// ============================================================================
// Log entry:
//   8-byte timestamp (ns)
//   4-byte type
//   4-byte length
//   payload
//   32-byte BLAKE3 commitment (over previous entry + this payload)
//
// Snapshot is the folded result of the log up to a checkpoint.
// Little-endian.
// ============================================================================

enum class StateFieldType : uint32_t {
    PHYSIOLOGY_SNAPSHOT      = 0x01,
    TASK_BUDGET_REMAINING    = 0x02,
    CONTEXT_SWITCH_COUNT     = 0x03,
    THREAT_SCORE             = 0x04,
    MORPH_CYCLE_ID           = 0x05,
    ORIGIN_VAULT_REF         = 0x06,
    DAEMON_HEARTBEAT         = 0x07,
    PROFILE_STATE            = 0x08,
    INTERVENTION_TIER        = 0x09,
    DRIFT_SCORE              = 0x0A,
    SNAPSHOT_HASH            = 0x0B,
    // Additive only until v2.0
};

struct StateField {
    StateFieldType type;
    uint64_t timestamp_ns;
    std::vector<uint8_t> payload;
};

// Minimal in-memory shared state (mmap-backed or SHM in production)
class SharedState {
public:
    static SharedState& instance();

    // Write a field to the append-only log
    bool write_field(StateFieldType type, const std::vector<uint8_t>& payload);

    // Read the latest field of a given type
    bool read_latest(StateFieldType type, StateField& out);

    // Merge a remote field if its timestamp is newer than the latest local entry
    // of the same type. Preserves append-only log semantics.
    bool merge_field(const StateField& remote);

    // Read all fields (for iteration / counting / scanning)
    std::vector<StateField> read_all() const;

    // Fold log into a snapshot (stub — returns fixed-size buffer)
    std::vector<uint8_t> snapshot() const;

    // Version prefix for wire compatibility
    static constexpr uint32_t VERSION = 0x00010000; // v1.0.0

private:
    SharedState() = default;
    mutable std::mutex mutex_;
    std::vector<StateField> log_;
    std::atomic<uint64_t> last_timestamp_{0};
};

// Capability token for daemon-to-daemon IPC
struct CapabilityToken {
    uint64_t issued_at;
    uint64_t expires_at;
    uint32_t permissions; // bitfield
    uint8_t signature[32]; // HMAC-SHA256 stub
};

} // namespace nnos
