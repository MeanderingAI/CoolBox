#include "cli_tools.hpp"
#include "uuid_generation.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>

namespace {

std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

bool parse_positive_int(const std::string& value, int& out) {
    if (value.empty()) {
        return false;
    }
    for (char c : value) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            return false;
        }
    }
    try {
        out = std::stoi(value);
    } catch (...) {
        return false;
    }
    return out > 0;
}

std::string generate_uuid_by_version(const std::string& version_value, int index) {
    using namespace trekker::misc::uuid_generation;
    const std::string v = lower(version_value);

    if (v == "1" || v == "v1") return generate_v1().to_string();
    if (v == "2" || v == "v2") return generate_v2(static_cast<std::uint32_t>(index + 1), 0).to_string();
    if (v == "3" || v == "v3") return generate_v3(namespaces::dns(), "secret-gen-" + std::to_string(index)).to_string();
    if (v == "4" || v == "v4") return generate_v4().to_string();
    if (v == "5" || v == "v5") return generate_v5(namespaces::dns(), "secret-gen-" + std::to_string(index)).to_string();
    if (v == "6" || v == "v6") return generate_v6().to_string();
    if (v == "7" || v == "v7") return generate_v7().to_string();
    if (v == "8" || v == "v8") return generate_v8({}).to_string();
    if (v == "guid") return generate_guid();
    if (v == "cuid") return generate_cuid();

    return "";
}

void print_usage(const os_generics::cli::CommandLineParser& parser) {
    std::cerr << parser.render_help() << "\n";
    std::cerr << "\nExamples:\n";
    std::cerr << "  secret_gen_cli --version v7 --batch 5\n";
    std::cerr << "  secret_gen_cli -v 4 -b 10\n";
}

} // namespace

int main(int argc, const char* const argv[]) {
    os_generics::cli::CommandLineParser parser;
    parser.set_program_name("secret_gen_cli");
    parser.set_description("Generate UUIDs in terminal output by version and batch size.");

    parser.add_option({"help", 'h', false, false, "", "Show help and exit."});
    parser.add_option({"version", 'v', true, true, "VERSION", "UUID version: 1,2,3,4,5,6,7,8,v1..v8,guid or cuid"});
    parser.add_option({"batch", 'b', true, true, "COUNT", "How many UUIDs to generate"});

    const auto result = parser.parse_argv(argc, argv);
    if (!result.ok()) {
        for (const auto& error : result.errors) {
            std::cerr << "Error: " << error << "\n";
        }
        print_usage(parser);
        return 1;
    }

    if (result.has_option("help")) {
        std::cout << parser.render_help() << "\n";
        return 0;
    }

    const std::string version = result.option_value("version");
    int batch = 0;
    if (!parse_positive_int(result.option_value("batch"), batch)) {
        std::cerr << "Error: --batch must be a positive integer\n";
        print_usage(parser);
        return 1;
    }

    for (int i = 0; i < batch; ++i) {
        const std::string value = generate_uuid_by_version(version, i);
        if (value.empty()) {
            std::cerr << "Error: unsupported --version value '" << version << "'\n";
            print_usage(parser);
            return 1;
        }
        std::cout << value << "\n";
    }

    return 0;
}
