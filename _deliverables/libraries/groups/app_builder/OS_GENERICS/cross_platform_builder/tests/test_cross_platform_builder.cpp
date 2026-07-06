#include "cross_platform_builder.hpp"

#include <iostream>

namespace {

int fail(const char* msg) {
    std::cerr << msg << "\n";
    return 1;
}

} // namespace

int main() {
    using namespace os_generics::cross_platform_builder;

    {
        BuildRequest req;
        req.project_root = "/repo";
        req.build_root = "/repo/build";
        req.app_target = "recording_studio";
        req.platform = TargetPlatform::AndroidSimulator;
        req.configuration = "Debug";

        const auto plan = make_build_plan(req);
        if (plan.configure_command.find("CMAKE_SYSTEM_NAME=Android") == std::string::npos) {
            return fail("android configure command missing Android system flag");
        }
        if (plan.build_command.find("--target recording_studio") == std::string::npos) {
            return fail("android build command missing target");
        }
    }

    {
        BuildRequest req;
        req.project_root = "/repo";
        req.build_root = "/repo/build";
        req.app_target = "recording_studio";
        req.platform = TargetPlatform::IosSimulator;
        req.configuration = "Release";

        const auto plan = make_build_plan(req);
        if (plan.configure_command.find("CMAKE_OSX_SYSROOT=iphonesimulator") == std::string::npos) {
            return fail("ios configure command missing iphonesimulator sysroot");
        }
        if (plan.build_command.find("--config Release") == std::string::npos) {
            return fail("ios build command missing configuration");
        }
    }

    return 0;
}
