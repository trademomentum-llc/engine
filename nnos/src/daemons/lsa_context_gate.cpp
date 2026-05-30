// lsa_context_gate — NUC Context Switch & Hyperfocus Protection Daemon
// Responsibility: Enforce context-switch budgets, protect hyperfocus, generate handoffs.
#include "common/signal_handler.hpp"
#include "common/logger.hpp"
#include "common/shared_state.hpp"
#include <nnos/context_gating.hpp>
#include <nnos/neuro_profile.hpp>
#include <thread>
#include <chrono>

using namespace nnos;

int main() {
    SignalHandler::install([]{});
    Logger logger("lsa_context_gate");
    logger.info("BOOT", "Context gate starting");

    NeuroProfile profile = profiles::SYSTEMS_HYPERFOCUS;
    ContextGating gate(profile);

    uint32_t switch_count = 0;
    auto last_reset = std::chrono::steady_clock::now();

    while (!SignalHandler::should_shutdown()) {
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::hours>(now - last_reset).count() >= 1) {
            gate.reset_switch_counter();
            switch_count = 0;
            last_reset = now;
            logger.info("BUDGET", "Hourly context-switch counter reset");
        }

        // Stub: evaluate a synthetic context state
        ContextState state{};
        state.active_task_id = 1;
        state.active_task_importance = 7;
        state.incoming_task_importance = 5;
        state.incoming_is_interrupt = false;
        state.hyperfocus_active = true;
        state.hyperfocus_minutes = 45;
        state.physiological_risk_high = false;
        state.self_report_dissociated = false;

        HandoffNote handoff{};
        ContextDecision decision = gate.evaluate(state, handoff);

        std::string decision_str;
        switch (decision) {
            case ContextDecision::KEEP_FOCUS: decision_str = "KEEP_FOCUS"; break;
            case ContextDecision::ALLOW_SWITCH_WITH_HANDOFF: decision_str = "ALLOW_SWITCH"; ++switch_count; break;
            case ContextDecision::DEFER_INCOMING: decision_str = "DEFER"; break;
            case ContextDecision::BREAK_HYPERFOCUS: decision_str = "BREAK_HYPERFOCUS"; break;
        }

        logger.info("GATE", "decision=" + decision_str + " switches_this_hour=" + std::to_string(switch_count));

        // Publish decision to shared state
        std::vector<uint8_t> payload{static_cast<uint8_t>(decision), static_cast<uint8_t>(switch_count)};
        SharedState::instance().write_field(StateFieldType::CONTEXT_SWITCH_COUNT, payload);

        std::this_thread::sleep_for(std::chrono::seconds(3));
    }

    logger.info("SHUTDOWN", "Context gate stopping");
    return 0;
}
