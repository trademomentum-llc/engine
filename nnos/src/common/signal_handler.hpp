#pragma once
#include <atomic>
#include <functional>

namespace nnos {

class SignalHandler {
public:
    static void install(std::function<void()> on_shutdown);
    static bool should_shutdown();

private:
    static void handle_signal(int sig);
    static std::atomic<bool> shutdown_requested_;
    static std::function<void()> shutdown_callback_;
};

} // namespace nnos
