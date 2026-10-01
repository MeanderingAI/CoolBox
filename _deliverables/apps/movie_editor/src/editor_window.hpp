#ifndef COOLBOX_APPS_MOVIE_EDITOR_EDITOR_WINDOW_HPP
#define COOLBOX_APPS_MOVIE_EDITOR_EDITOR_WINDOW_HPP

#include "full_application_window.hpp"
#include "movie_editor_core.h"

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace movie_editor_app {

// Mouse-driven, single-window movie editor UI: a media bin, a scrubbable
// preview, a multi-track timeline, a clip inspector, and transport controls.
// Built entirely from Canvas draw primitives (no retained-mode widgets),
// matching the hand-drawn-UI style used by the other CoolBox GUI apps.
//
// There is no cross-platform keyboard hook exposed by full_application_window,
// so every action here is reachable via the mouse (menu bar + on-canvas
// buttons + direct clicks on the timeline/media bin).
class MovieEditorWindow {
public:
    explicit MovieEditorWindow(trekker::movie_editor::EditorProject project = {});

    // Creates the OS window and runs the event loop until closed. Returns
    // true on a clean exit.
    bool run();

private:
    struct Layout {
        int width = 0, height = 0;
        int topbar_y1 = 0; // bottom of the File/Edit/Help topbar strip
        int media_x0 = 0, media_x1 = 0, media_y0 = 0, media_y1 = 0;
        int preview_x0 = 0, preview_x1 = 0, preview_y0 = 0, preview_y1 = 0;
        int transport_y0 = 0, transport_y1 = 0;
        int inspector_x0 = 0, inspector_x1 = 0, inspector_y0 = 0, inspector_y1 = 0;
        int ruler_y0 = 0, ruler_y1 = 0;
        int tracks_y0 = 0, tracks_y1 = 0;
        int track_header_w = 130;
        int lane_h = 56;
        int status_y0 = 0;
    };

    enum class TransportButton { GoToStart, PlayPause, Stop, GoToEnd, Split, Delete, AddVideoTrack, AddAudioTrack };

    struct ButtonRect { TransportButton id; int x0, y0, x1, y1; const char* label; };

    // Shared menu data: drives both the native Win32/Cocoa menu bar (via
    // build_menu()) and the canvas-drawn topbar below (the native menu bar
    // is a no-op on the X11 backend, so the on-canvas topbar is what
    // actually makes these actions reachable there).
    struct MenuItemDef { std::string label; bool divider = false; };
    struct MenuDef { std::string title; std::vector<MenuItemDef> items; };
    std::vector<MenuDef> menu_definitions() const;

    struct TopBarButtonRect { std::size_t menu_index; int x0, y0, x1, y1; };
    struct DropdownItemRect { std::size_t item_index; int x0, y0, x1, y1; };

    Layout compute_layout(int width, int height) const;
    std::vector<ButtonRect> transport_buttons(const Layout& layout) const;
    std::vector<TopBarButtonRect> topbar_buttons(const Layout& layout) const;
    std::vector<DropdownItemRect> dropdown_item_rects(const Layout& layout, std::size_t menu_index) const;

    void render_scene();
    void handle_click(int x, int y);
    void handle_menu_command(std::size_t menu_index, std::size_t item_index);
    void build_menu();

    std::int64_t x_to_time_us(const Layout& layout, int x) const;
    int time_to_x(const Layout& layout, std::int64_t time_us) const;

    void do_import_image_sequence();
    void do_import_still_image();
    void do_import_audio();
    void do_save_project();
    void do_open_project();
    void do_export_avi();
    void do_export_gif();
    void do_export_wav();
    void do_show_version();

    void set_status(const std::string& message);

    trekker::movie_editor::EditorProject project_;
    graphics::full_application_window::FullApplicationWindow window_;

    // Playback/selection state.
    std::int64_t playhead_us_ = 0;
    bool playing_ = false;
    std::chrono::steady_clock::time_point last_tick_;
    std::int64_t visible_duration_us_ = 30'000'000; // fixed 30s-wide timeline view

    std::optional<std::string> pending_media_key_;
    std::optional<std::size_t> open_menu_; // which topbar dropdown is currently expanded, if any
    std::optional<std::uint64_t> selected_track_id_;
    std::optional<std::uint64_t> selected_clip_id_;

    std::string status_message_ = "Ready.";
    std::string last_project_path_;

    static constexpr std::size_t kPreviewWidth = 320;
    static constexpr std::size_t kPreviewHeight = 180;
};

} // namespace movie_editor_app

#endif // COOLBOX_APPS_MOVIE_EDITOR_EDITOR_WINDOW_HPP
