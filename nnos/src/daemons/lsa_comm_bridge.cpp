// lsa_comm_bridge — M1 Communication & Anti-Masking Daemon
// Responsibility: Evaluate incoming requests, detect masking, recommend negotiate/decline.
#include "common/signal_handler.hpp"
#include "common/logger.hpp"
#include "common/shared_state.hpp"
#include <nnos/communication_bridge.hpp>
#include <nnos/neuro_profile.hpp>
#include <thread>
#include <chrono>
#include <random>

using namespace nnos;

int main() {
    SignalHandler::install([]{});
    Logger logger("lsa_comm_bridge");
    logger.info("BOOT", "Communication bridge starting");

    NeuroProfile profile = profiles::SYSTEMS_HYPERFOCUS;
    CommunicationBridge bridge(profile);

    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> energy_dist(2, 9);
    std::uniform_int_distribution<int> social_dist(1, 10);

    while (!SignalHandler::should_shutdown()) {
        IncomingRequest req{};
        req.id = 1;
        req.importance = 7;
        req.est_effort_min = 90;
        req.deadline_hours = 8;
        req.social_intensity = social_dist(rng);

        CapacitySnapshot cap{};
        cap.energy = energy_dist(rng);
        cap.sensory_load = 4;
        cap.social_battery = social_dist(rng);
        cap.existing_commitment_load = 5;

        CommDecision decision = bridge.evaluate(req, cap);
        bool masking = bridge.is_likely_masking(req, cap);

        std::string type_str;
        switch (decision.type) {
            case CommDecisionType::ACCEPT: type_str = "ACCEPT"; break;
            case CommDecisionType::NEGOTIATE_SCOPE: type_str = "NEGOTIATE_SCOPE"; break;
            case CommDecisionType::NEGOTIATE_DEADLINE: type_str = "NEGOTIATE_DEADLINE"; break;
            case CommDecisionType::DECLINE: type_str = "DECLINE"; break;
        }

        logger.info("EVAL", "decision=" + type_str + " masking=" + (masking ? "YES" : "NO") +
                    " confidence=" + std::to_string(decision.confidence));

        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    logger.info("SHUTDOWN", "Communication bridge stopping");
    return 0;
}
