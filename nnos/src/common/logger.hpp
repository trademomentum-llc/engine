#pragma once
#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <sstream>
#include <iomanip>

namespace nnos {

enum class LogLevel : uint8_t {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
    CRITICAL = 4
};

class Logger {
public:
    explicit Logger(const std::string& daemon_name);
    ~Logger();

    void log(LogLevel level, const std::string& event_type, const std::string& message);
    void info(const std::string& event, const std::string& msg) { log(LogLevel::INFO, event, msg); }
    void warn(const std::string& event, const std::string& msg) { log(LogLevel::WARN, event, msg); }
    void error(const std::string& event, const std::string& msg) { log(LogLevel::ERROR, event, msg); }
    void critical(const std::string& event, const std::string& msg) { log(LogLevel::CRITICAL, event, msg); }

private:
    std::string daemon_name_;
    std::ofstream file_;
    mutable std::mutex mutex_;

    std::string level_str(LogLevel l) const;
    std::string timestamp() const;
};

} // namespace nnos
