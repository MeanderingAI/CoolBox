#include "workspace_dock_host.hpp"

#include <algorithm>
#include <cstdio>
#include <sstream>
#include <utility>

#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <objbase.h>
#include <shobjidl.h>
#endif

namespace graphics {
namespace full_application_window {

namespace {

constexpr int kLeftDockWidth = 260;
constexpr int kRightDockWidth = 300;
constexpr int kBottomDockHeight = 180;
constexpr int kStatusHeight = 24;
constexpr int kPanelHeaderHeight = 28;
constexpr int kPadding = 8;
constexpr int kMinimumPanelExtent = 120;
constexpr int kMinimumBottomHeight = 100;

#if defined(_WIN32)
constexpr const char* kHostWindowProperty = "CoolBoxWorkspaceDockHost";
constexpr const char* kPanelHeaderProperty = "CoolBoxWorkspaceDockPanelHeader";
constexpr const char* kPanelJointProperty = "CoolBoxWorkspaceDockPanelJoint";

enum ControlId {
    kIdEditor = 1100,
    kIdFileTree = 1101,
    kIdInspector = 1102,
    kIdStatus = 1103,
    kIdPreview = 1104,
    kIdNavRail = 1105,
    kIdLeftTitle = 1106,
    kIdMenuOpen = 40001,
    kIdMenuSave = 40002,
    kIdMenuSaveAs = 40003,
    kIdMenuExit = 40004,
    kIdMenuOpenWorkspace = 40005
};

HMENU build_native_menu_bar() {
    HMENU menu_bar = CreateMenu();
    HMENU file_popup = CreatePopupMenu();
    AppendMenuA(file_popup, MF_STRING, kIdMenuOpen, "&Open...");
    AppendMenuA(file_popup, MF_STRING, kIdMenuOpenWorkspace, "Open &Workspace...");
    AppendMenuA(file_popup, MF_SEPARATOR, 0, nullptr);
    AppendMenuA(file_popup, MF_STRING, kIdMenuSave, "&Save");
    AppendMenuA(file_popup, MF_STRING, kIdMenuSaveAs, "Save &As...");
    AppendMenuA(file_popup, MF_SEPARATOR, 0, nullptr);
    AppendMenuA(file_popup, MF_STRING, kIdMenuExit, "E&xit");
    AppendMenuA(menu_bar, MF_POPUP, reinterpret_cast<UINT_PTR>(file_popup), "File");
    return menu_bar;
}

std::string wide_to_utf8(const wchar_t* value) {
    if (value == nullptr) {
        return {};
    }

    const int required = WideCharToMultiByte(CP_UTF8, 0, value, -1, nullptr, 0, nullptr, nullptr);
    if (required <= 1) {
        return {};
    }

    std::string converted(static_cast<std::size_t>(required - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value, -1, converted.data(), required, nullptr, nullptr);
    return converted;
}
#endif

graphics::components::MenuBarModel build_menu_model() {
    using namespace graphics::components;

    MenuModel file_menu("File");
    file_menu.add_item(MenuItem::action("Open", "Ctrl+O"))
             .add_item(MenuItem::action("Open Workspace", "Ctrl+Shift+O"))
             .add_item(MenuItem::action("Save", "Ctrl+S"))
             .add_item(MenuItem::action("Save As", "Ctrl+Shift+S"));

    MenuBarModel menu_bar;
    menu_bar.add_menu(file_menu);
    return menu_bar;
}

} // namespace

struct WorkspaceDockHost::Impl {
    WindowConfig config;
    WorkspaceDockCallbacks callbacks;
    WorkspaceDockModels models;
    FullApplicationWindow window;
    bool suppress_change_notifications = false;

#if defined(_WIN32)
    struct DockPanel {
        std::string id;
        std::string title;
        WorkspaceDockArea area = WorkspaceDockArea::Center;
        HWND container = nullptr;
        HWND header = nullptr;
        HWND body = nullptr;
        WNDPROC original_header_proc = nullptr;
    };

    struct PanelJointHandle {
        WorkspacePanelJoint joint = WorkspacePanelJoint::LeftCenter;
        HWND handle = nullptr;
        WNDPROC original_proc = nullptr;
    };

    HWND window_handle = nullptr;
    HWND editor = nullptr;
    HWND left_panel = nullptr;
    HWND nav_rail = nullptr;
    HWND left_title = nullptr;
    HWND file_tree = nullptr;
    HWND inspector = nullptr;
    HWND preview = nullptr;
    HWND status = nullptr;
    HFONT ui_font = nullptr;
    HMENU menu_bar = nullptr;
    WNDPROC original_window_proc = nullptr;
    std::vector<DockPanel> dock_panels;
    std::vector<PanelJointHandle> panel_joints;
    int active_drag_panel_index = -1;
    WorkspaceDockArea active_drop_area = WorkspaceDockArea::Center;
    WorkspacePanelJoint active_joint = WorkspacePanelJoint::LeftCenter;
    bool dragging_joint = false;
    POINT joint_drag_start{};
    WorkspacePanelLayout joint_drag_origin_layout{};

