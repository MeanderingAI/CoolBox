#include "sun_command_parser.hpp"

#include "sun_text.hpp"

namespace sun::comms::parser {

std::optional<Command> parse_command_line(const std::string& line) {
    const auto parts = sun::misc::split_ws(line);
    if (parts.empty()) {
        return std::nullopt;
    }

    Command cmd;
    const std::string op = sun::misc::to_upper(parts[0]);

    if (op == "GET" && parts.size() == 2) {
        cmd.type = CommandType::kGet;
        cmd.key = parts[1];
        return cmd;
    }

    if (op == "SET" && parts.size() >= 3) {
        cmd.type = CommandType::kSet;
        cmd.key = parts[1];
        cmd.value = parts[2];
        return cmd;
    }

    if (op == "DELETE" && parts.size() == 2) {
        cmd.type = CommandType::kDelete;
        cmd.key = parts[1];
        return cmd;
    }

    return std::nullopt;
}

}  // namespace sun::comms::parser
