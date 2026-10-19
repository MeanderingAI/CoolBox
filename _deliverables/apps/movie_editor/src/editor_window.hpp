#ifndef COOLBOX_APPS_MOVIE_EDITOR_EDITOR_WINDOW_HPP
#define COOLBOX_APPS_MOVIE_EDITOR_EDITOR_WINDOW_HPP

#include "full_application_window.hpp"
#include "native_menu.hpp"
#include "movie_editor_core.h"
#include "fonts.hpp"
#include "audio_playback.hpp"
#include "timeline_gesture.hpp"

#include <chrono>
#include <array>
#include <cstdint>
#include <optional>
#include <future>
#include <string>
#include <vector>

namespace movie_editor_app {

// Mouse-driven, single-window movie editor UI: a media bin, a scrubbable
// preview, a multi-track timeline, a clip inspector, and transport controls.
// Built entirely from Canvas draw primitives (no retained-mode widgets),
// matching the hand-drawn-UI style used by the other CoolBox GUI apps.
//
class MovieEditorWindow {
public:
    explicit MovieEditorWindow(trekker::movie_editor::EditorProject project = {},
                               std::string project_path = {});

    // Creates the OS window and runs the event loop until closed. Returns
    // true on a clean exit.
    bool run();

private:
    friend struct MovieEditorWindowTestAccess;
    struct Layout {
        int width = 0, height = 0;
        int topbar_y1 = 0;
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

    enum class TransportButton { GoToStart, PlayPause, Stop, GoToEnd, Split, Delete, AddVideoTrack, AddAudioTrack,
                                 ImportMedia, AddMedia, ZoomIn, ZoomOut, Fit, Undo, Redo, ExportMp4,
                                 AddText };

    struct ButtonRect { TransportButton id; int x0, y0, x1, y1; const char* label; };

    // Shared definitions drive native menus and the non-native fallback.
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
    void handle_context_click(int x, int y);
    void handle_pointer(const graphics::full_application_window::PointerPressEvent& event, bool release);
    void delete_selected_track();
    void open_text_editor(bool background, bool edit = false);
    void apply_text_editor();
    trekker::movie_editor::TextStyle draft_text_style() const;
    void text_editor_pointer(int x, int y, bool release, bool press);
    void handle_text_input(const std::string& text);
    void text_editor_key(const graphics::full_application_window::KeyEvent& event);
    void handle_menu_command(std::size_t menu_index, std::size_t item_index);
    void configure_window();
    void process_pointer();
    int text_width(const std::string& text, int scale) const;
    void add_selected_media();
    void do_import_media();
    void finish_import();
    void finish_export();
    void do_export_mp4();
    void pause_playback();
    void toggle_playback();
    void handle_key(const graphics::full_application_window::KeyEvent& event);
    void update_drag(int x, int y, bool release);
    void undo_redo(bool redo);
    void report_error(const std::string& message);

    std::int64_t x_to_time_us(const Layout& layout, int x) const;
    int time_to_x(const Layout& layout, std::int64_t time_us) const;

    void do_import_image_sequence();
    void do_import_video_file();
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
    std::unique_ptr<graphics::os_generic::NativeMenuBar> menu_bar_;
    graphics::fonts::FontFace ui_font_;
    std::future<trekker::movie_editor::MediaAsset> video_import_;
    std::future<void> export_job_;
    AudioPlayback audio_playback_;
    struct DragState {
        DragKind kind;
        trekker::timeline::Clip original;
        trekker::timeline::Clip preview;
        std::uint64_t from_track, to_track;
        int start_x, start_y;
        bool moved = false;
    };
    std::optional<DragState> drag_;
    struct MediaDrag {
        std::string key;
        int start_x, start_y, x, y;
        bool moved = false;
    };
    std::optional<MediaDrag> media_drag_;
    struct TrackContextMenu { std::uint64_t track_id; int x, y; };
    std::optional<TrackContextMenu> track_context_menu_;
    struct TextEditor {
        bool background = true;
        double opacity = 1.0;
        bool edit = false;
        std::uint64_t track_id = 0, clip_id = 0;
        std::array<std::string, 10> fields;
        std::size_t active = 0;
        bool replace = true;
        std::string error;
        int drag = 0;
        int offset_x = 0, offset_y = 0;
        std::size_t picker_field = 3;
        graphics::components::ColorPicker picker;
        std::int64_t preview_us = 0;
    };
    std::optional<TextEditor> text_editor_;
    int media_scroll_ = 0;
    int track_scroll_ = 0;

    // Playback/selection state.
    std::int64_t playhead_us_ = 0;
    bool playing_ = false;
    std::int64_t visible_duration_us_ = 30'000'000;
    std::int64_t view_start_us_ = 0;

    std::optional<std::string> pending_media_key_;
    std::optional<std::size_t> open_menu_; // which topbar dropdown is currently expanded, if any
    bool native_menus_ = false;
    std::optional<std::uint64_t> selected_track_id_;
    std::optional<std::uint64_t> selected_clip_id_;

    std::string status_message_ = "Ready.";
    std::string last_project_path_;

    static constexpr std::size_t kPreviewWidth = 320;
    static constexpr std::size_t kPreviewHeight = 180;
};

} // namespace movie_editor_app

#endif // COOLBOX_APPS_MOVIE_EDITOR_EDITOR_WINDOW_HPP
