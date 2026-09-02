#ifndef COOLBOX__PRODUCT_MSTUDIO_SRC_GUI_GTK_LIKE_HPP
#define COOLBOX__PRODUCT_MSTUDIO_SRC_GUI_GTK_LIKE_HPP
#include <memory>
#include <string>
#include <vector>
#include <cstddef>

#include "file_browser.hpp"
#if defined(COOLBOX_PRODUCT_INSTALLER_ABSTRACTIONS)
#include "installer_abstraction.hpp"
#endif
#include "workspace_dock_host.hpp"

namespace product {
namespace mstudio {

class GtkLikeApp {
public:
    enum class LeftView {
        Workspace,
        Settings
    };

    explicit GtkLikeApp(const std::string &title = "CoolBox MStudio");
    ~GtkLikeApp();

    int run();
    void openFile(const std::string &path);
    bool saveFile(const std::string &path);

    struct WorkspaceItem {
        std::string label;
        std::string path;
        bool is_directory = false;
    };

private:
    std::string title_;
    std::vector<std::string> buffer_;
    std::string current_path_;
    std::string workspace_root_;
    graphics::full_application_window::WorkspacePanelLayout panel_layout_;
    LeftView active_left_view_ = LeftView::Workspace;
    std::size_t cursor_row_ = 0;
    std::size_t cursor_col_ = 0;
    bool dirty_ = false;
    tools::file_browser::FileBrowserComponent file_browser_;
#if defined(COOLBOX_PRODUCT_INSTALLER_ABSTRACTIONS)
    os_generics::installer::PackageInstallerPlan package_plan_;
    os_generics::installer::PreReleaseScreen pre_release_screen_;
#endif

    std::string joined_buffer() const;
    void replace_buffer_from_text(const std::string& text);
    void refresh_window();

    std::unique_ptr<graphics::full_application_window::WorkspaceDockHost> host_;
    void update_host_models();
    void update_component_views();
    void update_window_title();
    void set_dirty(bool dirty);
    void load_workspace(const std::string& path);
    void rebuild_workspace_items();
    void open_workspace_entry(int index);
    void set_left_view(LeftView view);
};

} // namespace mstudio
} // namespace product
#endif  // COOLBOX__PRODUCT_MSTUDIO_SRC_GUI_GTK_LIKE_HPP
