#ifndef COOLBOX__LIBRARIES_PACKAGES_OS_GENERICS_INSTALLER_ABSTRACTION_HEADERS_INSTALLER_ABSTRACTION_HPP
#define COOLBOX__LIBRARIES_PACKAGES_OS_GENERICS_INSTALLER_ABSTRACTION_HEADERS_INSTALLER_ABSTRACTION_HPP

#include <string>
#include <vector>

namespace os_generics {
namespace installer {

enum class InstallerExperience {
    InstallGenie,
    DragAndDropBundle,
    PortableBinary
};

enum class ReleaseChannel {
    PreRelease,
    Stable
};

struct PackageInstallerPlan {
    std::string product_name;
    std::string executable_name;
    InstallerExperience experience = InstallerExperience::PortableBinary;
    ReleaseChannel channel = ReleaseChannel::PreRelease;
    std::string install_hint;
    std::string package_hint;
    std::string launch_hint;
};

struct PreReleaseScreen {
    std::string title;
    std::string summary;
    std::string package_hint;
    std::string install_hint;
    std::string launch_hint;
    std::string disclaimer;
    std::vector<std::string> next_steps;
};

InstallerExperience detect_installer_experience();
const char* to_string(InstallerExperience experience);
const char* to_string(ReleaseChannel channel);

PackageInstallerPlan create_packaged_plan(const std::string& product_name,
                                          const std::string& executable_name,
                                          ReleaseChannel channel = ReleaseChannel::PreRelease);

PreReleaseScreen create_pre_release_screen(const std::string& product_name,
                                           const std::string& executable_name,
                                           ReleaseChannel channel = ReleaseChannel::PreRelease);

std::string render_installation_notes(const PackageInstallerPlan& plan);
std::string render_pre_release_text(const PreReleaseScreen& screen);

} // namespace installer
} // namespace os_generics

#endif