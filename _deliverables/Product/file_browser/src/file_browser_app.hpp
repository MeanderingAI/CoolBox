#ifndef COOLBOX__PRODUCT_FILE_BROWSER_SRC_FILE_BROWSER_APP_HPP
#define COOLBOX__PRODUCT_FILE_BROWSER_SRC_FILE_BROWSER_APP_HPP

#include <memory>
#include <string>

#include "file_browser.hpp"
#if defined(COOLBOX_PRODUCT_INSTALLER_ABSTRACTIONS)
#include "installer_abstraction.hpp"
#endif
#include "workspace_dock_host.hpp"

namespace product {
namespace file_browser_app {

class FileBrowserApp {
public:
    enum class LeftView {
        Browser,
        Settings
    };

    explicit FileBrowserApp(const std::string& title = "CoolBox File Browser");
    ~FileBrowserApp();

    int run();

private:
    std::string title_;
    std::string selected_path_;
    std::string preview_text_;
    graphics::full_application_window::WorkspacePanelLayout panel_layout_;
    LeftView active_left_view_ = LeftView::Browser;
    tools::file_browser::FileBrowserComponent browser_;
#if defined(COOLBOX_PRODUCT_INSTALLER_ABSTRACTIONS)
    os_generics::installer::PackageInstallerPlan package_plan_;
    os_generics::installer::PreReleaseScreen pre_release_screen_;
#endif
    std::unique_ptr<graphics::full_application_window::WorkspaceDockHost> host_;

    void refresh_window();
    void set_root(const std::string& path);
    void set_left_view(LeftView view);
    void open_entry(int index);
    void update_models();
    void load_preview(const std::string& path);
};

} // namespace file_browser_app
} // namespace product

#endif