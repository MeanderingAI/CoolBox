#include "file_browser_app.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>

namespace product {
namespace file_browser_app {

namespace {

std::string path_leaf_or_self(const std::string& path) {
    const std::filesystem::path fs_path(path);
    const std::string name = fs_path.filename().string();
    return name.empty() ? path : name;
}

bool path_is_directory(const std::string& path) {
    std::error_code error;
    return !path.empty() && std::filesystem::is_directory(std::filesystem::path(path), error);
}

} // namespace

FileBrowserApp::FileBrowserApp(const std::string& title)
    : title_(title),
#if defined(COOLBOX_PRODUCT_INSTALLER_ABSTRACTIONS)
      preview_text_(os_generics::installer::render_pre_release_text(
          os_generics::installer::create_pre_release_screen("File Browser", "file_browser"))),
#else
      preview_text_(),
#endif
      panel_layout_(),
      active_left_view_(LeftView::Browser),
      browser_(std::filesystem::current_path()),
#if defined(COOLBOX_PRODUCT_INSTALLER_ABSTRACTIONS)
      package_plan_(os_generics::installer::create_packaged_plan("File Browser", "file_browser")),
      pre_release_screen_(os_generics::installer::create_pre_release_screen("File Browser", "file_browser")),
#endif
      host_(std::make_unique<graphics::full_application_window::WorkspaceDockHost>(
          graphics::full_application_window::WindowConfig{title, 1180, 780, true, true})) {
    host_->set_callbacks({
        [this]() {
            std::string path;
            if (host_ && host_->prompt_open_file(path)) {
                load_preview(path);
                refresh_window();
            }
        },
        [this]() {
            std::string path;
            if (host_ && host_->prompt_open_workspace(path)) {
                set_root(path);
            }
        },
        []() {},
        []() {},
        [this]() {
            if (host_) {
                host_->close();
            }
        },
        [this](int index) {
            open_entry(index);
        },
        [this](int index) {
            set_left_view(index == 1 ? LeftView::Settings : LeftView::Browser);
        },
        [this](const std::string& text) {
            preview_text_ = text;
        },
        [this](const graphics::full_application_window::WorkspacePanelLayout& layout) {
            panel_layout_ = layout;
        }
    });
}

FileBrowserApp::~FileBrowserApp() = default;

void FileBrowserApp::set_root(const std::string& path) {
    browser_.set_root(path);
    selected_path_.clear();
    preview_text_.clear();
    refresh_window();
}

void FileBrowserApp::set_left_view(LeftView view) {
    active_left_view_ = view;
    refresh_window();
}

void FileBrowserApp::load_preview(const std::string& path) {
    selected_path_ = path;
    if (path_is_directory(path)) {
        preview_text_ = "Directory: " + path + "\nDouble-click a file to preview it.";
        return;
    }

    std::ifstream input(path, std::ios::binary);
    if (!input) {
        preview_text_ = "Unable to open: " + path;
        return;
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    preview_text_ = buffer.str();
}

void FileBrowserApp::open_entry(int index) {
    if (active_left_view_ != LeftView::Browser) {
        return;
    }

    const tools::file_browser::FileEntry* entry = browser_.entry_at(index);
    if (!entry) {
        return;
    }

    if (entry->is_directory) {
        set_root(entry->path);
        return;
    }

    load_preview(entry->path);
    refresh_window();
}

void FileBrowserApp::refresh_window() {
    if (host_) {
        update_models();
        host_->request_redraw();
    }
}

void FileBrowserApp::update_models() {
    if (!host_) {
        return;
    }

    std::ostringstream inspector;
    inspector << "Root: " << (browser_.has_root() ? browser_.root().string() : std::string("(none)")) << "\r\n"
              << "Entries: " << browser_.entries().size() << "\r\n"
              << "Selected: " << (selected_path_.empty() ? std::string("(none)") : selected_path_) << "\r\n"
              << "Backend: " << host_->backend_name();

#if defined(COOLBOX_PRODUCT_INSTALLER_ABSTRACTIONS)
    inspector << "\r\nInstall Mode: " << os_generics::installer::to_string(package_plan_.experience);
#endif

    std::ostringstream preview;
#if defined(COOLBOX_PRODUCT_INSTALLER_ABSTRACTIONS)
    if (selected_path_.empty() && preview_text_.empty()) {
        preview << os_generics::installer::render_pre_release_text(pre_release_screen_);
    } else {
        preview << "File Browser\r\n------------\r\n"
                << (browser_.has_root() ? browser_.root().string() : std::string("Use File > Open Workspace to choose a root."));
    }
    #else
        preview << "File Browser\r\n------------\r\n"
            << (browser_.has_root() ? browser_.root().string() : std::string("Use File > Open Workspace to choose a root."));
    #endif

    std::vector<std::string> left_entries;
    int selected_left_entry_index = -1;
    std::string left_title;
    if (active_left_view_ == LeftView::Browser) {
        left_title = "Browser";
        for (const auto& entry : browser_.entries()) {
            left_entries.push_back(entry.label);
        }
        selected_left_entry_index = browser_.selected_index_for_path(selected_path_);
    } else {
        left_title = "Settings";
        left_entries = {
            std::string("Root: ") + (browser_.has_root() ? browser_.root().string() : std::string("(none)")),
            std::string("Left width: ") + std::to_string(panel_layout_.left_width),
            std::string("Right width: ") + std::to_string(panel_layout_.right_width),
            std::string("Bottom height: ") + std::to_string(panel_layout_.bottom_height),
            std::string("Joint thickness: ") + std::to_string(panel_layout_.joint_thickness)
        };
    }

    host_->set_models({
        title_ + (selected_path_.empty() ? std::string() : std::string(" - ") + path_leaf_or_self(selected_path_)),
        preview_text_.empty() ? preview.str() : preview_text_,
        {{"[F]", "Files"}, {"[S]", "Settings"}},
        active_left_view_ == LeftView::Browser ? 0 : 1,
        left_title,
        left_entries,
        selected_left_entry_index,
        inspector.str(),
        preview.str(),
#if defined(COOLBOX_PRODUCT_INSTALLER_ABSTRACTIONS)
        std::string(os_generics::installer::to_string(package_plan_.experience)) + " | " +
            (browser_.has_root() ? std::string("Browsing ") + browser_.root().string() : std::string("No root selected")),
#else
        browser_.has_root() ? std::string("Browsing ") + browser_.root().string() : std::string("No root selected"),
#endif
        panel_layout_
    });
}

int FileBrowserApp::run() {
    if (!host_ || !host_->create()) {
        return 1;
    }

    browser_.refresh();
    update_models();
    host_->show();
    while (host_->pump_events()) {
    }
    return 0;
}

} // namespace file_browser_app
} // namespace product