// lsa_task_manager — NUC Task Intake & Scheduling Daemon
// Responsibility: Accept/defer tasks based on profile capacity, breathing room, masking risk.
#include "common/signal_handler.hpp"
#include "common/logger.hpp"
#include "common/shared_state.hpp"
#include <nnos/task_manager.hpp>
#include <nnos/neuro_profile.hpp>
#include <nnos/profile_matcher.hpp>
#include <thread>
#include <chrono>
#include <vector>

using namespace nnos;

int main() {
    SignalHandler::install([]{});
    Logger logger("lsa_task_manager");
    logger.info("BOOT", "Task manager starting");

    // Load active profile from shared state (or use default)
    NeuroProfile profile = profiles::SYSTEMS_HYPERFOCUS;
    TaskManager task_mgr(profile);

    uint8_t active_count = 0;

    while (!SignalHandler::should_shutdown()) {
        // Check for incoming task requests in shared state
        StateField field;
        if (SharedState::instance().read_latest(StateFieldType::TASK_BUDGET_REMAINING, field)) {
            if (!field.payload.empty()) {
                uint8_t requested = field.payload[0];
                logger.info("INTAKE", "Task request received, demand_flags=" + std::to_string(requested));

                // Stub: create a synthetic task for evaluation
                Task task{};
                task.id = 1;
                task.demand_flags = requested;
                task.est_effort_minutes = 60;
                task.importance = 5;
                task.social_cost = 3;
                task.sensory_cost = 3;
                task.deadline_timestamp = 0;

                bool accepted = task_mgr.intake_task(task);
                if (accepted) {
                    ++active_count;
                    logger.info("INTAKE", "Task accepted, scheduled_minute=" + std::to_string(task.scheduled_minute));
                } else {
                    logger.warn("INTAKE", "Task deferred — capacity or slot unavailable");
                }

                // Publish result back to shared state
                std::vector<uint8_t> ack{static_cast<uint8_t>(accepted ? 1 : 0), active_count};
                SharedState::instance().write_field(StateFieldType::DAEMON_HEARTBEAT, ack);
            }
        }

        logger.info("HEARTBEAT", "active_tasks=" + std::to_string(active_count));
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    logger.info("SHUTDOWN", "Task manager stopping");
    return 0;
}
