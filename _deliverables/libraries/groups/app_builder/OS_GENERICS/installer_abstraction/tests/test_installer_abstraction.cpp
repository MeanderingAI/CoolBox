#include "installer_abstraction.hpp"
#include "tyst_framework.hpp"

TEST(InstallerAbstraction, PreReleaseTextIncludesTitleAndSteps) {
    const auto screen = os_generics::installer::create_pre_release_screen("File Browser", "file_browser");
    const std::string rendered = os_generics::installer::render_pre_release_text(screen);

    EXPECT_NE(rendered.find("File Browser Pre-Release"), std::string::npos);
    EXPECT_NE(rendered.find("Next Steps"), std::string::npos);
}

TEST(InstallerAbstraction, InstallationNotesReportMode) {
    const auto plan = os_generics::installer::create_packaged_plan("Worksplace Editor", "worksplace_editor");
    const std::string notes = os_generics::installer::render_installation_notes(plan);

    EXPECT_NE(notes.find("Installer mode:"), std::string::npos);
    EXPECT_NE(notes.find(os_generics::installer::to_string(plan.experience)), std::string::npos);
}