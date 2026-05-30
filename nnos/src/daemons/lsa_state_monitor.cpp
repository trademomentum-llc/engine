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

    // Stub: random generator for simulated sensor data
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> hr_dist(0.4f, 0.9f);
    std::uniform_real_distribution<float> noise_dist(0.0f, 1.0f);
    std::uniform_real_distribution<float> light_dist(0.0f, 1.0f);

    while (!SignalHandler::should_shutdown()) {
        PhysiologicalState state{};
        state.heart_rate_variability = hr_dist(rng);
        state.noise_level = noise_dist(rng);
        state.light_level = light_dist(rng);
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

        float sensory_load = state.noise_level * 0.4f + state.light_level * 0.3f + (state.notifications_count / 100.0f) * 0.3f;
        logger.info("ASSESS", "tier=" + tier_str + " sensory_load=" + std::to_string(sensory_load));

        // Publish intervention tier to shared state
        std::vector<uint8_t> payload{static_cast<uint8_t>(tier)};
        SharedState::instance().write_field(StateFieldType::INTERVENTION_TIER, payload);

        // 4 Hz = 250 ms
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    logger.info("SHUTDOWN", "State monitor stopping");
    return 0;
}