    explicit Impl(WindowConfig window_config)
        : config(std::move(window_config)),
          window(config) {}

    const char* dock_area_name(WorkspaceDockArea area) const {
        switch (area) {
        case WorkspaceDockArea::Left:
            return "left";
        case WorkspaceDockArea::Center:
            return "center";
        case WorkspaceDockArea::Right:
            return "right";
        case WorkspaceDockArea::Bottom:
            return "bottom";
        }
        return "center";
    }

    const char* joint_name(WorkspacePanelJoint joint) const {
        switch (joint) {
        case WorkspacePanelJoint::LeftCenter:
            return "left/center";
        case WorkspacePanelJoint::CenterRight:
            return "center/right";
        case WorkspacePanelJoint::CenterBottom:
            return "center/bottom";
        }
        return "joint";
    }

    int find_panel_index_by_header(HWND hwnd) const {
        for (std::size_t index = 0; index < dock_panels.size(); ++index) {
            if (dock_panels[index].header == hwnd) {
                return static_cast<int>(index);
            }
        }
        return -1;
    }

    RECT dock_area_rect(WorkspaceDockArea area, const RECT& bounds) const {
        const auto has_panels = [this](WorkspaceDockArea check_area) {
            return std::any_of(dock_panels.begin(), dock_panels.end(), [check_area](const DockPanel& panel) {
                return panel.area == check_area;
            });
        };

        const bool has_left = has_panels(WorkspaceDockArea::Left);
        const bool has_right = has_panels(WorkspaceDockArea::Right);
        const bool has_bottom = has_panels(WorkspaceDockArea::Bottom);

        const int left_width = has_left ? models.layout.left_width : 0;
        const int right_width = has_right ? models.layout.right_width : 0;
        const int bottom_height = has_bottom ? models.layout.bottom_height : 0;
        const int center_left = bounds.left + left_width + (has_left ? kPadding : 0);
        const int center_right = bounds.right - right_width - (has_right ? kPadding : 0);
        const int center_bottom = bounds.bottom - bottom_height - (has_bottom ? kPadding : 0);

        switch (area) {
        case WorkspaceDockArea::Left:
            return RECT{bounds.left, bounds.top, center_left - (has_left ? kPadding : 0), center_bottom};
        case WorkspaceDockArea::Center:
            return RECT{center_left, bounds.top, center_right, center_bottom};
        case WorkspaceDockArea::Right:
            return RECT{center_right + (has_right ? kPadding : 0), bounds.top, bounds.right, center_bottom};
        case WorkspaceDockArea::Bottom:
            return RECT{center_left, center_bottom + (has_bottom ? kPadding : 0), center_right, bounds.bottom};
        }
        return bounds;
    }

    RECT joint_rect(WorkspacePanelJoint joint, const RECT& bounds) const {
        const RECT left_rect = dock_area_rect(WorkspaceDockArea::Left, bounds);
        const RECT center_rect = dock_area_rect(WorkspaceDockArea::Center, bounds);
        const RECT right_rect = dock_area_rect(WorkspaceDockArea::Right, bounds);
        const RECT bottom_rect = dock_area_rect(WorkspaceDockArea::Bottom, bounds);
        const int joint_thickness = (std::max)(models.layout.joint_thickness, 4);

        switch (joint) {
        case WorkspacePanelJoint::LeftCenter: {
            const int x = left_rect.right;
            return RECT{x, bounds.top, x + joint_thickness, center_rect.bottom};
        }
        case WorkspacePanelJoint::CenterRight: {
            const int x = right_rect.left - joint_thickness;
            return RECT{x, bounds.top, x + joint_thickness, center_rect.bottom};
        }
        case WorkspacePanelJoint::CenterBottom: {
            const int y = bottom_rect.top - joint_thickness;
            return RECT{center_rect.left, y, center_rect.right, y + joint_thickness};
        }
        }
        return bounds;
    }

    WorkspaceDockArea dock_area_from_screen_point(POINT point) const {
        if (!window_handle) {
            return WorkspaceDockArea::Center;
        }

        RECT client_rect{};
        GetClientRect(window_handle, &client_rect);
        RECT content_bounds{client_rect.left + kPadding,
                            client_rect.top + kPadding,
                            client_rect.right - kPadding,
                            client_rect.bottom - (kStatusHeight + kPadding)};

        POINT local = point;
        ScreenToClient(window_handle, &local);
        for (WorkspaceDockArea area : {WorkspaceDockArea::Left, WorkspaceDockArea::Center, WorkspaceDockArea::Right, WorkspaceDockArea::Bottom}) {
            RECT area_rect = dock_area_rect(area, content_bounds);
            if (PtInRect(&area_rect, local)) {
                return area;
            }
        }
        return WorkspaceDockArea::Center;
    }

