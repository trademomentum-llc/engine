#include "signal_handler.hpp"
#include <csignal>
#include <cstdlib>

namespace nnos {

std::atomic<bool> SignalHandler::shutdown_requested_{false};
std::function<void()> SignalHandler::shutdown_callback_;

void SignalHandler::handle_signal(int sig) {
    (void)sig;
    shutdown_requested_.store(true);
    if (shutdown_callback_) {
        shutdown_callback_();
    }
}

void SignalHandler::install(std::function<void()> on_shutdown) {
    shutdown_callback_ = on_shutdown;
    std::signal(SIGINT, SignalHandler::handle_signal);
    std::signal(SIGTERM, SignalHandler::handle_signal);
}

bool SignalHandler::should_shutdown() {
    return shutdown_requested_.load();
}

} // namespace nnos
