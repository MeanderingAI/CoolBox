#ifndef COOLBOX__LIBRARIES_GROUPS_APP_BUILDER_OS_GENERICS_CROSS_PLATFORM_BUILDER_HEADERS_CROSS_PLATFORM_BUILDER_HPP
#define COOLBOX__LIBRARIES_GROUPS_APP_BUILDER_OS_GENERICS_CROSS_PLATFORM_BUILDER_HEADERS_CROSS_PLATFORM_BUILDER_HPP

#include <string>
#include <vector>

namespace os_generics {
namespace cross_platform_builder {

enum class TargetPlatform {
    AndroidSimulator,
    IosSimulator,
};

struct BuildRequest {
    std::string project_root;
    std::string build_root;
    std::string app_target;
    TargetPlatform platform = TargetPlatform::AndroidSimulator;
    std::string configuration = "Debug";

    // Android simulator defaults (x86_64 for emulator).
    std::string android_ndk_env = "ANDROID_NDK_HOME";
    std::string android_abi = "x86_64";
    std::string android_api = "24";

    // iOS simulator defaults.
    std::string ios_generator = "Xcode";
    std::string ios_arch = "arm64";
    std::string ios_deployment_target = "14.0";
};

struct BuildPlan {
    std::string configure_command;
    std::string build_command;
    std::vector<std::string> notes;
};

BuildPlan make_build_plan(const BuildRequest& request);

std::vector<std::string> validate_request(const BuildRequest& request);

std::string platform_name(TargetPlatform platform);

} // namespace cross_platform_builder
} // namespace os_generics

#endif
