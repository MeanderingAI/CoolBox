#ifndef ADVANCED_LOGGING_H
#define ADVANCED_LOGGING_H

#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iomanip>

namespace advanced_logging {

class Logger {
public:
    enum class Level { DEBUG, INFO, WARN, ERROR };

    Logger(const std::string& filename);
    ~Logger();

    void log(Level level, const std::string& message);
    void debug(const std::string& message);
    void info(const std::string& message);
    void warn(const std::string& message);
    void error(const std::string& message);

private:
    std::ofstream file_;
    std::mutex mutex_;
    std::string level_to_string(Level level);
    std::string timestamp();
};

} // namespace advanced_logging

#endif // ADVANCED_LOGGING_H