    int find_joint_index_by_handle(HWND hwnd) const {
        for (std::size_t index = 0; index < panel_joints.size(); ++index) {
            if (panel_joints[index].handle == hwnd) {
                return static_cast<int>(index);
            }
        }
        return -1;
    }

    void begin_panel_drag(HWND header) {
        active_drag_panel_index = find_panel_index_by_header(header);
        if (active_drag_panel_index < 0) {
            return;
        }

        active_drop_area = dock_panels[static_cast<std::size_t>(active_drag_panel_index)].area;
        SetCapture(header);

        std::string status_text = "Dragging ";
        status_text += dock_panels[static_cast<std::size_t>(active_drag_panel_index)].title;
        status_text += " - release over a dock zone to snap";
        SetWindowTextA(status, status_text.c_str());
    }

    void update_panel_drag(POINT point) {
        if (active_drag_panel_index < 0) {
            return;
        }

        active_drop_area = dock_area_from_screen_point(point);
        std::string status_text = "Snap ";
        status_text += dock_panels[static_cast<std::size_t>(active_drag_panel_index)].title;
        status_text += " to ";
        status_text += dock_area_name(active_drop_area);
        status_text += " zone";
        SetWindowTextA(status, status_text.c_str());
    }

    void finish_panel_drag(POINT point) {
        if (active_drag_panel_index < 0) {
            return;
        }

        dock_panels[static_cast<std::size_t>(active_drag_panel_index)].area = dock_area_from_screen_point(point);
        active_drag_panel_index = -1;
    }

    void begin_joint_drag(HWND joint_handle) {
        const int joint_index = find_joint_index_by_handle(joint_handle);
        if (joint_index < 0) {
            return;
        }

        active_joint = panel_joints[static_cast<std::size_t>(joint_index)].joint;
        dragging_joint = true;
        GetCursorPos(&joint_drag_start);
        joint_drag_origin_layout = models.layout;
        SetCapture(joint_handle);

        std::string status_text = "Dragging joint ";
        status_text += joint_name(active_joint);
        status_text += " - release to resize grouped panels";
        SetWindowTextA(status, status_text.c_str());
    }

    void update_joint_drag(POINT point) {
        if (!dragging_joint || !window_handle) {
            return;
        }

        RECT client_rect{};
        GetClientRect(window_handle, &client_rect);
        const int client_width = static_cast<int>(client_rect.right - client_rect.left);
        const int client_height = static_cast<int>(client_rect.bottom - client_rect.top);
        const int delta_x = static_cast<int>(point.x - joint_drag_start.x);
        const int delta_y = static_cast<int>(point.y - joint_drag_start.y);
        WorkspacePanelLayout next_layout = joint_drag_origin_layout;
        const int min_center_width = kMinimumPanelExtent;

        switch (active_joint) {
        case WorkspacePanelJoint::LeftCenter: {
            const int max_left_width = (std::max)(kMinimumPanelExtent,
                client_width - next_layout.right_width - min_center_width - (kPadding * 4));
            next_layout.left_width = (std::clamp)(joint_drag_origin_layout.left_width + delta_x,
                kMinimumPanelExtent,
                max_left_width);
            break;
        }
        case WorkspacePanelJoint::CenterRight: {
            const int max_right_width = (std::max)(kMinimumPanelExtent,
                client_width - next_layout.left_width - min_center_width - (kPadding * 4));
            next_layout.right_width = (std::clamp)(joint_drag_origin_layout.right_width - delta_x,
                kMinimumPanelExtent,
                max_right_width);
            break;
        }
        case WorkspacePanelJoint::CenterBottom: {
            const int max_bottom_height = (std::max)(kMinimumBottomHeight,
                client_height - kStatusHeight - kMinimumPanelExtent - (kPadding * 4));
            next_layout.bottom_height = (std::clamp)(joint_drag_origin_layout.bottom_height - delta_y,
                kMinimumBottomHeight,
                max_bottom_height);
            break;
        }
        }

        models.layout = next_layout;
        if (callbacks.on_layout_changed) {
            callbacks.on_layout_changed(models.layout);
        }
        apply_models_to_controls();
    }

    void finish_joint_drag() {
        if (!dragging_joint) {
            return;
        }
        dragging_joint = false;
    }

