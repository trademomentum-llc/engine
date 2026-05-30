// lsa_drift_detector — Global Drift & Recovery Daemon
// Responsibility: Monitors all node state for divergence from NDPL rules, raises alerts.
// Efficiency Mandate: zero floats in the hot path — all arithmetic is integer-only.
#include "common/signal_handler.hpp"
#include "common/logger.hpp"
#include "common/shared_state.hpp"
#include <thread>
#include <chrono>
#include <cstdint>

using namespace nnos;

// Baseline physiology values (0–10 scale) scaled by 255 to eliminate division.
// Original floats: {4.2, 3.1, 0.0, 7.8}
static constexpr uint32_t BASELINE_RAW[4] = { 1071, 791, 0, 1989 };
static constexpr uint32_t THRESH_CRIT = 1275; // 5.0 * 255
static constexpr uint32_t THRESH_WARN = 510;  // 2.0 * 255

static inline uint32_t abs_delta(uint32_t a, uint32_t b) {
    return (a > b) ? (a - b) : (b - a);
}

int main() {
    SignalHandler::install([]{});
    Logger logger("lsa_drift_detector");
    logger.info("BOOT", "Drift detector starting (integer-only path)");

    while (!SignalHandler::should_shutdown()) {
        auto fields = SharedState::instance().read_all();

        // Compute simple drift metric: deviation from baseline (scaled by 255)
        uint32_t drift = 0;
        for (const auto& f : fields) {
            if (f.type == StateFieldType::PHYSIOLOGY_SNAPSHOT) {
                for (size_t i = 0; i < 4 && i < f.payload.size(); ++i) {
                    uint32_t val = static_cast<uint32_t>(f.payload[i]) * 10;
                    drift += abs_delta(val, BASELINE_RAW[i]);
                }
            }
        }

        if (drift > THRESH_CRIT) {
            logger.critical("DRIFT", "Physiology drift detected: " + std::to_string(drift) + "/2550");
        } else if (drift > THRESH_WARN) {
            logger.warn("DRIFT", "Elevated drift: " + std::to_string(drift) + "/2550");
        } else {
            logger.info("HEALTH", "Drift nominal: " + std::to_string(drift) + "/2550");
        }

        std::this_thread::sleep_for(std::chrono::seconds(10));
    }

    logger.info("SHUTDOWN", "Drift detector stopping");
    return 0;
}
