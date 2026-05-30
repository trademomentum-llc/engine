#include "shared_state.hpp"
#include <cstring>
#include <chrono>

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

std::vector<uint8_t> SharedState::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    // Stub: return a minimal snapshot header
    std::vector<uint8_t> snap;
    snap.reserve(16);
    uint32_t ver = VERSION;
    snap.insert(snap.end(), reinterpret_cast<uint8_t*>(&ver), reinterpret_cast<uint8_t*>(&ver) + 4);
    uint64_t count = log_.size();
    snap.insert(snap.end(), reinterpret_cast<uint8_t*>(&count), reinterpret_cast<uint8_t*>(&count) + 8);
    uint32_t crc = 0; // stub CRC
    snap.insert(snap.end(), reinterpret_cast<uint8_t*>(&crc), reinterpret_cast<uint8_t*>(&crc) + 4);
    return snap;
}

} // namespace nnos