    void layout_child_controls(int client_width, int client_height) {
        if (!window_handle) {
            return;
        }

        RECT client_rect{};
        GetClientRect(window_handle, &client_rect);
        if (client_width <= 0) {
            client_width = static_cast<int>(client_rect.right - client_rect.left);
        }
        if (client_height <= 0) {
            client_height = static_cast<int>(client_rect.bottom - client_rect.top);
        }

        const RECT content_bounds{client_rect.left + kPadding,
                                  client_rect.top + kPadding,
                                  client_rect.right - kPadding,
                                  client_rect.bottom - (kStatusHeight + kPadding)};

        for (WorkspaceDockArea area : {WorkspaceDockArea::Left, WorkspaceDockArea::Center, WorkspaceDockArea::Right, WorkspaceDockArea::Bottom}) {
            RECT area_rect = dock_area_rect(area, content_bounds);
            std::vector<DockPanel*> panels_in_area;
            for (DockPanel& panel : dock_panels) {
                if (panel.area == area) {
                    panels_in_area.push_back(&panel);
                }
            }

            if (panels_in_area.empty()) {
                continue;
            }

            const int area_height = static_cast<int>(area_rect.bottom - area_rect.top);
            const int area_width = static_cast<int>(area_rect.right - area_rect.left);
            const int total_height = (std::max)(area_height, static_cast<int>(panels_in_area.size()) * 80);
            const int panel_height = (std::max)(total_height / static_cast<int>(panels_in_area.size()), 80);
            int next_top = area_rect.top;
            for (std::size_t index = 0; index < panels_in_area.size(); ++index) {
                DockPanel* panel = panels_in_area[index];
                int height = (index + 1 == panels_in_area.size())
                    ? static_cast<int>(area_rect.bottom - next_top)
                    : panel_height;
                height = (std::max)(height, 80);
                const int panel_width = (std::max)(area_width, 120);
                const int body_height = (std::max)(height - kPanelHeaderHeight, 40);

                MoveWindow(panel->container, area_rect.left, next_top, panel_width, height, TRUE);
                MoveWindow(panel->header, 0, 0, panel_width, kPanelHeaderHeight, TRUE);
                MoveWindow(panel->body, 0, kPanelHeaderHeight, panel_width, body_height, TRUE);

                if (panel->id == "workspace" && left_panel && nav_rail && left_title && file_tree) {
                    const int rail_width = 64;
                    const int title_height = 28;
                    const int inner_width = (std::max)(panel_width - (kPadding * 2), 80);
                    const int body_inner_height = (std::max)(body_height - (kPadding * 2), 60);
                    const int content_width = (std::max)(inner_width - rail_width - kPadding, 80);
                    MoveWindow(nav_rail,
                               kPadding,
                               kPadding,
                               rail_width,
                               body_inner_height,
                               TRUE);
                    MoveWindow(left_title,
                               kPadding + rail_width + kPadding,
                               kPadding,
                               content_width,
                               title_height,
                               TRUE);
                    MoveWindow(file_tree,
                               kPadding + rail_width + kPadding,
                               kPadding + title_height + 4,
                               content_width,
                               (std::max)(body_inner_height - title_height - 4, 40),
                               TRUE);
                }
                next_top += height + kPadding;
            }
        }

        for (PanelJointHandle& joint : panel_joints) {
            RECT splitter_rect = joint_rect(joint.joint, content_bounds);
            MoveWindow(joint.handle,
                       splitter_rect.left,
                       splitter_rect.top,
                       static_cast<int>(splitter_rect.right - splitter_rect.left),
                       static_cast<int>(splitter_rect.bottom - splitter_rect.top),
                       TRUE);
            ShowWindow(joint.handle, SW_SHOW);
        }

        MoveWindow(status,
                   kPadding,
                   client_height - kStatusHeight,
                   (std::max)(client_width - (kPadding * 2), 120),
                   kStatusHeight - 4,
                   TRUE);
    }

    void create_dock_panel(const std::string& id,
                           const std::string& title,
                           WorkspaceDockArea area,
                           HWND body) {
        DockPanel panel;
        panel.id = id;
        panel.title = title;
        panel.area = area;
        panel.body = body;
        panel.container = CreateWindowExA(WS_EX_CLIENTEDGE, "STATIC", "",
            WS_CHILD | WS_VISIBLE,
            0, 0, 0, 0,
            window_handle, nullptr, nullptr, nullptr);
        panel.header = CreateWindowExA(0, "BUTTON", title.c_str(),
            WS_CHILD | WS_VISIBLE | BS_FLAT,
            0, 0, 0, 0,
            panel.container, nullptr, nullptr, nullptr);

        if (ui_font) {
            SendMessage(panel.container, WM_SETFONT, reinterpret_cast<WPARAM>(ui_font), TRUE);
            SendMessage(panel.header, WM_SETFONT, reinterpret_cast<WPARAM>(ui_font), TRUE);
            SendMessage(panel.body, WM_SETFONT, reinterpret_cast<WPARAM>(ui_font), TRUE);
        }

        SetParent(panel.body, panel.container);
        SetPropA(panel.header, kPanelHeaderProperty, this);
        panel.original_header_proc = reinterpret_cast<WNDPROC>(
            SetWindowLongPtr(panel.header, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&Impl::panel_header_proc)));

