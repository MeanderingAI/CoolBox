#include "cli_tools.hpp"

#include <iostream>

int main(int argc, const char* const argv[]) {
    os_generics::cli::CommandLineParser parser;
    parser.set_program_name("cli_tools_demo");
    parser.set_description("Example app built with app_builder/OS_GENERICS/cli_tools.");

    parser.add_option({"help", 'h', false, false, "", "Show help and exit."});
    parser.add_option({"name", 'n', true, false, "NAME", "Optional user name."});
    parser.add_option({"verbose", 'v', false, false, "", "Enable verbose mode."});

    const auto result = parser.parse_argv(argc, argv);
    if (!result.ok()) {
        for (const auto& error : result.errors) {
            std::cerr << "Error: " << error << "\n";
        }
        std::cerr << "\n" << parser.render_help() << "\n";
        return 1;
    }

    if (result.has_option("help")) {
        std::cout << parser.render_help() << "\n";
        return 0;
    }

    const std::string name = result.option_value("name", "world");
    const bool verbose = result.has_option("verbose");

    std::cout << "Hello, " << name << "!\n";
    if (verbose) {
        std::cout << "Verbose mode enabled. Positional arg count: " << result.positionals.size() << "\n";
    }

    return 0;
}
