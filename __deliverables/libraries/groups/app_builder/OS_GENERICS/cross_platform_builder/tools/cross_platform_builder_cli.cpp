#include "cli_tools.hpp"
#include "cross_platform_builder.hpp"

#include <iostream>

int main(int argc, const char* const argv[]) {
    using namespace os_generics::cli;
    using namespace os_generics::cross_platform_builder;

    CommandLineParser parser;
    parser.set_program_name("cross_platform_builder_cli");
    parser.set_description("Generate simulator build plans for Android and iOS targets.");

    parser.add_option({"help", 'h', false, false, "", "Show help and exit."});
    parser.add_option({"project-root", 'p', true, true, "PATH", "CMake project root."});
    parser.add_option({"build-root", 'b', true, true, "PATH", "Build output root folder."});
    parser.add_option({"target", 't', true, true, "NAME", "App target to build."});
    parser.add_option({"platform", '\0', true, true, "android|ios", "Simulator platform."});
    parser.add_option({"config", 'c', true, false, "Debug|Release", "Build configuration."});
    parser.add_option({"android-abi", '\0', true, false, "ABI", "Android ABI (default x86_64)."});
    parser.add_option({"android-api", '\0', true, false, "API", "Android API level (default 24)."});
    parser.add_option({"android-ndk-env", '\0', true, false, "ENV", "NDK environment variable (default ANDROID_NDK_HOME)."});
    parser.add_option({"ios-arch", '\0', true, false, "ARCH", "iOS simulator architecture (default arm64)."});
    parser.add_option({"ios-generator", '\0', true, false, "GEN", "CMake generator for iOS (default Xcode)."});
    parser.add_option({"ios-deployment-target", '\0', true, false, "VER", "iOS deployment target (default 14.0)."});

    const auto parsed = parser.parse_argv(argc, argv);
    if (!parsed.ok()) {
        for (const auto& err : parsed.errors) {
            std::cerr << "Error: " << err << "\n";
        }
        std::cerr << "\n" << parser.render_help() << "\n";
        return 1;
    }

    if (parsed.has_option("help")) {
        std::cout << parser.render_help() << "\n";
        return 0;
    }

    BuildRequest request;
    request.project_root = parsed.option_value("project-root", "");
    request.build_root = parsed.option_value("build-root", "");
    request.app_target = parsed.option_value("target", "");
    request.configuration = parsed.option_value("config", "Debug");

    const std::string platform = parsed.option_value("platform", "");
    if (platform == "android") {
        request.platform = TargetPlatform::AndroidSimulator;
    } else if (platform == "ios") {
        request.platform = TargetPlatform::IosSimulator;
    } else {
        std::cerr << "Error: --platform must be one of: android, ios\n";
        return 1;
    }

    request.android_abi = parsed.option_value("android-abi", request.android_abi);
    request.android_api = parsed.option_value("android-api", request.android_api);
    request.android_ndk_env = parsed.option_value("android-ndk-env", request.android_ndk_env);
    request.ios_arch = parsed.option_value("ios-arch", request.ios_arch);
    request.ios_generator = parsed.option_value("ios-generator", request.ios_generator);
    request.ios_deployment_target = parsed.option_value("ios-deployment-target", request.ios_deployment_target);

    const auto validation = validate_request(request);
    if (!validation.empty()) {
        for (const auto& err : validation) {
            std::cerr << "Error: " << err << "\n";
        }
        return 1;
    }

    const BuildPlan plan = make_build_plan(request);

    std::cout << "Platform: " << platform_name(request.platform) << "\n";
    std::cout << "Configure: " << plan.configure_command << "\n";
    std::cout << "Build: " << plan.build_command << "\n";

    if (!plan.notes.empty()) {
        std::cout << "Notes:\n";
        for (const auto& note : plan.notes) {
            std::cout << "  - " << note << "\n";
        }
    }

    return 0;
}
