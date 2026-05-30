// lsa_profile_refiner — M1 Profile Matching & Refinement Daemon
// Responsibility: Daily batch inference on 30-day logs, match observed behavior to templates.
#include "common/signal_handler.hpp"
#include "common/logger.hpp"
#include "common/shared_state.hpp"
#include <nnos/profile_matcher.hpp>
#include <nnos/neuro_profile.hpp>
#include <thread>
#include <chrono>
#include <random>

using namespace nnos;

int main() {
    SignalHandler::install([]{});
    Logger logger("lsa_profile_refiner");
    logger.info("BOOT", "Profile refiner starting");

    ProfileMatcher matcher;

    // Simulate daily 03:00 batch run
    auto last_run = std::chrono::steady_clock::now();

    while (!SignalHandler::should_shutdown()) {
        auto now = std::chrono::steady_clock::now();
        auto hours_since = std::chrono::duration_cast<std::chrono::hours>(now - last_run).count();

        if (hours_since >= 24) {
            logger.info("BATCH", "Starting daily profile refinement");

            // Simulate observed profile from 30-day logs
            NeuroProfile observed = profiles::SYSTEMS_HYPERFOCUS;
            observed.hyperfocus_inclination = TraitLevel::VERY_HIGH;
            observed.sensory_sensitivity_level = TraitLevel::HIGH;
            observed.social_energy_capacity = TraitLevel::LOW;
            observed.confidence = 0;

            NeuroProfile matched = matcher.match_profile(observed);

            logger.info("BATCH", "Matched profile_id=" + std::to_string(matched.profile_id) +
                        " confidence=" + std::to_string(matched.confidence) + "/255");

            // Publish refined profile to shared state
            std::vector<uint8_t> payload{
                matched.profile_id,
                matched.confidence
            };
            SharedState::instance().write_field(StateFieldType::PROFILE_STATE, payload);

            last_run = now;
        }

        std::this_thread::sleep_for(std::chrono::seconds(10));
    }

    logger.info("SHUTDOWN", "Profile refiner stopping");
    return 0;
}
