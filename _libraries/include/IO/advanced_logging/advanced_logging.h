
/**
 * @mainpage advanced_logging Library
 *
 * @section usage_examples Usage Examples
 *
 * @subsection cpp_example C++ Example
 * @code{.cpp}
 * #include "advanced_logging/advanced_logging.h"
 * int main() {
 *     advanced_logging::Logger logger("mylog.log");
 *     logger.info("Hello from C++!");
 *     logger.warn("This is a warning");
 *     logger.error("This is an error");
 *     return 0;
 * }
 * @endcode
 *
 * @subsection python_example Python Example
 * @code{.python}
 * from ml_toolbox import advanced_logging
 * logger = advanced_logging.Logger("mylog.log")
 * logger.info("Hello from Python!")
 * logger.warn("This is a warning")
 * logger.error("This is an error")
 * @endcode
 *
 * @subsection js_example JavaScript Example (WASM/Emscripten)
 * @code{.js}
 * // Assuming advanced_logging.js and .wasm are loaded
 * createAdvancedLoggingModule().then(Module => {
 *     const Logger = Module.Logger;
 *     const logger = new Logger(); // logs to console by default
 *     logger.info("Hello from JS!");
 *     logger.warn("This is a warning");
 *     logger.error("This is an error");
 * });
 * @endcode
 *
 * @subsection js_example_sync JavaScript Example (Synchronous, MODULARIZE=0)
 * @code{.js}
 * // If advanced_logging.js is loaded and exposes 'Module' globally:
 * const Logger = Module.Logger;
 * const logger = new Logger(); // logs to console by default
 * logger.info("Hello from JS (sync)!");
 * logger.warn("This is a warning");
 * logger.error("This is an error");
 * // Note: If built with MODULARIZE=1 (default), you must use createAdvancedLoggingModule().then(...)
 * // If built with MODULARIZE=0, you can use the Module object directly after script load.
 * @endcode
 */
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

    Logger(const std::string& filename = "");
    ~Logger();

    void log(Level level, const std::string& message);
    void debug(const std::string& message);
    void info(const std::string& message);
    void warn(const std::string& message);
    void error(const std::string& message);

private:
    std::ofstream file_;
    std::mutex mutex_;
    bool log_to_console_ = false;
    std::string level_to_string(Level level);
    std::string timestamp();
};

} // namespace advanced_logging

#endif // ADVANCED_LOGGING_H
