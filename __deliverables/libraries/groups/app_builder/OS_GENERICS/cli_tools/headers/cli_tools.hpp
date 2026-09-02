#ifndef COOLBOX__LIBRARIES_GROUPS_APP_BUILDER_OS_GENERICS_CLI_TOOLS_HEADERS_CLI_TOOLS_HPP
#define COOLBOX__LIBRARIES_GROUPS_APP_BUILDER_OS_GENERICS_CLI_TOOLS_HEADERS_CLI_TOOLS_HPP

#include <string>
#include <unordered_map>
#include <vector>

namespace os_generics {
namespace cli {

struct OptionSpec {
    std::string long_name;
    char short_name = '\0';
    bool expects_value = false;
    bool required = false;
    std::string value_name;
    std::string description;
};

struct ParseResult {
    std::unordered_map<std::string, std::vector<std::string>> options;
    std::vector<std::string> positionals;
    std::vector<std::string> errors;

    bool ok() const;
    bool has_option(const std::string& long_name) const;
    std::string option_value(const std::string& long_name, const std::string& fallback = "") const;
    std::vector<std::string> option_values(const std::string& long_name) const;
};

class CommandLineParser {
public:
    CommandLineParser() = default;

    void set_program_name(const std::string& name);
    void set_description(const std::string& description);
    void add_option(const OptionSpec& option);

    ParseResult parse(const std::vector<std::string>& args) const;
    ParseResult parse_argv(int argc, const char* const argv[]) const;
    std::string render_help() const;

private:
    std::string program_name_ = "app";
    std::string description_;
    std::vector<OptionSpec> options_;
};

std::vector<std::string> argv_to_vector(int argc, const char* const argv[]);

} // namespace cli
} // namespace os_generics

#endif
