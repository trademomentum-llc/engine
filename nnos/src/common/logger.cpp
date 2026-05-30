#include "logger.hpp"
#include <iostream>
#include <cstdlib>

namespace nnos {

Logger::Logger(const std::string& daemon_name)
    : daemon_name_(daemon_name)
{
    const char* log_dir = std::getenv("NNOS_LOG_DIR");
    std::string dir = log_dir ? log_dir : "/var/log/lsa";
    std::string path = dir + "/" + daemon_name + ".jsonl";
    file_.open(path, std::ios::app);
    if (!file_) {
        std::cerr << "[NNOS] Failed to open log: " << path << std::endl;
    }
}

Logger::~Logger() {
    if (file_.is_open()) file_.close();
}

void Logger::log(LogLevel level, const std::string& event_type, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string line = "{"
        "\"ts\":\"" + timestamp() + "\","
        "\"daemon\":\"" + daemon_name_ + "\","
        "\"level\":\"" + level_str(level) + "\","
        "\"event\":\"" + event_type + "\","
        "\"msg\":\"" + message + "\""
        "}\n";

    if (file_.is_open()) {
        file_ << line;
        file_.flush();
    }

    // Also echo CRITICAL to stderr
    if (level >= LogLevel::ERROR) {
        std::cerr << "[" << daemon_name_ << "] " << level_str(level) << ": " << message << std::endl;
    }
}

std::string Logger::level_str(LogLevel l) const {
    switch (l) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARN: return "WARN";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::CRITICAL: return "CRITICAL";
    }
    return "UNKNOWN";
}

std::string Logger::timestamp() const {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    localtime_r(&time, &tm);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S") << "." << std::setfill('0') << std::setw(3) << ms.count() << "Z";
    return oss.str();
}

} // namespace nnos
