#include "logger.hpp"
#include <iostream>
#include <cstdlib>
#include <cctype>
#include <climits>
#include <cstdio>
#include <stdlib.h>  // realpath
#include <time.h>    // localtime_r

namespace nnos {

namespace {

// Canonicalize the log directory: resolve NNOS_LOG_DIR (or the default
// root) with realpath so '..' and symlinked components are eliminated.
// Fail closed to the default root if the env-supplied directory cannot
// be canonicalized.
std::string canonical_log_dir() {
    const std::string fallback = "/var/log/lsa";
    const char* env = std::getenv("NNOS_LOG_DIR");
    const std::string dir = (env && env[0]) ? env : fallback;
    char resolved[PATH_MAX];
    if (realpath(dir.c_str(), resolved) != nullptr)
        return std::string(resolved);
    if (dir != fallback && realpath(fallback.c_str(), resolved) != nullptr)
        return std::string(resolved);
    return fallback;
}

// The daemon name becomes the single filename component of the log path:
// reject path separators, '..' sequences, and control characters so it
// cannot escape the canonicalized log directory.
bool safe_daemon_name(const std::string& name) {
    if (name.empty()) return false;
    if (name.find('/') != std::string::npos) return false;
    if (name.find("..") != std::string::npos) return false;
    for (unsigned char c : name) {
        if (std::iscntrl(c)) return false;
    }
    return true;
}

} // namespace

Logger::Logger(const std::string& daemon_name)
    : daemon_name_(daemon_name)
{
    if (!safe_daemon_name(daemon_name)) {
        std::cerr << "[NNOS] Refusing unsafe log name: " << daemon_name << std::endl;
        return;
    }
    std::string dir = canonical_log_dir();
    std::string path = dir + "/" + daemon_name + ".jsonl";
    // Confinement: the final path must live directly inside the
    // canonicalized log directory.
    if (path.compare(0, dir.size() + 1, dir + "/") != 0) {
        std::cerr << "[NNOS] Refusing log path outside log dir: " << path << std::endl;
        return;
    }
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
