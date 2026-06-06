#pragma once

#include <optional>
#include <string>

namespace sun::comms::parser {

enum class CommandType {
    kGet,
    kSet,
    kDelete,
    kUnknown,
};

struct Command {
    CommandType type = CommandType::kUnknown;
    std::string key;
    std::string value;
};

std::optional<Command> parse_command_line(const std::string& line);

}  // namespace sun::comms::parser
