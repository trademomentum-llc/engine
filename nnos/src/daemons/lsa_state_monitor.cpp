// lsa_state_monitor — Orin Physiological State Assessment Daemon
// Responsibility: Sample physiology at 4 Hz, compute intervention tier, publish to shared state.
#include "common/signal_handler.hpp"
#include "common/logger.hpp"
#include "common/shared_state.hpp"
#include <nnos/state_monitor.hpp>
#include <nnos/neuro_profile.hpp>
#include <thread>
#include <chrono>
#include <random>

using namespace nnos;

int main() {
    SignalHandler::install([]{});
    Logger logger("lsa_state_monitor");
    logger.info("BOOT", "State monitor starting — 4 Hz sampling");

    NeuroProfile profile = profiles::SENSORY_SOCIAL_FRAGILE;
    StateMonitor monitor(profile);

    // Stub: random generator for simulated sensor data (integer 0-255)
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<uint16_t> hr_dist(102, 230);   // ~0.4-0.9
    std::uniform_int_distribution<uint16_t> noise_dist(0, 255);
    std::uniform_int_distribution<uint16_t> light_dist(0, 255);

    while (!SignalHandler::should_shutdown()) {
        PhysiologicalState state{};
        state.heart_rate_variability = static_cast<uint8_t>(hr_dist(rng));
        state.noise_level = static_cast<uint8_t>(noise_dist(rng));
        state.light_level = static_cast<uint8_t>(light_dist(rng));
        state.notifications_count = 3;
        state.self_report_overwhelm = 4;

        InterventionTier tier = monitor.assess_and_intervene(state);

        std::string tier_str;
        switch (tier) {
            case InterventionTier::NONE: tier_str = "NONE"; break;
            case InterventionTier::ADJUST_ENVIRONMENT: tier_str = "ADJUST_ENV"; break;
            case InterventionTier::GUIDED_REGULATION: tier_str = "GUIDED_REG"; break;
            case InterventionTier::EMERGENCY_SHUTDOWN: tier_str = "EMERGENCY"; break;
        }

        uint16_t sensory_load = monitor.compute_sensory_load(state);
        logger.info("ASSESS", "tier=" + tier_str + " sensory_load=" + std::to_string(sensory_load) + "/100");

        // Publish intervention tier to shared state
        std::vector<uint8_t> payload{static_cast<uint8_t>(tier)};
        SharedState::instance().write_field(StateFieldType::INTERVENTION_TIER, payload);

        // 4 Hz = 250 ms
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    logger.info("SHUTDOWN", "State monitor stopping");
    return 0;
}
