#include "cli_tools.hpp"

#include <sstream>

namespace os_generics {
namespace cli {
namespace {

const OptionSpec* find_by_long_name(const std::vector<OptionSpec>& options, const std::string& long_name) {
    for (const auto& option : options) {
        if (option.long_name == long_name) {
            return &option;
        }
    }
    return nullptr;
}

const OptionSpec* find_by_short_name(const std::vector<OptionSpec>& options, char short_name) {
    for (const auto& option : options) {
        if (option.short_name == short_name) {
            return &option;
        }
    }
    return nullptr;
}

void append_option(ParseResult& result, const std::string& key, const std::string& value) {
    result.options[key].push_back(value);
}

bool is_option_token(const std::string& token) {
    return token.size() >= 2 && token[0] == '-';
}

} // namespace

bool ParseResult::ok() const {
    return errors.empty();
}

bool ParseResult::has_option(const std::string& long_name) const {
    return options.find(long_name) != options.end();
}

std::string ParseResult::option_value(const std::string& long_name, const std::string& fallback) const {
    const auto it = options.find(long_name);
    if (it == options.end() || it->second.empty()) {
        return fallback;
    }
    return it->second.back();
}

std::vector<std::string> ParseResult::option_values(const std::string& long_name) const {
    const auto it = options.find(long_name);
    if (it == options.end()) {
        return {};
    }
    return it->second;
}

void CommandLineParser::set_program_name(const std::string& name) {
    if (!name.empty()) {
        program_name_ = name;
    }
}

void CommandLineParser::set_description(const std::string& description) {
    description_ = description;
}

void CommandLineParser::add_option(const OptionSpec& option) {
    options_.push_back(option);
}

ParseResult CommandLineParser::parse_argv(int argc, const char* const argv[]) const {
    return parse(argv_to_vector(argc, argv));
}

ParseResult CommandLineParser::parse(const std::vector<std::string>& args) const {
    ParseResult result;

    for (std::size_t i = 1; i < args.size(); ++i) {
        const std::string& token = args[i];

        if (token == "--") {
            for (std::size_t j = i + 1; j < args.size(); ++j) {
                result.positionals.push_back(args[j]);
            }
            break;
        }

        if (token.rfind("--", 0) == 0) {
            const std::size_t eq_pos = token.find('=');
            const std::string long_name = token.substr(2, eq_pos == std::string::npos ? std::string::npos : eq_pos - 2);
            const OptionSpec* option = find_by_long_name(options_, long_name);
            if (option == nullptr) {
                result.errors.push_back("Unknown option: --" + long_name);
                continue;
            }

            if (!option->expects_value) {
                if (eq_pos != std::string::npos) {
                    result.errors.push_back("Option does not accept a value: --" + long_name);
                    continue;
                }
                append_option(result, option->long_name, "true");
                continue;
            }

            if (eq_pos != std::string::npos) {
                append_option(result, option->long_name, token.substr(eq_pos + 1));
                continue;
            }

            if (i + 1 >= args.size() || is_option_token(args[i + 1])) {
                result.errors.push_back("Missing value for option: --" + long_name);
                continue;
            }

            append_option(result, option->long_name, args[++i]);
            continue;
        }

        if (token.size() >= 2 && token[0] == '-') {
            // Supports compact short flags like -abc and value short option like -o out.txt.
            for (std::size_t pos = 1; pos < token.size(); ++pos) {
                const char short_name = token[pos];
                const OptionSpec* option = find_by_short_name(options_, short_name);
                if (option == nullptr) {
                    result.errors.push_back(std::string("Unknown option: -") + short_name);
                    continue;
                }

                if (!option->expects_value) {
                    append_option(result, option->long_name, "true");
                    continue;
                }

                if (pos + 1 < token.size()) {
                    append_option(result, option->long_name, token.substr(pos + 1));
                    break;
                }

                if (i + 1 >= args.size() || is_option_token(args[i + 1])) {
                    result.errors.push_back(std::string("Missing value for option: -") + short_name);
                    break;
                }

                append_option(result, option->long_name, args[++i]);
                break;
            }
            continue;
        }

        result.positionals.push_back(token);
    }

    for (const auto& option : options_) {
        if (option.required && !result.has_option(option.long_name)) {
            result.errors.push_back("Missing required option: --" + option.long_name);
        }
    }

    return result;
}

std::string CommandLineParser::render_help() const {
    std::ostringstream out;
    out << "Usage: " << program_name_ << " [options] [args]";

    if (!description_.empty()) {
        out << "\n\n" << description_;
    }

    if (!options_.empty()) {
        out << "\n\nOptions:";
    }

    for (const auto& option : options_) {
        out << "\n  ";
        if (option.short_name != '\0') {
            out << '-' << option.short_name << ", ";
        }
        out << "--" << option.long_name;
        if (option.expects_value) {
            out << ' ' << (option.value_name.empty() ? "VALUE" : option.value_name);
        }
        if (!option.description.empty()) {
            out << "\n      " << option.description;
        }
        if (option.required) {
            out << " (required)";
        }
    }

    return out.str();
}

std::vector<std::string> argv_to_vector(int argc, const char* const argv[]) {
    std::vector<std::string> args;
    args.reserve(static_cast<std::size_t>(argc));
    for (int i = 0; i < argc; ++i) {
        args.emplace_back(argv[i] != nullptr ? argv[i] : "");
    }
    return args;
}

} // namespace cli
} // namespace os_generics
