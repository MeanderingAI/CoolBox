#ifndef SERVICE_MANAGER_LOGGER_HPP
#define SERVICE_MANAGER_LOGGER_HPP
#include <iostream>
#include <fstream>
#include <string>
#include <mutex>
#include <chrono>
#include <ctime>

class Logger {
public:
    enum Level { INFO, WARN, ERROR };
    Logger(const std::string& filename = "service_manager.log") : log_file_(filename, std::ios::app) {}
    void log(Level level, const std::string& msg) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        log_file_ << "[" << std::ctime(&now) << "] [" << level_str(level) << "] " << msg << std::endl;
        log_file_.flush();
        std::cout << "[" << level_str(level) << "] " << msg << std::endl;
    }
    void info(const std::string& msg) { log(INFO, msg); }
    void warn(const std::string& msg) { log(WARN, msg); }
    void error(const std::string& msg) { log(ERROR, msg); }
private:
    std::ofstream log_file_;
    std::mutex mutex_;
    std::string level_str(Level level) {
        switch (level) {
            case INFO: return "INFO";
            case WARN: return "WARN";
            case ERROR: return "ERROR";
        }
        return "UNKNOWN";
    }
};
#endif // SERVICE_MANAGER_LOGGER_HPP
