// lsa_boot_epn — External Perception Node Bootstrap
// Responsibility: Fork Orin-local children, restart crashes, cascading shutdown.
#include "common/signal_handler.hpp"
#include "common/logger.hpp"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <csignal>
#include <chrono>
#include <thread>

using namespace nnos;

struct ChildSpec {
    const char* name;
    const char* binary;
};

static const ChildSpec EPN_CHILDREN[] = {
    {"lsa_state_monitor", "./lsa_state_monitor"},
    {"lsa_ethernet_sync", "./lsa_ethernet_sync"},
};

static constexpr size_t CHILD_COUNT = sizeof(EPN_CHILDREN) / sizeof(EPN_CHILDREN[0]);
static pid_t child_pids[CHILD_COUNT] = {};
static Logger logger("lsa_boot_epn");

static void shutdown_children() {
    logger.info("SHUTDOWN", "Sending SIGTERM to all EPN children");
    for (size_t i = 0; i < CHILD_COUNT; ++i) {
        if (child_pids[i] > 0) kill(child_pids[i], SIGTERM);
    }
    for (int s = 0; s < 10; ++s) {
        bool all_done = true;
        for (size_t i = 0; i < CHILD_COUNT; ++i) {
            if (child_pids[i] > 0) {
                int status = 0;
                pid_t r = waitpid(child_pids[i], &status, WNOHANG);
                if (r == 0) all_done = false;
                else child_pids[i] = 0;
            }
        }
        if (all_done) break;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    for (size_t i = 0; i < CHILD_COUNT; ++i) {
        if (child_pids[i] > 0) kill(child_pids[i], SIGKILL);
    }
}

static pid_t spawn_child(const ChildSpec& spec) {
    pid_t pid = fork();
    if (pid == 0) {
        execl(spec.binary, spec.name, nullptr);
        _exit(127);
    } else if (pid > 0) {
        logger.info("SPAWN", std::string("Started ") + spec.name + " pid=" + std::to_string(pid));
    }
    return pid;
}

int main() {
    SignalHandler::install(shutdown_children);
    logger.info("BOOT", "lsa_boot_epn starting — EPN supervision");

    for (size_t i = 0; i < CHILD_COUNT; ++i) {
        child_pids[i] = spawn_child(EPN_CHILDREN[i]);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    while (!SignalHandler::should_shutdown()) {
        for (size_t i = 0; i < CHILD_COUNT; ++i) {
            if (child_pids[i] <= 0) continue;
            int status = 0;
            pid_t r = waitpid(child_pids[i], &status, WNOHANG);
            if (r > 0) {
                int exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
                logger.warn("SUPERVISION",
                    std::string(EPN_CHILDREN[i].name) + " exited code=" + std::to_string(exit_code) +
                    ". Restarting in 2s...");
                std::this_thread::sleep_for(std::chrono::seconds(2));
                child_pids[i] = spawn_child(EPN_CHILDREN[i]);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    shutdown_children();
    logger.info("BOOT", "lsa_boot_epn shutdown complete");
    return 0;
}
