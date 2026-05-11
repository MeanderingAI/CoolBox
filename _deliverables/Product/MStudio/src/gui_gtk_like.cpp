#include "gui_gtk_like.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <system_error>

#include "components.hpp"

namespace product {
namespace mstudio {

namespace {

namespace fs = std::filesystem;

std::vector<std::string> split_lines(const std::string& text) {
    std::vector<std::string> lines;
    std::string current;
    for (char ch : text) {
        if (ch == '\r') {
            continue;
        }
        if (ch == '\n') {
            lines.push_back(current);
            current.clear();
            continue;
        }
        current.push_back(ch);
    }
    lines.push_back(current);
    if (lines.empty()) {
        lines.push_back("");
    }
    return lines;
}

std::vector<std::string> build_initial_lines() {
#if defined(COOLBOX_PRODUCT_INSTALLER_ABSTRACTIONS)
    return split_lines(os_generics::installer::render_pre_release_text(
        os_generics::installer::create_pre_release_screen("MStudio", "MStudio")));
#else
    return {
        "Welcome to CoolBox MStudio",
        "This editor window is hosted by graphics::full_application_window and uses graphics component models."
    };
#endif
}

std::string to_windows_newlines(const std::string& text) {
    std::string converted;
    converted.reserve(text.size() * 2U);
    for (char ch : text) {
        if (ch == '\n') {
            converted += "\r\n";
        } else {
            converted.push_back(ch);
        }
    }
    return converted;
}

std::string path_leaf_or_self(const std::string& path) {
    const fs::path fs_path(path);
    const std::string name = fs_path.filename().string();
    return name.empty() ? path : name;
}

bool path_is_directory(const std::string& path) {
    std::error_code error;
    return !path.empty() && fs::is_directory(fs::path(path), error);
}

} // namespace

GtkLikeApp::GtkLikeApp(const std::string& title)
    : title_(title),
      buffer_(build_initial_lines()),
      current_path_(),
      workspace_root_(),
      panel_layout_(),
      active_left_view_(LeftView::Workspace),
      cursor_row_(0),
      cursor_col_(0),
      dirty_(false),
      file_browser_(),
#if defined(COOLBOX_PRODUCT_INSTALLER_ABSTRACTIONS)
    package_plan_(os_generics::installer::create_packaged_plan("MStudio", "MStudio")),
    pre_release_screen_(os_generics::installer::create_pre_release_screen("MStudio", "MStudio")),
#endif
      host_(std::make_unique<graphics::full_application_window::WorkspaceDockHost>(
          graphics::full_application_window::WindowConfig{title, 1280, 800, true, true})) {
    host_->set_callbacks({
        [this]() {
            std::string path;
            if (host_ && host_->prompt_open_file(path)) {
                openFile(path);
            }
        },
        [this]() {
            std::string path;
            if (host_ && host_->prompt_open_workspace(path)) {
                load_workspace(path);
            }
        },
        [this]() {
            if (current_path_.empty()) {
                std::string path;
                if (host_ && host_->prompt_save_file(path)) {
                    saveFile(path);
                }
            } else {
                saveFile(current_path_);
            }
        },
        [this]() {
            std::string path = current_path_;
            if (host_ && host_->prompt_save_file(path)) {
                saveFile(path);
            }
        },
        [this]() {
            if (host_) {
                host_->close();
            }
        },
        [this](int index) {
            open_workspace_entry(index);
        },
        [this](int index) {
            if (index == 1) {
                set_left_view(LeftView::Settings);
            } else {
                set_left_view(LeftView::Workspace);
            }
        },
        [this](const std::string& text) {
            replace_buffer_from_text(text);
            set_dirty(true);
        },
        [this](const graphics::full_application_window::WorkspacePanelLayout& layout) {
            panel_layout_ = layout;
        }
    });
}

GtkLikeApp::~GtkLikeApp() = default;

std::string GtkLikeApp::joined_buffer() const {
    std::ostringstream stream;
    for (std::size_t index = 0; index < buffer_.size(); ++index) {
        if (index != 0) {
            stream << '\n';
        }
        stream << buffer_[index];
    }
    return stream.str();
}

void GtkLikeApp::replace_buffer_from_text(const std::string& text) {
    buffer_ = split_lines(text);
    if (buffer_.empty()) {
        buffer_.push_back("");
    }
    cursor_row_ = std::min(cursor_row_, buffer_.size() - 1U);
    cursor_col_ = std::min(cursor_col_, buffer_[cursor_row_].size());
}

void GtkLikeApp::openFile(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    buffer_.clear();
    if (!input) {
        current_path_.clear();
        buffer_.push_back("");
        return;
    }

    std::string contents((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    replace_buffer_from_text(contents);
    current_path_ = path;
    cursor_row_ = 0;
    cursor_col_ = 0;
    dirty_ = false;
    rebuild_workspace_items();
    refresh_window();
}

bool GtkLikeApp::saveFile(const std::string& path) {
    std::ofstream output(path, std::ios::binary);
    if (!output) {
        return false;
    }
    output << joined_buffer();
    current_path_ = path;
    rebuild_workspace_items();
    set_dirty(false);
    refresh_window();
    return true;
}

void GtkLikeApp::refresh_window() {
    if (host_) {
        update_component_views();
        update_window_title();
        update_host_models();
        host_->request_redraw();
    }
}

void GtkLikeApp::set_dirty(bool dirty) {
    dirty_ = dirty;
    update_window_title();
    update_component_views();
}

void GtkLikeApp::update_window_title() {
    if (!host_) {
        return;
    }

    std::string caption = title_;
    caption += " - ";
    caption += current_path_.empty() ? "Untitled" : current_path_;
    if (!workspace_root_.empty()) {
        caption += " [";
        caption += path_leaf_or_self(workspace_root_);
        caption += "]";
    }
    if (dirty_) {
        caption += " *";
    }
    if (host_) {
        update_host_models();
    }
}

void GtkLikeApp::load_workspace(const std::string& path) {
    if (!path_is_directory(path)) {
        return;
    }
    workspace_root_ = path;
    file_browser_.set_root(path);
    rebuild_workspace_items();
    refresh_window();
}

void GtkLikeApp::rebuild_workspace_items() {
    if (path_is_directory(workspace_root_)) {
        file_browser_.set_root(workspace_root_);
        return;
    }

    if (!current_path_.empty()) {
        file_browser_.set_root(fs::path(current_path_).parent_path());
        return;
    }

    file_browser_ = tools::file_browser::FileBrowserComponent();
}

void GtkLikeApp::open_workspace_entry(int index) {
    if (active_left_view_ != LeftView::Workspace) {
        return;
    }
    const tools::file_browser::FileEntry* entry = file_browser_.entry_at(index);
    if (!entry) {
        return;
    }
    if (entry->is_directory || entry->path.empty()) {
        return;
    }
    openFile(entry->path);
}

void GtkLikeApp::set_left_view(LeftView view) {
    if (active_left_view_ == view) {
        return;
    }
    active_left_view_ = view;
    refresh_window();
}

void GtkLikeApp::update_component_views() {
    using namespace graphics::components;

    PropertyInspectorModel properties;
    properties.add_property("Path", current_path_.empty() ? "(unsaved)" : current_path_)
              .add_property("Workspace", workspace_root_.empty() ? "(none)" : workspace_root_)
              .add_property("Lines", std::to_string(buffer_.size()))
              .add_property("Characters", std::to_string(joined_buffer().size()))
              .add_property("Backend", host_ ? host_->backend_name() : "Unknown")
              .add_property("Modified", dirty_ ? "yes" : "no");
}

void GtkLikeApp::update_host_models() {
    if (!host_) {
        return;
    }

    std::ostringstream inspector_stream;
    using namespace graphics::components;
    PropertyInspectorModel properties;
    properties.add_property("Path", current_path_.empty() ? "(unsaved)" : current_path_)
              .add_property("Workspace", workspace_root_.empty() ? "(none)" : workspace_root_)
              .add_property("Lines", std::to_string(buffer_.size()))
              .add_property("Characters", std::to_string(joined_buffer().size()))
              .add_property("Backend", host_->backend_name())
              .add_property("Modified", dirty_ ? "yes" : "no");
    const auto inspector_lines = Component::property_inspector(properties).render();
    for (std::size_t index = 0; index < inspector_lines.size(); ++index) {
        if (index != 0) {
            inspector_stream << "\r\n";
        }
        inspector_stream << inspector_lines[index];
    }

    std::ostringstream preview_stream;
#if defined(COOLBOX_PRODUCT_INSTALLER_ABSTRACTIONS)
    if (workspace_root_.empty() && current_path_.empty()) {
        preview_stream << os_generics::installer::render_pre_release_text(pre_release_screen_);
    } else {
        preview_stream << "Workspace\r\n---------\r\n";
        preview_stream << (workspace_root_.empty() ? "Use File > Open Workspace to load a folder."
                                                   : workspace_root_)
                       << "\r\n\r\nActive File\r\n-----------\r\n"
                       << (current_path_.empty() ? "Untitled" : current_path_);
    }
#else
    preview_stream << "Workspace\r\n---------\r\n";
    preview_stream << (workspace_root_.empty() ? "Use File > Open Workspace to load a folder."
                                               : workspace_root_)
                   << "\r\n\r\nActive File\r\n-----------\r\n"
                   << (current_path_.empty() ? "Untitled" : current_path_);
#endif

    std::ostringstream status;
#if defined(COOLBOX_PRODUCT_INSTALLER_ABSTRACTIONS)
    status << os_generics::installer::to_string(package_plan_.experience)
           << " | ";
#endif
    status << (workspace_root_.empty() ? "No workspace" : path_leaf_or_self(workspace_root_))
           << " | "
           << (current_path_.empty() ? "Untitled" : path_leaf_or_self(current_path_))
           << " | Lines: " << buffer_.size()
           << " | Chars: " << joined_buffer().size()
           << (dirty_ ? " | Modified" : " | Saved");

    std::vector<std::string> labels;
    std::string left_panel_title;
    int selected_left_entry_index = -1;

    if (active_left_view_ == LeftView::Workspace) {
        left_panel_title = "Workspace";
        labels.reserve(file_browser_.entries().size());
        for (const auto& entry : file_browser_.entries()) {
            labels.push_back(entry.label);
        }
        selected_left_entry_index = file_browser_.selected_index_for_path(current_path_);
        if (labels.empty()) {
            labels.push_back("Open Workspace from the File menu");
        }
    } else {
        left_panel_title = "Settings";
        labels = {
            std::string("Theme: Windows native"),
            std::string("Backend: ") + host_->backend_name(),
            std::string("Workspace: ") + (workspace_root_.empty() ? "(none)" : workspace_root_),
            std::string("Left width: ") + std::to_string(panel_layout_.left_width),
            std::string("Right width: ") + std::to_string(panel_layout_.right_width),
            std::string("Bottom height: ") + std::to_string(panel_layout_.bottom_height),
            std::string("Dirty buffer: ") + (dirty_ ? "yes" : "no")
        };
    }

    host_->set_models({
        title_ + " - " + (current_path_.empty() ? std::string("Untitled") : current_path_) +
            (workspace_root_.empty() ? std::string() : std::string(" [") + path_leaf_or_self(workspace_root_) + "]") +
            (dirty_ ? std::string(" *") : std::string()),
        to_windows_newlines(joined_buffer()),
        {{"[W]", "Workspace"}, {"[S]", "Settings"}},
        active_left_view_ == LeftView::Workspace ? 0 : 1,
        left_panel_title,
        labels,
        selected_left_entry_index,
        inspector_stream.str(),
        preview_stream.str(),
        status.str(),
        panel_layout_
    });
}

int GtkLikeApp::run() {
    if (!host_ || !host_->create()) {
        return 1;
    }

    rebuild_workspace_items();
    update_component_views();
    update_window_title();
    update_host_models();
    host_->show();
    while (host_->pump_events()) {
    }
    return 0;
}

} // namespace mstudio
} // namespace product
