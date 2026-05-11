#include "installer_abstraction.hpp"

#include <sstream>

namespace os_generics {
namespace installer {

InstallerExperience detect_installer_experience() {
#if defined(_WIN32)
    return InstallerExperience::InstallGenie;
#elif defined(__APPLE__)
    return InstallerExperience::DragAndDropBundle;
#else
    return InstallerExperience::PortableBinary;
#endif
}

const char* to_string(InstallerExperience experience) {
    switch (experience) {
    case InstallerExperience::InstallGenie:
        return "Install Genie";
    case InstallerExperience::DragAndDropBundle:
        return "Drag And Drop Bundle";
    case InstallerExperience::PortableBinary:
        return "Portable Binary";
    }
    return "Portable Binary";
}

const char* to_string(ReleaseChannel channel) {
    switch (channel) {
    case ReleaseChannel::PreRelease:
        return "pre-release";
    case ReleaseChannel::Stable:
        return "stable";
    }
    return "pre-release";
}

PackageInstallerPlan create_packaged_plan(const std::string& product_name,
                                          const std::string& executable_name,
                                          ReleaseChannel channel) {
    PackageInstallerPlan plan;
    plan.product_name = product_name;
    plan.executable_name = executable_name;
    plan.experience = detect_installer_experience();
    plan.channel = channel;

    switch (plan.experience) {
    case InstallerExperience::InstallGenie:
        plan.package_hint = "Ship the prerelease build with a lightweight Windows installer wrapper so testers enter through an install genie instead of a raw binary drop.";
        plan.install_hint = "Review the prerelease notice, then run the Windows install genie entrypoint and accept the staged destination before launch.";
        plan.launch_hint = "Launch the installed product from the Start Menu shortcut or the installed application folder once the wizard completes.";
        break;
    case InstallerExperience::DragAndDropBundle:
        plan.package_hint = "Ship the prerelease build as a drag-and-drop macOS package with the app bundle presented beside Applications.";
        plan.install_hint = "Review the prerelease notice, then drag the app bundle into Applications before first launch.";
        plan.launch_hint = "Launch the app from Applications after the bundle copy finishes.";
        break;
    case InstallerExperience::PortableBinary:
        plan.package_hint = "Ship the prerelease build as a portable archive that keeps the binary and runtime libraries together.";
        plan.install_hint = "Review the prerelease notice, extract the archive into a writable folder, and keep the shipped runtime libraries beside the binary.";
        plan.launch_hint = "Run the shipped binary directly from the extracted folder.";
        break;
    }

    return plan;
}

PreReleaseScreen create_pre_release_screen(const std::string& product_name,
                                           const std::string& executable_name,
                                           ReleaseChannel channel) {
    const PackageInstallerPlan plan = create_packaged_plan(product_name, executable_name, channel);

    PreReleaseScreen screen;
    screen.title = product_name + " Pre-Release";
    screen.summary = "This build is packaged as a " + std::string(to_string(plan.experience)) +
                     " preview so install expectations are explicit before a final release channel is cut.";
    screen.package_hint = plan.package_hint;
    screen.install_hint = plan.install_hint;
    screen.launch_hint = plan.launch_hint;
    screen.disclaimer = "Use this build for validation and feedback. Packaging details may still change before stable release.";
    screen.next_steps = {
        "Review the packaged install path for your operating system.",
        "Validate startup, runtime dependencies, and basic workflow coverage.",
        "Report anything that should change before the stable packaging path is locked."
    };
    return screen;
}

std::string render_installation_notes(const PackageInstallerPlan& plan) {
    std::ostringstream stream;
    stream << plan.product_name << " installation notes\n"
           << "Channel: " << to_string(plan.channel) << "\n"
           << "Installer mode: " << to_string(plan.experience) << "\n\n"
           << "Package: " << plan.package_hint << "\n"
           << "Install: " << plan.install_hint << "\n"
           << "Launch: " << plan.launch_hint;
    return stream.str();
}

std::string render_pre_release_text(const PreReleaseScreen& screen) {
    std::ostringstream stream;
    stream << screen.title << "\n"
           << "====================\n"
           << screen.summary << "\n\n"
           << "Package Path\n"
           << "------------\n"
           << screen.package_hint << "\n\n"
           << "Install Path\n"
           << "------------\n"
           << screen.install_hint << "\n\n"
           << "Launch Path\n"
           << "-----------\n"
           << screen.launch_hint << "\n\n"
           << "Pre-Release Notice\n"
           << "------------------\n"
           << screen.disclaimer << "\n\n"
           << "Next Steps\n"
           << "----------\n";

    for (std::size_t index = 0; index < screen.next_steps.size(); ++index) {
        stream << (index + 1) << ". " << screen.next_steps[index] << "\n";
    }

    return stream.str();
}

} // namespace installer
} // namespace os_generics