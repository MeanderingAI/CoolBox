
#include <sstream>
#include "advanced_logging/advanced_logging.h"

namespace advanced_logging {

Logger::Logger(const std::string& filename) {
    file_.open(filename, std::ios::app);
}

Logger::~Logger() {
    if (file_.is_open()) file_.close();
}

void Logger::log(Level level, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_.is_open()) {
        file_ << timestamp() << " [" << level_to_string(level) << "] " << message << std::endl;
    }
}

void Logger::debug(const std::string& message) { log(Level::DEBUG, message); }
void Logger::info(const std::string& message) { log(Level::INFO, message); }
void Logger::warn(const std::string& message) { log(Level::WARN, message); }
void Logger::error(const std::string& message) { log(Level::ERROR, message); }

std::string Logger::level_to_string(Level level) {
    switch (level) {
        case Level::DEBUG: return "DEBUG";
        case Level::INFO:  return "INFO";
        case Level::WARN:  return "WARN";
        case Level::ERROR: return "ERROR";
    }
    return "UNKNOWN";
}

std::string Logger::timestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

} // namespace advanced_logging