        dock_panels.push_back(panel);
    }

    void create_joint_handle(WorkspacePanelJoint joint) {
        PanelJointHandle handle;
        handle.joint = joint;
        handle.handle = CreateWindowExA(0, "STATIC", "",
            WS_CHILD | WS_VISIBLE,
            0, 0, 0, 0,
            window_handle, nullptr, nullptr, nullptr);
        SetPropA(handle.handle, kPanelJointProperty, this);
        handle.original_proc = reinterpret_cast<WNDPROC>(
            SetWindowLongPtr(handle.handle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&Impl::panel_joint_proc)));
        panel_joints.push_back(handle);
    }

    void create_child_controls() {
        ui_font = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));

        left_panel = CreateWindowExA(0, "STATIC", "",
            WS_CHILD | WS_VISIBLE,
            0, 0, 0, 0,
            window_handle, nullptr, nullptr, nullptr);
        nav_rail = CreateWindowExA(WS_EX_CLIENTEDGE, "LISTBOX", "",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOINTEGRALHEIGHT,
            0, 0, 0, 0,
            left_panel, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kIdNavRail)), nullptr, nullptr);
        left_title = CreateWindowExA(0, "STATIC", "Workspace",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            0, 0, 0, 0,
            left_panel, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kIdLeftTitle)), nullptr, nullptr);
        file_tree = CreateWindowExA(WS_EX_CLIENTEDGE, "LISTBOX", "",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOINTEGRALHEIGHT,
            0, 0, 0, 0,
            left_panel, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kIdFileTree)), nullptr, nullptr);
        editor = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL,
            0, 0, 0, 0,
            window_handle, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kIdEditor)), nullptr, nullptr);
        inspector = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
            0, 0, 0, 0,
            window_handle, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kIdInspector)), nullptr, nullptr);
        preview = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
            0, 0, 0, 0,
            window_handle, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kIdPreview)), nullptr, nullptr);
        status = CreateWindowExA(0, "STATIC", "Ready",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            0, 0, 0, 0,
            window_handle, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kIdStatus)), nullptr, nullptr);

        for (HWND control : {left_panel, nav_rail, left_title, file_tree, editor, inspector, preview, status}) {
            SendMessage(control, WM_SETFONT, reinterpret_cast<WPARAM>(ui_font), TRUE);
        }

        dock_panels.clear();
        panel_joints.clear();
        create_dock_panel("workspace", "Workspace", WorkspaceDockArea::Left, left_panel);
        create_dock_panel("editor", "Editor", WorkspaceDockArea::Center, editor);
        create_dock_panel("inspector", "Inspector", WorkspaceDockArea::Right, inspector);
        create_dock_panel("preview", "Preview", WorkspaceDockArea::Bottom, preview);
        create_joint_handle(WorkspacePanelJoint::LeftCenter);
        create_joint_handle(WorkspacePanelJoint::CenterRight);
        create_joint_handle(WorkspacePanelJoint::CenterBottom);
    }

    void apply_models_to_controls() {
        if (!window_handle) {
            return;
        }

        suppress_change_notifications = true;
        SetWindowTextA(editor, models.editor_text.c_str());
        suppress_change_notifications = false;

        SendMessage(nav_rail, LB_RESETCONTENT, 0, 0);
        for (const WorkspaceNavItem& item : models.nav_items) {
            std::string label = item.icon;
            if (!label.empty()) {
                label += " ";
            }
            label += item.label;
            SendMessageA(nav_rail, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
        }
        SendMessage(nav_rail, LB_SETCURSEL, static_cast<WPARAM>((std::max)(models.selected_nav_index, 0)), 0);

        SendMessage(file_tree, LB_RESETCONTENT, 0, 0);
        for (const std::string& entry : models.left_panel_entries) {
            SendMessageA(file_tree, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(entry.c_str()));
        }
        if (models.selected_left_entry_index >= 0) {
            SendMessage(file_tree, LB_SETCURSEL, static_cast<WPARAM>(models.selected_left_entry_index), 0);
        }
        SetWindowTextA(left_title, models.left_panel_title.c_str());

        SetWindowTextA(inspector, models.inspector_text.c_str());
        SetWindowTextA(preview, models.preview_text.c_str());
        SetWindowTextA(status, models.status_text.c_str());
        window.set_title(models.title.empty() ? config.title : models.title);

        std::ostringstream preview_stream;
        if (!models.preview_text.empty()) {
            preview_stream << models.preview_text;
        }
        if (!dock_panels.empty()) {
            preview_stream << (models.preview_text.empty() ? "" : "\r\n\r\n") << "Panels\r\n------\r\n";
            for (const DockPanel& panel : dock_panels) {
                preview_stream << panel.title << " -> " << dock_area_name(panel.area) << "\r\n";
            }
            preview_stream << "\r\nLayout\r\n------\r\n"
                           << "Left width: " << models.layout.left_width << "\r\n"
                           << "Right width: " << models.layout.right_width << "\r\n"
                           << "Bottom height: " << models.layout.bottom_height;
            SetWindowTextA(preview, preview_stream.str().c_str());
        }

        RECT client_rect{};
        GetClientRect(window_handle, &client_rect);
        layout_child_controls(static_cast<int>(client_rect.right - client_rect.left),
                              static_cast<int>(client_rect.bottom - client_rect.top));
    }

    std::string read_editor_text() const {
        if (!editor) {
            return {};
        }

        const int length = GetWindowTextLengthA(editor);
        std::string text(static_cast<std::size_t>((std::max)(length, 0)) + 1U, '\0');
        if (length > 0) {
            GetWindowTextA(editor, text.data(), length + 1);
            text.resize(static_cast<std::size_t>(length));
        } else {
            text.clear();
        }
        return text;
    }

    LRESULT on_panel_header_message(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
        const int panel_index = find_panel_index_by_header(hwnd);
        WNDPROC original_proc = nullptr;
        if (panel_index >= 0) {
            original_proc = dock_panels[static_cast<std::size_t>(panel_index)].original_header_proc;
        }

        switch (message) {
        case WM_LBUTTONDOWN:
            begin_panel_drag(hwnd);
            return 0;
        case WM_MOUSEMOVE:
            if (GetCapture() == hwnd && active_drag_panel_index >= 0) {
                POINT point{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
                ClientToScreen(hwnd, &point);
                update_panel_drag(point);
                return 0;
            }
            break;
        case WM_LBUTTONUP:
            if (GetCapture() == hwnd && active_drag_panel_index >= 0) {
                POINT point{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
                ClientToScreen(hwnd, &point);
                ReleaseCapture();
                finish_panel_drag(point);
                apply_models_to_controls();
                return 0;
            }
            break;
        case WM_CAPTURECHANGED:
            if (active_drag_panel_index >= 0) {
                POINT point{};
                GetCursorPos(&point);
                finish_panel_drag(point);
                apply_models_to_controls();
            }
            break;
        case WM_SETCURSOR:
            SetCursor(LoadCursor(nullptr, IDC_SIZEALL));
            return TRUE;
        }

        if (original_proc) {
            return CallWindowProc(original_proc, hwnd, message, wparam, lparam);
        }
        return DefWindowProc(hwnd, message, wparam, lparam);
    }

    LRESULT on_panel_joint_message(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
        const int joint_index = find_joint_index_by_handle(hwnd);
        WNDPROC original_proc = nullptr;
        if (joint_index >= 0) {
            original_proc = panel_joints[static_cast<std::size_t>(joint_index)].original_proc;
        }

        switch (message) {
        case WM_LBUTTONDOWN:
            begin_joint_drag(hwnd);
            return 0;
        case WM_MOUSEMOVE:
            if (GetCapture() == hwnd && dragging_joint) {
                POINT point{};
                GetCursorPos(&point);
                update_joint_drag(point);
                return 0;
            }
            break;
        case WM_LBUTTONUP:
            if (GetCapture() == hwnd && dragging_joint) {
                ReleaseCapture();
                finish_joint_drag();
                apply_models_to_controls();
                return 0;
            }
            break;
        case WM_CAPTURECHANGED:
            if (dragging_joint) {
                finish_joint_drag();
                apply_models_to_controls();
            }
            break;
        case WM_SETCURSOR:
            if (joint_index >= 0) {
                const WorkspacePanelJoint joint = panel_joints[static_cast<std::size_t>(joint_index)].joint;
                const LPCSTR cursor_id = joint == WorkspacePanelJoint::CenterBottom ? IDC_SIZENS : IDC_SIZEWE;
                SetCursor(LoadCursor(nullptr, cursor_id));
                return TRUE;
            }
            break;
        }

        if (original_proc) {
            return CallWindowProc(original_proc, hwnd, message, wparam, lparam);
        }
        return DefWindowProc(hwnd, message, wparam, lparam);
    }

    LRESULT on_host_message(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
        switch (message) {
        case WM_SIZE:
            layout_child_controls(LOWORD(lparam), HIWORD(lparam));
            return 0;
        case WM_COMMAND: {
            const int control_id = LOWORD(wparam);
            const int notification = HIWORD(wparam);

            if (control_id == kIdEditor && notification == EN_CHANGE && !suppress_change_notifications) {
                if (callbacks.on_editor_text_changed) {
                    callbacks.on_editor_text_changed(read_editor_text());
                }
                return 0;
            }
            if (control_id == kIdNavRail && notification == LBN_SELCHANGE) {
                const int selection = static_cast<int>(SendMessage(nav_rail, LB_GETCURSEL, 0, 0));
                if (callbacks.on_nav_selected) {
                    callbacks.on_nav_selected(selection);
                }
                return 0;
            }
            if (control_id == kIdFileTree && notification == LBN_DBLCLK) {
                const int selection = static_cast<int>(SendMessage(file_tree, LB_GETCURSEL, 0, 0));
                if (callbacks.on_left_item_activated) {
                    callbacks.on_left_item_activated(selection);
                }
                return 0;
            }
            if (control_id == kIdMenuOpen && callbacks.on_open_file) {
                callbacks.on_open_file();
                return 0;
            }
            if (control_id == kIdMenuOpenWorkspace && callbacks.on_open_workspace) {
                callbacks.on_open_workspace();
                return 0;
            }
            if (control_id == kIdMenuSave && callbacks.on_save) {
                callbacks.on_save();
                return 0;
            }
            if (control_id == kIdMenuSaveAs && callbacks.on_save_as) {
                callbacks.on_save_as();
                return 0;
            }
            if (control_id == kIdMenuExit) {
                if (callbacks.on_exit) {
                    callbacks.on_exit();
                } else {
                    DestroyWindow(hwnd);
                }
                return 0;
            }
            break;
        }
        case WM_NCDESTROY:
            RemovePropA(hwnd, kHostWindowProperty);
            return 0;
        }

        if (original_window_proc) {
            return CallWindowProc(original_window_proc, hwnd, message, wparam, lparam);
        }
        return DefWindowProc(hwnd, message, wparam, lparam);
    }

    static LRESULT CALLBACK host_window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
        auto* impl = reinterpret_cast<Impl*>(GetPropA(hwnd, kHostWindowProperty));
        if (impl) {
            return impl->on_host_message(hwnd, message, wparam, lparam);
        }
        return DefWindowProc(hwnd, message, wparam, lparam);
    }

    static LRESULT CALLBACK panel_header_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
        auto* impl = reinterpret_cast<Impl*>(GetPropA(hwnd, kPanelHeaderProperty));
        if (impl) {
            return impl->on_panel_header_message(hwnd, message, wparam, lparam);
        }
        return DefWindowProc(hwnd, message, wparam, lparam);
    }

    static LRESULT CALLBACK panel_joint_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
        auto* impl = reinterpret_cast<Impl*>(GetPropA(hwnd, kPanelJointProperty));
        if (impl) {
            return impl->on_panel_joint_message(hwnd, message, wparam, lparam);
        }
        return DefWindowProc(hwnd, message, wparam, lparam);
    }

    bool create() {
        window.set_menu_bar(build_menu_model());
        window.set_platform_style(::graphics::windows::PlatformStyle::Windows);

        RenderHooks hooks;
        hooks.on_resize = [this](const RenderEvent& event) {
            if (window_handle) {
                layout_child_controls(static_cast<int>(event.width), static_cast<int>(event.height));
            }
        };
        hooks.on_render = [this](const RenderEvent&) {
            apply_models_to_controls();
        };
        window.set_render_hooks(std::move(hooks));

        if (!window.create()) {
            return false;
        }

        window_handle = reinterpret_cast<HWND>(window.native_handle());
        if (!window_handle) {
            return false;
        }

        menu_bar = build_native_menu_bar();
        SetMenu(window_handle, menu_bar);
        SetPropA(window_handle, kHostWindowProperty, this);
        original_window_proc = reinterpret_cast<WNDPROC>(
            SetWindowLongPtr(window_handle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&Impl::host_window_proc)));

        create_child_controls();
        apply_models_to_controls();
        return true;
    }

    void destroy_native() {
        for (DockPanel& panel : dock_panels) {
            if (panel.header && panel.original_header_proc) {
                SetWindowLongPtr(panel.header, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(panel.original_header_proc));
                panel.original_header_proc = nullptr;
            }
            if (panel.header) {
                RemovePropA(panel.header, kPanelHeaderProperty);
            }
        }

        for (PanelJointHandle& joint : panel_joints) {
            if (joint.handle && joint.original_proc) {
                SetWindowLongPtr(joint.handle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(joint.original_proc));
                joint.original_proc = nullptr;
            }
            if (joint.handle) {
                RemovePropA(joint.handle, kPanelJointProperty);
            }
        }

        if (window_handle && original_window_proc) {
            SetWindowLongPtr(window_handle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(original_window_proc));
            original_window_proc = nullptr;
        }

        if (window_handle) {
            RemovePropA(window_handle, kHostWindowProperty);
        }
        if (menu_bar) {
            DestroyMenu(menu_bar);
            menu_bar = nullptr;
        }
        window_handle = nullptr;
    }
