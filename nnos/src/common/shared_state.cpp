#include "shared_state.hpp"
#include <cstring>
#include <chrono>
#include <unordered_set>

namespace nnos {

SharedState& SharedState::instance() {
    static SharedState inst;
    return inst;
}

bool SharedState::write_field(StateFieldType type, const std::vector<uint8_t>& payload) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto now = std::chrono::steady_clock::now().time_since_epoch();
    uint64_t ts = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());
    if (ts <= last_timestamp_) ts = last_timestamp_ + 1;
    last_timestamp_ = ts;

    log_.push_back(StateField{
        .type = type,
        .timestamp_ns = ts,
        .payload = payload,
    });

    // Truncate old entries beyond a reasonable window (stub: keep last 10k)
    if (log_.size() > 10000) {
        log_.erase(log_.begin(), log_.begin() + (log_.size() - 10000));
    }

    return true;
}

bool SharedState::merge_field(const StateField& remote) {
    std::lock_guard<std::mutex> lock(mutex_);
    // Reject if local has a newer or equal entry of the same type
    for (auto it = log_.rbegin(); it != log_.rend(); ++it) {
        if (it->type == remote.type) {
            if (it->timestamp_ns >= remote.timestamp_ns) {
                return false;
            }
            break;
        }
    }
    log_.push_back(remote);
    if (log_.size() > 10000) {
        log_.erase(log_.begin(), log_.begin() + (log_.size() - 10000));
    }
    return true;
}

bool SharedState::read_latest(StateFieldType type, StateField& out) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = log_.rbegin(); it != log_.rend(); ++it) {
        if (it->type == type) {
            out = *it;
            return true;
        }
    }
    return false;
}

std::vector<StateField> SharedState::read_all() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return log_;
}

namespace {

// FNV-1a 32-bit hash for deterministic checksums
static uint32_t fnv1a_32(const uint8_t* data, size_t len) {
    uint32_t hash = 0x811c9dc5;
    for (size_t i = 0; i < len; ++i) {
        hash ^= data[i];
        hash *= 0x01000193;
    }
    return hash;
}

static void append_le32(std::vector<uint8_t>& buf, uint32_t val) {
    buf.push_back(static_cast<uint8_t>(val));
    buf.push_back(static_cast<uint8_t>(val >> 8));
    buf.push_back(static_cast<uint8_t>(val >> 16));
    buf.push_back(static_cast<uint8_t>(val >> 24));
}

static void append_le64(std::vector<uint8_t>& buf, uint64_t val) {
    buf.push_back(static_cast<uint8_t>(val));
    buf.push_back(static_cast<uint8_t>(val >> 8));
    buf.push_back(static_cast<uint8_t>(val >> 16));
    buf.push_back(static_cast<uint8_t>(val >> 24));
    buf.push_back(static_cast<uint8_t>(val >> 32));
    buf.push_back(static_cast<uint8_t>(val >> 40));
    buf.push_back(static_cast<uint8_t>(val >> 48));
    buf.push_back(static_cast<uint8_t>(val >> 56));
}

} // anonymous namespace

std::vector<uint8_t> SharedState::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);

    // Fold log: keep only the latest entry per field type
    std::vector<StateField> folded;
    folded.reserve(log_.size());
    std::unordered_set<uint32_t> seen;
    for (auto it = log_.rbegin(); it != log_.rend(); ++it) {
        uint32_t type_val = static_cast<uint32_t>(it->type);
        if (seen.insert(type_val).second) {
            folded.push_back(*it);
        }
    }
    std::reverse(folded.begin(), folded.end());

    // Serialize snapshot v1: [version][count][entries...][crc32]
    std::vector<uint8_t> snap;
    snap.reserve(8 + folded.size() * 16);

    append_le32(snap, VERSION);
    append_le32(snap, static_cast<uint32_t>(folded.size()));

    for (const auto& f : folded) {
        append_le32(snap, static_cast<uint32_t>(f.type));
        append_le64(snap, f.timestamp_ns);
        append_le32(snap, static_cast<uint32_t>(f.payload.size()));
        snap.insert(snap.end(), f.payload.begin(), f.payload.end());
    }

    uint32_t crc = fnv1a_32(snap.data(), snap.size());
    append_le32(snap, crc);
    return snap;
}

} // namespace nnos
