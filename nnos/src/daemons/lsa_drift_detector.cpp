// lsa_drift_detector — Global Drift & Recovery Daemon
// Responsibility: Monitors all node state for divergence from NDPL rules, raises alerts.
#include "common/signal_handler.hpp"
#include "common/logger.hpp"
#include "common/shared_state.hpp"
#include <thread>
#include <chrono>
#include <cstdint>
#include <cmath>

using namespace nnos;

int main() {
    SignalHandler::install([]{});
    Logger logger("lsa_drift_detector");
    logger.info("BOOT", "Drift detector starting");

    float baseline_physiology[4] = { 4.2f, 3.1f, 0.0f, 7.8f }; // Mean over last 7 days

    while (!SignalHandler::should_shutdown()) {
        auto fields = SharedState::instance().read_all();

        // Compute simple drift metric: deviation from baseline
        float drift = 0.0f;
        for (const auto& f : fields) {
            if (f.type == StateFieldType::PHYSIOLOGY_SNAPSHOT) {
                for (size_t i = 0; i < 4 && i < f.payload.size(); ++i) {
                    float val = f.payload[i] / 255.0f * 10.0f;
                    drift += std::abs(val - baseline_physiology[i]);
                }
            }
        }

        if (drift > 5.0f) {
            logger.critical("DRIFT", "Physiology drift detected: " + std::to_string(drift));
        } else if (drift > 2.0f) {
            logger.warn("DRIFT", "Elevated drift: " + std::to_string(drift));
        } else {
            logger.info("HEALTH", "Drift nominal: " + std::to_string(drift));
        }

        std::this_thread::sleep_for(std::chrono::seconds(10));
    }

    logger.info("SHUTDOWN", "Drift detector stopping");
    return 0;
}
