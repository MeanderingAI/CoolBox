#ifndef COOLBOX__LIBRARIES_BACKAGES_GRAPHICS_FULL_APPLICATION_WINDOW_HEADERS_WORKSPACE_DOCK_HOST_HPP
#define COOLBOX__LIBRARIES_BACKAGES_GRAPHICS_FULL_APPLICATION_WINDOW_HEADERS_WORKSPACE_DOCK_HOST_HPP

#include <functional>
#include <string>
#include <vector>

#include "../../graphics_object.hpp"
#include "full_application_window.hpp"

namespace graphics {
namespace full_application_window {

enum class WorkspaceDockArea {
    Left,
    Center,
    Right,
    Bottom
};

enum class WorkspacePanelJoint {
    LeftCenter,
    CenterRight,
    CenterBottom
};

struct WorkspacePanelLayout : public ::graphics::GraphicsObject {
    int left_width = 260;
    int right_width = 300;
    int bottom_height = 180;
    int joint_thickness = 6;

    WorkspacePanelLayout() = default;
    WorkspacePanelLayout(int left,
                         int right,
                         int bottom,
                         int thickness)
        : left_width(left)
        , right_width(right)
        , bottom_height(bottom)
        , joint_thickness(thickness) {}

    std::string graphics_object_kind() const override { return "workspacePanelLayout"; }
    std::string graphics_object_name() const override {
        return std::to_string(left_width) + "x" + std::to_string(right_width) + "x" + std::to_string(bottom_height);
    }
};

struct WorkspaceNavItem : public ::graphics::GraphicsObject {
    std::string icon;
    std::string label;

    WorkspaceNavItem() = default;
    WorkspaceNavItem(std::string nav_icon, std::string nav_label)
        : icon(std::move(nav_icon)), label(std::move(nav_label)) {}

    std::string graphics_object_kind() const override { return "workspaceNavItem"; }
    std::string graphics_object_name() const override { return label; }
};

struct WorkspaceDockModels : public ::graphics::GraphicsObject {
    std::string title;
    std::string editor_text;
    std::vector<WorkspaceNavItem> nav_items;
    int selected_nav_index = 0;
    std::string left_panel_title;
    std::vector<std::string> left_panel_entries;
    int selected_left_entry_index = -1;
    std::string inspector_text;
    std::string preview_text;
    std::string status_text;
    WorkspacePanelLayout layout;

    WorkspaceDockModels() = default;
    WorkspaceDockModels(std::string dock_title,
                        std::string dock_editor_text,
                        std::vector<WorkspaceNavItem> dock_nav_items,
                        int dock_selected_nav_index,
                        std::string dock_left_panel_title,
                        std::vector<std::string> dock_left_panel_entries,
                        int dock_selected_left_entry_index,
                        std::string dock_inspector_text,
                        std::string dock_preview_text,
                        std::string dock_status_text,
                        WorkspacePanelLayout dock_layout)
        : title(std::move(dock_title))
        , editor_text(std::move(dock_editor_text))
        , nav_items(std::move(dock_nav_items))
        , selected_nav_index(dock_selected_nav_index)
        , left_panel_title(std::move(dock_left_panel_title))
        , left_panel_entries(std::move(dock_left_panel_entries))
        , selected_left_entry_index(dock_selected_left_entry_index)
        , inspector_text(std::move(dock_inspector_text))
        , preview_text(std::move(dock_preview_text))
        , status_text(std::move(dock_status_text))
        , layout(std::move(dock_layout)) {}

    std::string graphics_object_kind() const override { return "workspaceDockModels"; }
    std::string graphics_object_name() const override { return title; }
};

struct WorkspaceDockCallbacks {
    std::function<void()> on_open_file;
    std::function<void()> on_open_workspace;
    std::function<void()> on_save;
    std::function<void()> on_save_as;
    std::function<void()> on_exit;
    std::function<void(int index)> on_left_item_activated;
    std::function<void(int index)> on_nav_selected;
    std::function<void(const std::string& text)> on_editor_text_changed;
    std::function<void(const WorkspacePanelLayout& layout)> on_layout_changed;
};

class WorkspaceDockHost : public ::graphics::GraphicsObject {
public:
    explicit WorkspaceDockHost(WindowConfig config = {});
    ~WorkspaceDockHost();

    void set_callbacks(WorkspaceDockCallbacks callbacks);
    bool create();
    void show();
    void close();
    bool pump_events();
    void request_redraw();

    void set_models(const WorkspaceDockModels& models);
    std::string backend_name() const;
    void set_platform_style(::graphics::windows::PlatformStyle platform_style);

    bool prompt_open_file(std::string& path);
    bool prompt_open_workspace(std::string& path);
    bool prompt_save_file(std::string& path);

    std::string graphics_object_kind() const override { return "workspaceDockHost"; }
    std::string graphics_object_name() const override { return backend_name(); }

private:
    struct Impl;
    Impl* impl_;
};

} // namespace full_application_window
} // namespace graphics

#endif  // COOLBOX__LIBRARIES_BACKAGES_GRAPHICS_FULL_APPLICATION_WINDOW_HEADERS_WORKSPACE_DOCK_HOST_HPP