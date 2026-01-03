
#include <sstream>
#include <iostream>
#include "advanced_logging/advanced_logging.h"

namespace advanced_logging {

Logger::Logger(const std::string& filename) {
#ifdef __EMSCRIPTEN__
    log_to_console_ = true;
#else
    if (filename.empty()) {
        log_to_console_ = true;
    } else {
        file_.open(filename, std::ios::app);
        log_to_console_ = !file_.is_open();
    }
#endif
}

Logger::~Logger() {
    if (file_.is_open()) file_.close();
}

void Logger::log(Level level, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string log_entry = timestamp() + " [" + level_to_string(level) + "] " + message;
#ifdef __EMSCRIPTEN__
    // Always log to JS console in Emscripten
    EM_ASM_({
        if ($0 === 0) console.debug(UTF8ToString($1));
        else if ($0 === 1) console.info(UTF8ToString($1));
        else if ($0 === 2) console.warn(UTF8ToString($1));
        else if ($0 === 3) console.error(UTF8ToString($1));
        else console.log(UTF8ToString($1));
    }, static_cast<int>(level), log_entry.c_str());
#else
    if (file_.is_open()) {
        file_ << log_entry << std::endl;
    }
    if (log_to_console_ || !file_.is_open()) {
        if (level == Level::ERROR || level == Level::WARN)
            std::cerr << log_entry << std::endl;
        else
            std::cout << log_entry << std::endl;
    }
#endif
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
