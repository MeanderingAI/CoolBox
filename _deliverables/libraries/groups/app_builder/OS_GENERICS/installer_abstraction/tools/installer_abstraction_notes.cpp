#include "installer_abstraction.hpp"

#include <iostream>
#include <string>

namespace {

os_generics::installer::ReleaseChannel parse_channel(const std::string& value) {
    if (value == "stable") {
        return os_generics::installer::ReleaseChannel::Stable;
    }
    return os_generics::installer::ReleaseChannel::PreRelease;
}

int print_usage() {
    std::cerr << "Usage: installer_abstraction_notes <pre-release|install> <product-name> <executable-name> [pre-release|stable]\n";
    return 1;
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 4 || argc > 5) {
        return print_usage();
    }

    const std::string mode = argv[1];
    const std::string product_name = argv[2];
    const std::string executable_name = argv[3];
    const auto channel = parse_channel(argc == 5 ? argv[4] : "pre-release");

    if (mode == "pre-release") {
        const auto screen = os_generics::installer::create_pre_release_screen(product_name, executable_name, channel);
        std::cout << os_generics::installer::render_pre_release_text(screen);
        return 0;
    }

    if (mode == "install") {
        const auto plan = os_generics::installer::create_packaged_plan(product_name, executable_name, channel);
        std::cout << os_generics::installer::render_installation_notes(plan);
        return 0;
    }

    return print_usage();
}