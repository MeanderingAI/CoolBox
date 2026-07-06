#include "cross_platform_builder.hpp"

#include <sstream>

namespace os_generics {
namespace cross_platform_builder {
namespace {

std::string quote(const std::string& value) {
    if (value.find(' ') == std::string::npos) {
        return value;
    }
    return "\"" + value + "\"";
}

std::string platform_build_dir(const BuildRequest& request) {
    if (request.platform == TargetPlatform::AndroidSimulator) {
        return request.build_root + "/android-sim";
    }
    return request.build_root + "/ios-sim";
}

} // namespace

std::vector<std::string> validate_request(const BuildRequest& request) {
    std::vector<std::string> errors;
    if (request.project_root.empty()) {
        errors.push_back("project_root is required");
    }
    if (request.build_root.empty()) {
        errors.push_back("build_root is required");
    }
    if (request.app_target.empty()) {
        errors.push_back("app_target is required");
    }
    if (request.configuration.empty()) {
        errors.push_back("configuration is required");
    }

    if (request.platform == TargetPlatform::AndroidSimulator) {
        if (request.android_ndk_env.empty()) {
            errors.push_back("android_ndk_env is required for Android simulator builds");
        }
        if (request.android_abi.empty()) {
            errors.push_back("android_abi is required for Android simulator builds");
        }
        if (request.android_api.empty()) {
            errors.push_back("android_api is required for Android simulator builds");
        }
    } else {
        if (request.ios_generator.empty()) {
            errors.push_back("ios_generator is required for iOS simulator builds");
        }
        if (request.ios_arch.empty()) {
            errors.push_back("ios_arch is required for iOS simulator builds");
        }
        if (request.ios_deployment_target.empty()) {
            errors.push_back("ios_deployment_target is required for iOS simulator builds");
        }
    }

    return errors;
}

BuildPlan make_build_plan(const BuildRequest& request) {
    BuildPlan plan;
    const std::string build_dir = platform_build_dir(request);

    std::ostringstream configure;
    configure << "cmake -S " << quote(request.project_root)
              << " -B " << quote(build_dir);

    if (request.platform == TargetPlatform::AndroidSimulator) {
        configure << " -DCMAKE_SYSTEM_NAME=Android"
                  << " -DCMAKE_ANDROID_ARCH_ABI=" << request.android_abi
                  << " -DCMAKE_SYSTEM_VERSION=" << request.android_api
                  << " -DCMAKE_ANDROID_NDK=$" << request.android_ndk_env
                  << " -DCMAKE_BUILD_TYPE=" << request.configuration;

        plan.notes.push_back("Android simulator plan expects a valid NDK in env var: " + request.android_ndk_env);
        plan.notes.push_back("Use android_abi=x86_64 for classic emulators or x86 for legacy images.");
    } else {
        configure << " -G " << quote(request.ios_generator)
                  << " -DCMAKE_SYSTEM_NAME=iOS"
                  << " -DCMAKE_OSX_SYSROOT=iphonesimulator"
                  << " -DCMAKE_OSX_ARCHITECTURES=" << request.ios_arch
                  << " -DCMAKE_OSX_DEPLOYMENT_TARGET=" << request.ios_deployment_target;

        plan.notes.push_back("iOS simulator plan uses iphonesimulator SDK and Xcode generator by default.");
        plan.notes.push_back("Use ios_arch=arm64 on Apple Silicon; use x86_64 for Intel-only simulator setups.");
    }

    std::ostringstream build;
    build << "cmake --build " << quote(build_dir)
          << " --config " << request.configuration
          << " --target " << request.app_target;

    plan.configure_command = configure.str();
    plan.build_command = build.str();
    return plan;
}

std::string platform_name(TargetPlatform platform) {
    if (platform == TargetPlatform::AndroidSimulator) {
        return "android-simulator";
    }
    return "ios-simulator";
}

} // namespace cross_platform_builder
} // namespace os_generics