#else
    explicit Impl(WindowConfig window_config)
        : config(std::move(window_config)),
          window(config) {}

    bool create() {
        window.set_menu_bar(build_menu_model());
        return window.create();
    }

    void destroy_native() {}
#endif
};

WorkspaceDockHost::WorkspaceDockHost(WindowConfig config)
    : impl_(new Impl(std::move(config))) {}

WorkspaceDockHost::~WorkspaceDockHost() {
    close();
    delete impl_;
}

void WorkspaceDockHost::set_callbacks(WorkspaceDockCallbacks callbacks) {
    impl_->callbacks = std::move(callbacks);
}

bool WorkspaceDockHost::create() {
    return impl_->create();
}

void WorkspaceDockHost::show() {
    impl_->window.show();
}

void WorkspaceDockHost::close() {
    if (!impl_) {
        return;
    }
    impl_->destroy_native();
    impl_->window.close();
}

bool WorkspaceDockHost::pump_events() {
    return impl_->window.pump_events();
}

void WorkspaceDockHost::request_redraw() {
    impl_->window.request_redraw();
}

void WorkspaceDockHost::set_models(const WorkspaceDockModels& models) {
    impl_->models = models;
#if defined(_WIN32)
    impl_->apply_models_to_controls();
#endif
}

std::string WorkspaceDockHost::backend_name() const {
    return impl_->window.backend_name();
}

