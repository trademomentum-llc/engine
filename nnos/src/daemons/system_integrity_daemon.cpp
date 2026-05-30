// system_integrity_daemon — Global System Health & Integrity Monitor
// Responsibility: Monitor all node health, raise alerts on corruption or failure.
#include "common/signal_handler.hpp"
#include "common/logger.hpp"
#include "common/shared_state.hpp"
#include <thread>
#include <chrono>
#include <cstdint>
#include <vector>

using namespace nnos;

struct NodeHealth {
    uint32_t node_id;
    bool online;
    uint32_t last_seen_sec;
    float cpu_temp_c;
    float memory_pressure;
    uint32_t daemon_count;
};

int main() {
    SignalHandler::install([]{});
    Logger logger("system_integrity_daemon");
    logger.info("BOOT", "System integrity monitor starting");

    while (!SignalHandler::should_shutdown()) {
        auto fields = SharedState::instance().read_all();

        uint32_t active_nodes = 0;
        uint32_t error_count = 0;

        for (const auto& f : fields) {
            if (f.type == StateFieldType::DAEMON_HEARTBEAT) {
                active_nodes++;
            } else if (f.type == StateFieldType::THREAT_SCORE) {
                error_count++;
            }
        }

        logger.info("HEALTH", "active_nodes=" + std::to_string(active_nodes) +
                    " errors=" + std::to_string(error_count));

        if (error_count > 0) {
            logger.critical("INTEGRITY", std::to_string(error_count) + " errors in last cycle");
        }

        if (active_nodes < 3) {
            logger.critical("INTEGRITY", "Only " + std::to_string(active_nodes) +
                        " nodes active (expected >= 3)");
        }

        std::this_thread::sleep_for(std::chrono::seconds(10));
    }

    logger.info("SHUTDOWN", "System integrity monitor stopping");
    return 0;
}
