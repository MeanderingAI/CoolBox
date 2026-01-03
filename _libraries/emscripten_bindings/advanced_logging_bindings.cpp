#include <emscripten/bind.h>
#include "advanced_logging/advanced_logging.h"

using namespace emscripten;
using namespace advanced_logging;

EMSCRIPTEN_BINDINGS(advanced_logging_module) {
    class_<Logger>("Logger")
        .constructor<std::string>()
        .function("log", &Logger::log)
        .function("debug", &Logger::debug)
        .function("info", &Logger::info)
        .function("warn", &Logger::warn)
        .function("error", &Logger::error)
    ;
    enum_<Logger::Level>("Level")
        .value("DEBUG", Logger::Level::DEBUG)
        .value("INFO", Logger::Level::INFO)
        .value("WARN", Logger::Level::WARN)
        .value("ERROR", Logger::Level::ERROR)
    ;
}