void WorkspaceDockHost::set_platform_style(::graphics::windows::PlatformStyle platform_style) {
    impl_->window.set_platform_style(platform_style);
}

bool WorkspaceDockHost::prompt_open_file(std::string& path) {
#if defined(_WIN32)
    char file_name[MAX_PATH] = {0};
    OPENFILENAMEA dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = impl_->window_handle;
    dialog.lpstrFile = file_name;
    dialog.nMaxFile = MAX_PATH;
    dialog.lpstrFilter = "Text Files\0*.txt;*.md;*.cpp;*.hpp;*.h\0All Files\0*.*\0";
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_EXPLORER;
    if (!GetOpenFileNameA(&dialog)) {
        return false;
    }
    path = file_name;
    return true;
#else
    (void)path;
    return false;
#endif
}

bool WorkspaceDockHost::prompt_open_workspace(std::string& path) {
#if defined(_WIN32)
    HRESULT init_result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    const bool should_uninitialize = SUCCEEDED(init_result) || init_result == S_FALSE;

    IFileOpenDialog* dialog = nullptr;
    HRESULT result = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER,
                                      IID_PPV_ARGS(&dialog));
    if (FAILED(result) || dialog == nullptr) {
        if (should_uninitialize) {
            CoUninitialize();
        }
        return false;
    }

    DWORD options = 0;
    dialog->GetOptions(&options);
    dialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
    dialog->SetTitle(L"Open Workspace");

    result = dialog->Show(impl_->window_handle);
    if (FAILED(result)) {
        dialog->Release();
        if (should_uninitialize) {
            CoUninitialize();
        }
        return false;
    }

    IShellItem* item = nullptr;
    result = dialog->GetResult(&item);
    if (SUCCEEDED(result) && item != nullptr) {
        PWSTR file_path = nullptr;
        result = item->GetDisplayName(SIGDN_FILESYSPATH, &file_path);
        if (SUCCEEDED(result) && file_path != nullptr) {
            path = wide_to_utf8(file_path);
            CoTaskMemFree(file_path);
        }
        item->Release();
    }

    dialog->Release();
    if (should_uninitialize) {
        CoUninitialize();
    }
    return !path.empty();
#else
    (void)path;
    return false;
#endif
}

bool WorkspaceDockHost::prompt_save_file(std::string& path) {
#if defined(_WIN32)
    char file_name[MAX_PATH] = {0};
    if (!path.empty()) {
        std::snprintf(file_name, MAX_PATH, "%s", path.c_str());
    }

    OPENFILENAMEA dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = impl_->window_handle;
    dialog.lpstrFile = file_name;
    dialog.nMaxFile = MAX_PATH;
    dialog.lpstrFilter = "Text Files\0*.txt\0Markdown\0*.md\0All Files\0*.*\0";
    dialog.Flags = OFN_OVERWRITEPROMPT | OFN_EXPLORER;
    dialog.lpstrDefExt = "txt";
    if (!GetSaveFileNameA(&dialog)) {
        return false;
    }
    path = file_name;
    return true;
#else
    (void)path;
    return false;
#endif
}

} // namespace full_application_window
} // namespace graphics