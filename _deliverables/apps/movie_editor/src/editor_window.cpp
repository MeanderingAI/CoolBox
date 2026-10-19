#include "editor_window.hpp"
#include "version.hpp"

#include "graphics.h"
#include "os_dialog.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
#include <iomanip>
#include <sstream>
#include <thread>
#include <cstdlib>
#include <cctype>

namespace movie_editor_app {

using namespace trekker; // resolves timeline::/video::/audio:: nested namespaces
using graphics::Color;
namespace Colors = graphics::Colors;

namespace {
namespace fs = std::filesystem;
constexpr Color panel_color{28, 31, 38, 255};
constexpr Color toolbar_color{39, 43, 52, 255};
constexpr Color button_color{53, 60, 72, 255};
using graphics::full_application_window::FullApplicationWindow;
using graphics::full_application_window::PointerState;
using graphics::full_application_window::RenderEvent;
using graphics::full_application_window::RenderHooks;
using graphics::full_application_window::WindowConfig;

float font_size(int scale) {
    return scale == 3 ? 26.0f : scale == 2 ? 21.0f : 18.0f;
}

class EditorCanvas : public graphics::Canvas {
public:
    EditorCanvas(int width, int height, const graphics::fonts::FontFace& font)
        : Canvas(width, height, Color{23, 26, 32, 255}), font_(font) {}

    void draw_text(int x, int y, const std::string& text, Color color, int scale = 1, int max_width = -1) {
        std::string label = text;
        const int available = max_width < 0 ? width() - x - 6 : max_width;
        const auto measure = [&](const std::string& value) {
            return font_.is_loaded()
                ? graphics::fonts::TextRenderer::measure_text(font_, value, font_size(scale)).width
                : static_cast<int>(value.size()) * 9 * std::max(2, scale);
        };
        if (measure(label) > available) {
            while (!label.empty() && measure(label + "...") > available) {
                std::size_t last = label.size() - 1;
                while (last > 0 && (static_cast<unsigned char>(label[last]) & 0xc0) == 0x80) --last;
                label.erase(last);
            }
            label += "...";
        }
        if (font_.is_loaded()) {
            graphics::fonts::TextRenderer::draw_text(*this, font_, x, y, label, color, font_size(scale));
        } else {
            Canvas::draw_text(x, y, label, color, std::max(2, scale));
        }
    }

private:
    const graphics::fonts::FontFace& font_;
};

std::string format_timecode(std::int64_t us) {
    if (us < 0) us = 0;
    const std::int64_t total_ms = us / 1000;
    const std::int64_t ms = total_ms % 1000;
    const std::int64_t total_s = total_ms / 1000;
    const std::int64_t s = total_s % 60;
    const std::int64_t m = total_s / 60;
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%02lld:%02lld.%03lld", static_cast<long long>(m),
                  static_cast<long long>(s), static_cast<long long>(ms));
    return buf;
}

std::string pick_path(app_builder::os_generics::DialogAction action, const std::string& title,
                      const std::vector<app_builder::os_generics::DialogFilter>& filters,
                      const std::string& suggested_name = {}) {
    app_builder::os_generics::OsDialog dialog;
    app_builder::os_generics::DialogRequest request;
    request.action = action;
    request.title = title;
    request.initial_path = fs::current_path().string();
    request.allow_native_ui = true;
    request.filters = filters;
    request.suggested_name = suggested_name;
    const auto result = dialog.show(request);
    if (!result.accepted || result.selected_paths.empty()) return {};
    return result.selected_paths.front();
}

} // namespace

namespace {
WindowConfig make_window_config() {
    WindowConfig cfg("CoolBox Movie Editor", 1280, 800);
    cfg.min_width = 960;
    cfg.min_height = 640;
    return cfg;
}
} // namespace

MovieEditorWindow::MovieEditorWindow(trekker::movie_editor::EditorProject project, std::string project_path)
    : project_(std::move(project)),
      window_(make_window_config()),
      last_project_path_(std::move(project_path)) {
    configure_window();
    std::vector<std::string> candidates;
    if (const char* font = std::getenv("COOLBOX_UI_FONT")) candidates.emplace_back(font);
#ifdef _WIN32
    if (const char* windir = std::getenv("WINDIR")) {
        candidates.push_back((fs::path(windir) / "Fonts" / "segoeui.ttf").string());
    }
#elif defined(__APPLE__)
    candidates.emplace_back("/System/Library/Fonts/Supplemental/Arial.ttf");
#else
    candidates.emplace_back("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
#endif
    for (const auto& candidate : candidates) {
        if (ui_font_.load_from_file(candidate)) return;
    }
    std::cerr << "Movie editor: no TrueType UI font found; set COOLBOX_UI_FONT to a .ttf file.\n";
    set_status("UI font unavailable. Set COOLBOX_UI_FONT to a TrueType font.");
}

int MovieEditorWindow::text_width(const std::string& text, int scale) const {
    if (ui_font_.is_loaded()) {
        return graphics::fonts::TextRenderer::measure_text(ui_font_, text, font_size(scale)).width;
    }
    return static_cast<int>(text.size()) * 18 * std::max(1, scale - 1);
}

MovieEditorWindow::Layout MovieEditorWindow::compute_layout(int width, int height) const {
    Layout l;
    l.width = width;
    l.height = height;

    l.topbar_y1 = 48;
    const int top_h = static_cast<int>(height * 0.55);
    l.media_x0 = 0;
    l.media_x1 = std::clamp(width / 5, 220, 300);
    l.media_y0 = l.topbar_y1;
    l.media_y1 = top_h;

    l.inspector_x1 = width;
    l.inspector_x0 = width - std::clamp(width / 5, 220, 280);
    l.inspector_y0 = l.topbar_y1;
    l.inspector_y1 = top_h;

    l.preview_x0 = l.media_x1;
    l.preview_x1 = l.inspector_x0;
    l.preview_y0 = l.topbar_y1;
    l.transport_y0 = top_h - 72; // two rows of buttons
    l.transport_y1 = top_h;
    l.preview_y1 = l.transport_y0;

    l.ruler_y0 = top_h;
    l.ruler_y1 = top_h + 36;
    l.tracks_y0 = l.ruler_y1;
    l.status_y0 = height - 36;
    l.tracks_y1 = l.status_y0;

    return l;
}

std::vector<MovieEditorWindow::TopBarButtonRect> MovieEditorWindow::topbar_buttons(const Layout& l) const {
    std::vector<TopBarButtonRect> buttons;
    if (native_menus_) return buttons;
    int x = 8;
    const auto defs = menu_definitions();
    for (std::size_t i = 0; i < defs.size(); ++i) {
        const int w = text_width(defs[i].title, 2) + 20;
        buttons.push_back({i, x, 4, x + w, l.topbar_y1 - 4});
        x += w + 4;
    }
    return buttons;
}

std::vector<MovieEditorWindow::DropdownItemRect> MovieEditorWindow::dropdown_item_rects(const Layout& l, std::size_t menu_index) const {
    std::vector<DropdownItemRect> rects;
    const auto defs = menu_definitions();
    if (menu_index >= defs.size()) return rects;

    const auto top_buttons = topbar_buttons(l);
    if (menu_index >= top_buttons.size()) return rects;
    const auto& button = top_buttons[menu_index];
    const int x0 = button.x0;

    const auto& items = defs[menu_index].items;
    int max_text_w = 0;
    for (const auto& item : items) max_text_w = std::max(max_text_w, text_width(item.label, 2));
    const int panel_w = std::max(max_text_w + 24, button.x1 - button.x0);
    const int x1 = x0 + panel_w;

    constexpr int kRowH = 36;
    int y = l.topbar_y1;
    for (std::size_t i = 0; i < items.size(); ++i) {
        rects.push_back({i, x0, y, x1, y + kRowH});
        y += kRowH;
    }
    return rects;
}

std::vector<MovieEditorWindow::ButtonRect> MovieEditorWindow::transport_buttons(const Layout& l) const {
    const struct { TransportButton id; const char* label; int row; int col; } defs[] = {
        {TransportButton::GoToStart,    "|<",         0, 0},
        {TransportButton::PlayPause,    playing_ ? "Pause" : "Play", 0, 1},
        {TransportButton::Stop,         "Stop",       0, 2},
        {TransportButton::GoToEnd,      ">|",         0, 3},
        {TransportButton::Split,        "Split",      1, 0},
        {TransportButton::Delete,       "Delete",     1, 1},
        {TransportButton::AddVideoTrack, "+Video Trk", 1, 2},
        {TransportButton::AddAudioTrack, "+Audio Trk", 1, 3},
    };
    std::vector<ButtonRect> buttons;
    constexpr int kCols = 4;
    const int button_w = (l.preview_x1 - l.preview_x0) / kCols;
    const int row_h = (l.transport_y1 - l.transport_y0) / 2;
    for (const auto& d : defs) {
        const int x = l.preview_x0 + d.col * button_w;
        const int y = l.transport_y0 + d.row * row_h;
        buttons.push_back({d.id, x + 3, y + 3, x + button_w - 3, y + row_h - 3, d.label});
    }
    const auto menus = topbar_buttons(l);
    int x = menus.empty() ? 8 : menus.back().x1 + 20;
    for (const auto& action : std::vector<std::pair<TransportButton, const char*>>{
             {TransportButton::ImportMedia, "Import Media"}, {TransportButton::AddMedia, "Add Clip"},
             {TransportButton::ZoomOut, "-"}, {TransportButton::ZoomIn, "+"}, {TransportButton::Fit, "Fit"},
             {TransportButton::Undo, "Undo"}, {TransportButton::Redo, "Redo"}, {TransportButton::ExportMp4, "Export"},
             {TransportButton::AddText, "Text / Title"}}) {
        const int width = text_width(action.second, 1) + 20;
        buttons.push_back({action.first, x, 7, x + width, 41, action.second});
        x += width + 8;
    }
    return buttons;
}

std::int64_t MovieEditorWindow::x_to_time_us(const Layout& l, int x) const {
    const int usable_w = std::max(1, (l.width - l.track_header_w));
    const double frac = std::clamp(static_cast<double>(x - l.track_header_w) / usable_w, 0.0, 1.0);
    return view_start_us_ + static_cast<std::int64_t>(frac * visible_duration_us_);
}

int MovieEditorWindow::time_to_x(const Layout& l, std::int64_t time_us) const {
    const int usable_w = std::max(1, (l.width - l.track_header_w));
    const double frac = std::clamp(static_cast<double>(time_us - view_start_us_) /
                                   static_cast<double>(visible_duration_us_), 0.0, 1.0);
    return l.track_header_w + static_cast<int>(frac * usable_w);
}

void MovieEditorWindow::set_status(const std::string& message) {
    status_message_ = message;
}

std::vector<MovieEditorWindow::MenuDef> MovieEditorWindow::menu_definitions() const {
    return {
        {"File", {
            {"Open Project..."},
            {"Save Project As..."},
            {"", true}, // divider
            {"Import Image Sequence..."},
            {"Import Video (MP4/MOV/AVI/GIF)..."},
            {"Import Still Image..."},
            {"Import Audio (WAV)..."},
            {"", true}, // divider
            {"Export Video (AVI)..."},
            {"Export Video Preview (GIF)..."},
            {"Export Audio (WAV)..."},
            {"", true}, // divider
            {"Exit"},
            {"Export MP4 (H.264/AAC)..."},
        }},
        {"Edit", {
            {"Add Video Track"},
            {"Add Audio Track"},
            {"", true}, // divider
            {"Split Selected Track At Playhead"},
            {"Delete Selected Clip"},
            {"", true},
            {"Undo"},
            {"Redo"},
            {"Delete Selected Track"},
            {"Add Text / Title..."},
            {"Edit Selected Text..."},
        }},
        {"Help", {
            {"Version / About..."},
            {"Keyboard Shortcuts"},
        }},
    };
}

void MovieEditorWindow::handle_menu_command(std::size_t menu_index, std::size_t item_index) {
    if (text_editor_) return;
    if (video_import_.valid() || export_job_.valid()) {
        set_status("Media operation in progress. Please wait.");
        return;
    }
    drag_.reset();
    media_drag_.reset();
    track_context_menu_.reset();
    pause_playback();
    if (menu_index == 0) { // File
        switch (item_index) {
            case 0: do_open_project(); return;
            case 1: do_save_project(); return;
            case 2: return; // divider
            case 3: do_import_image_sequence(); return;
            case 4: do_import_video_file(); return;
            case 5: do_import_still_image(); return;
            case 6: do_import_audio(); return;
            case 7: return; // divider
            case 8: do_export_avi(); return;
            case 9: do_export_gif(); return;
            case 10: do_export_wav(); return;
            case 11: return; // divider
            case 12: window_.close(); return;
            case 13: do_export_mp4(); return;
            default: return;
        }
    } else if (menu_index == 1) { // Edit
        switch (item_index) {
            case 0: project_.add_video_track("Video"); set_status("Added video track."); return;
            case 1: project_.add_audio_track("Audio"); set_status("Added audio track."); return;
            case 2: return; // divider
            case 3:
                if (selected_track_id_) {
                    if (project_.split_clip(*selected_track_id_, playhead_us_)) {
                        selected_clip_id_.reset();
                        set_status("Split at " + format_timecode(playhead_us_));
                    } else set_status("Place the playhead inside an unlocked clip to split it.");
                } else {
                    set_status("Select a clip/track first.");
                }
                return;
            case 8: delete_selected_track(); return;
            case 9: open_text_editor(false); return;
            case 10: open_text_editor(false, true); return;
            case 4:
                if (selected_track_id_ && selected_clip_id_) {
                    if (project_.delete_clip(*selected_track_id_, *selected_clip_id_)) {
                        selected_clip_id_.reset();
                        set_status("Deleted clip.");
                    } else set_status("The clip cannot be deleted (missing or locked).");
                } else {
                    set_status("Select a clip first.");
                }
                return;
            case 6: undo_redo(false); return;
            case 7: undo_redo(true); return;
            default: return;
        }
    } else if (menu_index == 2) { // Help
        switch (item_index) {
            case 0: do_show_version(); return;
            case 1:
                window_.show_info_dialog("Editing shortcuts",
                    "Space: Play/Pause\nLeft/Right: Step one frame\nHome/End: Start/End\n"
                    "Ctrl/Cmd+I: Import\nEnter: Add selected media\nCtrl/Cmd+B: Split\n"
                    "Delete/Backspace: Delete selected clip\nCtrl/Cmd+Z: Undo\n"
                    "Ctrl/Cmd+Shift+Z or Ctrl+Y: Redo\nCtrl/Cmd+S: Save\n"
                    "Ctrl/Cmd+O: Open\nCtrl/Cmd+E: Export MP4\nEsc: Cancel drag/menu\n"
                    "Drag clip body: Move (between matching tracks)\nDrag clip edges: Trim\n"
                    "Timeline wheel: Vertical scroll; Shift+wheel: Pan time\nFit: Show the full timeline");
                return;
            default: return;
        }
    }
}

void MovieEditorWindow::do_import_image_sequence() {
    const std::string dir = pick_path(app_builder::os_generics::DialogAction::PickFolder,
                                      "Select Image Sequence Folder", {});
    if (dir.empty()) return;
    try {
        const std::string key = project_.media_bin().import_image_sequence(dir, 24.0);
        pending_media_key_ = key;
        set_status("Imported image sequence: " + key);
    } catch (const std::exception& e) {
        set_status(std::string("Import failed: ") + e.what());
    }
}

void MovieEditorWindow::do_import_video_file() {
    const std::string path = pick_path(app_builder::os_generics::DialogAction::OpenFile, "Import Video",
        {{"Video Files", {"*.mp4", "*.mov", "*.m4v", "*.mkv", "*.webm", "*.avi", "*.gif"}}, {"All Files", {"*.*"}}});
    if (path.empty()) return;
    set_status("Importing video with FFmpeg... Please wait.");
    video_import_ = std::async(std::launch::async, [path] { return trekker::movie_editor::decode_ffmpeg_video(path); });
}

void MovieEditorWindow::finish_import() {
    if (!video_import_.valid() ||
        video_import_.wait_for(std::chrono::seconds(0)) != std::future_status::ready) return;
    try {
        pending_media_key_ = project_.media_bin().add_asset(video_import_.get());
        int width = 0, height = 0;
        window_.client_size(width, height);
        const auto layout = compute_layout(width, height);
        const int visible = std::max(1, (layout.media_y1 - layout.media_y0 - 68) / 48);
        media_scroll_ = std::max(0, static_cast<int>(project_.media_bin().assets().size()) - visible);
        set_status("Video imported. Click Add Clip to start editing.");
    } catch (const std::exception& e) {
        set_status(std::string("Import failed: ") + e.what());
        window_.show_info_dialog("Video import failed", e.what());
    }
}

void MovieEditorWindow::report_error(const std::string& message) {
    set_status(message);
    std::cerr << "Movie editor: " << message << '\n';
    window_.show_info_dialog("Movie editor", message);
}

void MovieEditorWindow::pause_playback() {
    if (playing_) playhead_us_ = audio_playback_.position_us();
    audio_playback_.stop();
    playing_ = false;
}

void MovieEditorWindow::toggle_playback() {
    if (playing_) { pause_playback(); return; }
    if (project_.duration_us() <= 0) { set_status("Add media to the timeline first."); return; }
    if (playhead_us_ >= project_.duration_us()) playhead_us_ = 0;
    audio_playback_.start(project_, playhead_us_);
    playing_ = true;
    set_status("Playing with synchronized audio. Space to pause.");
}

void MovieEditorWindow::undo_redo(bool redo) {
    pause_playback();
    drag_.reset();
    media_drag_.reset();
    track_context_menu_.reset();
    const bool changed = redo ? project_.redo() : project_.undo();
    selected_clip_id_.reset();
    selected_track_id_.reset();
    track_scroll_ = 0;
    playhead_us_ = std::min(playhead_us_, project_.duration_us());
    view_start_us_ = std::min(view_start_us_, playhead_us_);
    set_status(changed ? (redo ? "Redone." : "Undone.") : "No further history.");
}

void MovieEditorWindow::do_export_mp4() {
    pause_playback();
    if (project_.duration_us() <= 0) { set_status("The timeline is empty."); return; }
    const auto path = pick_path(app_builder::os_generics::DialogAction::SaveFile, "Export MP4",
                                {{"MP4 Video", {"*.mp4"}}}, "movie.mp4");
    if (path.empty()) return;
    set_status("Exporting MP4 (720p, 24 fps, H.264/AAC)... Please wait.");
    export_job_ = std::async(std::launch::async, [project = project_, path] {
        project.export_mp4(path);
    });
}

void MovieEditorWindow::finish_export() {
    if (!export_job_.valid() || export_job_.wait_for(std::chrono::seconds(0)) != std::future_status::ready) return;
    try {
        export_job_.get();
        set_status("MP4 export complete.");
    } catch (const std::exception& error) { report_error(std::string("Export failed: ") + error.what()); }
}

void MovieEditorWindow::open_text_editor(bool background, bool edit) {
    pause_playback();
    drag_.reset();
    media_drag_.reset();
    track_context_menu_.reset();
    trekker::movie_editor::TextStyle style;
    style.background = background;
    std::int64_t duration = 5'000'000;
    TextEditor editor;
    editor.preview_us = playhead_us_;
    editor.edit = edit;
    if (edit) {
        if (!selected_track_id_ || !selected_clip_id_) {
            set_status("Select a title or text overlay first.");
            return;
        }
        const auto* existing = project_.text_style(*selected_track_id_, *selected_clip_id_);
        const auto* track = project_.timeline().track(*selected_track_id_);
        if (!existing || !track) { set_status("Select a title or text overlay first."); return; }
        if (track->locked()) { set_status("Unlock the track before editing text."); return; }
        style = *existing;
        editor.track_id = *selected_track_id_;
        editor.clip_id = *selected_clip_id_;
        for (const auto& clip : track->clips())
            if (clip.id == editor.clip_id) {
                duration = clip.timeline_duration_us();
                editor.preview_us = clip.position_us + duration / 2;
            }
    }
    auto number = [](double value) {
        std::ostringstream out;
        out << std::setprecision(9) << value;
        return out.str();
    };
    auto color = [](std::uint32_t value) {
        std::ostringstream out;
        out << std::hex << std::uppercase << std::setfill('0') << std::setw(6) << value;
        return out.str();
    };
    editor.background = style.background;
    editor.opacity = style.opacity;
    editor.fields = {style.text, number(duration / 1'000'000.0), number(style.font_size),
        color(style.color), color(style.background_color), number(style.x * 100), number(style.y * 100),
        number(style.fade_in_us / 1'000'000.0), number(style.fade_out_us / 1'000'000.0),
        number(style.background_opacity * 100)};
    editor.picker.set_color(Color{static_cast<unsigned char>(style.color >> 16),
        static_cast<unsigned char>(style.color >> 8), static_cast<unsigned char>(style.color),
        static_cast<unsigned char>(std::lround(style.opacity * 255))});
    text_editor_ = std::move(editor);
}

trekker::movie_editor::TextStyle MovieEditorWindow::draft_text_style() const {
    const auto& editor = *text_editor_;
    auto number = [&](std::size_t field) {
        std::size_t used = 0;
        const double value = std::stod(editor.fields[field], &used);
        if (used != editor.fields[field].size() || !std::isfinite(value))
            throw std::invalid_argument("Enter a valid number in each numeric field.");
        return value;
    };
    auto seconds = [&](std::size_t field) {
        const double value = number(field);
        if (value < 0 || value > 86400)
            throw std::invalid_argument("Times must be between 0 and 86400 seconds.");
        return static_cast<std::int64_t>(std::llround(value * 1'000'000));
    };
    auto color = [&](std::size_t field) {
        std::string text = editor.fields[field];
        if (!text.empty() && text.front() == '#') text.erase(0, 1);
        if (text.size() != 6 || text.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos)
            throw std::invalid_argument("Colors need six hex digits, e.g. FFFFFF.");
        return static_cast<std::uint32_t>(std::stoul(text, nullptr, 16));
    };
    trekker::movie_editor::TextStyle style;
    style.text = editor.fields[0];
    style.background = editor.background;
    style.opacity = editor.opacity;
    style.font_size = number(2);
    style.color = color(3);
    style.background_color = color(4);
    style.x = number(5) / 100;
    style.y = number(6) / 100;
    style.fade_in_us = seconds(7);
    style.fade_out_us = seconds(8);
    style.background_opacity = number(9) / 100;
    trekker::movie_editor::validate_text_style(style);
    return style;
}

void MovieEditorWindow::apply_text_editor() {
    if (!text_editor_) return;
    try {
        const auto& editor = *text_editor_;
        auto number = [&](std::size_t field) {
            std::size_t used = 0;
            const double value = std::stod(editor.fields[field], &used);
            if (used != editor.fields[field].size() || !std::isfinite(value))
                throw std::invalid_argument("Enter a valid number in each numeric field.");
            return value;
        };
        auto seconds = [&](std::size_t field) {
            const double value = number(field);
            if (value < 0 || value > 86400)
                throw std::invalid_argument("Times must be between 0 and 86400 seconds.");
            return static_cast<std::int64_t>(std::llround(value * 1'000'000));
        };
        const auto style = draft_text_style();
        const auto duration = seconds(1);
        if (editor.edit) {
            project_.edit_text_clip(editor.track_id, editor.clip_id, style, duration);
        } else {
            selected_clip_id_ = project_.add_text_clip(style, playhead_us_, duration);
            selected_track_id_ = project_.timeline().tracks().back().id();
            const int count = static_cast<int>(project_.timeline().tracks().size());
            int width = 0, height = 0;
            window_.client_size(width, height);
            const auto layout = compute_layout(width, height);
            track_scroll_ = std::max(0, count - std::max(1, (layout.tracks_y1 - layout.tracks_y0) / layout.lane_h));
        }
        if (selected_track_id_ && selected_clip_id_) {
            const auto* track = project_.timeline().track(*selected_track_id_);
            if (track) for (const auto& clip : track->clips())
                if (clip.id == *selected_clip_id_)
                    playhead_us_ = clip.position_us + clip.timeline_duration_us() / 2;
        }
        text_editor_.reset();
        set_status("Text applied. Select its clip and Edit Text to change it. Ctrl/Cmd+Z to undo.");
    } catch (const std::exception& error) {
        text_editor_->error = error.what();
    }
}

void MovieEditorWindow::handle_text_input(const std::string& text) {
    if (!text_editor_ || text.empty()) return;
    auto& editor = *text_editor_;
    auto& value = editor.fields[editor.active];
    if (editor.replace) { value.clear(); editor.replace = false; }
    if (value.size() + text.size() > (editor.active == 0 ? 4096U : 32U)) {
        editor.error = "Text field is too long.";
        return;
    }
    value += text;
    editor.error.clear();
}

void MovieEditorWindow::text_editor_pointer(int x, int y, bool release, bool press) {
    if (!text_editor_) return;
    auto& editor = *text_editor_;
    if (release) {
        if (editor.drag) text_editor_pointer(x, y, false, false);
        editor.drag = 0;
        return;
    }
    int width = 0, height = 0;
    window_.client_size(width, height);
    const int left = (width - 900) / 2, top = (height - 570) / 2;
    const int px = left + 430, py = top + 52, pw = 450, ph = 253;
    try {
        if (press && x >= px && x < px + pw && y >= py && y < py + ph) {
            const auto bounds = trekker::movie_editor::text_layout(draft_text_style(), pw, ph);
            const bool inside = x >= px + bounds.x && x < px + bounds.x + bounds.width &&
                y >= py + bounds.y && y < py + bounds.y + bounds.height;
            editor.offset_x = inside ? x - px - bounds.x : bounds.width / 2;
            editor.offset_y = inside ? y - py - bounds.y : bounds.height / 2;
            editor.drag = 1;
        }
        if (editor.drag == 1) {
            const auto bounds = trekker::movie_editor::text_layout(draft_text_style(), pw, ph);
            editor.fields[5] = std::to_string(100.0 * std::clamp(
                (x - px - editor.offset_x) / static_cast<double>(std::max(1, pw - bounds.width)), 0.0, 1.0));
            editor.fields[6] = std::to_string(100.0 * std::clamp(
                (y - py - editor.offset_y) / static_cast<double>(std::max(1, ph - bounds.height)), 0.0, 1.0));
            editor.error.clear();
            return;
        }
        if (press && x >= px && x < px + pw && y >= top + 334 && y < top + 514) editor.drag = 2;
        if (editor.drag == 2 && editor.picker.pointer(x, y, px, top + 334, pw, 180)) {
            const auto color = editor.picker.color();
            std::ostringstream hex;
            hex << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << static_cast<int>(color.r)
                << std::setw(2) << static_cast<int>(color.g) << std::setw(2) << static_cast<int>(color.b);
            editor.fields[editor.picker_field] = hex.str();
            if (editor.picker_field == 4) editor.fields[9] = std::to_string(color.a * 100.0 / 255);
            else editor.opacity = color.a / 255.0;
            editor.error.clear();
        }
    } catch (const std::exception& error) { editor.error = error.what(); }
}

void MovieEditorWindow::text_editor_key(const graphics::full_application_window::KeyEvent& event) {
    using graphics::full_application_window::Key;
    if (event.key == static_cast<int>(Key::Escape)) { text_editor_.reset(); return; }
    if (event.key == 9) {
        text_editor_->active = (text_editor_->active + (event.shift ? 9 : 1)) % 10;
        text_editor_->replace = true;
    } else if (event.key == static_cast<int>(Key::Enter)) {
        if (event.shift && text_editor_->active == 0) handle_text_input("\n");
        else apply_text_editor();
    } else if (event.command && event.key == 'A') {
        text_editor_->replace = true;
    } else if (event.key == static_cast<int>(Key::Backspace) || event.key == static_cast<int>(Key::Delete)) {
        auto& value = text_editor_->fields[text_editor_->active];
        if (text_editor_->replace) value.clear();
        else if (!value.empty()) {
            std::size_t position = value.size() - 1;
            while (position > 0 && (static_cast<unsigned char>(value[position]) & 0xc0) == 0x80) --position;
            value.erase(position);
        }
        text_editor_->replace = false;
    }
}

void MovieEditorWindow::handle_key(const graphics::full_application_window::KeyEvent& event) {
    using graphics::full_application_window::Key;
    if (text_editor_) { text_editor_key(event); return; }
    if (event.repeat) return;
    if (event.key == static_cast<int>(Key::Escape)) {
        drag_.reset(); media_drag_.reset(); open_menu_.reset(); track_context_menu_.reset(); return;
    }
    if (video_import_.valid() || export_job_.valid()) { set_status("Please wait for the current media operation."); return; }
    if (drag_ || media_drag_) return;
    try {
        if (event.command) {
            switch (event.key) {
                case 'Z': undo_redo(event.shift); return;
                case 'Y': undo_redo(true); return;
                case 'I': pause_playback(); do_import_media(); return;
                case 'O': pause_playback(); do_open_project(); return;
                case 'S':
                    pause_playback();
                    if (last_project_path_.empty() || event.shift) do_save_project();
                    else { project_.save_project(last_project_path_); set_status("Project saved."); }
                    return;
                case 'B': handle_menu_command(1, 3); return;
                case 'E': do_export_mp4(); return;
                default: return;
            }
        }
        switch (static_cast<Key>(event.key)) {
            case Key::Space: toggle_playback(); return;
            case Key::Enter: pause_playback(); add_selected_media(); return;
            case Key::Delete:
            case Key::Backspace: handle_menu_command(1, 4); return;
            case Key::Left: pause_playback(); playhead_us_ = std::max<std::int64_t>(0, playhead_us_ - 1'000'000 / 24); return;
            case Key::Right: pause_playback(); playhead_us_ = std::min(project_.duration_us(), playhead_us_ + 1'000'000 / 24); return;
            case Key::Home: pause_playback(); playhead_us_ = 0; return;
            case Key::End: pause_playback(); playhead_us_ = project_.duration_us(); return;
            default: return;
        }
    } catch (const std::exception& error) { pause_playback(); report_error(error.what()); }
}

void MovieEditorWindow::update_drag(int x, int y, bool release) {
    if (!drag_ && !media_drag_) return;
    int width = 0, height = 0;
    window_.client_size(width, height);
    const auto layout = compute_layout(width, height);
    if (media_drag_) {
        auto& media = *media_drag_;
        media.x = x;
        media.y = y;
        media.moved = media.moved || std::abs(x - media.start_x) >= 3 || std::abs(y - media.start_y) >= 3;
        if (!release) return;
        const auto completed = media;
        media_drag_.reset();
        if (!completed.moved) return;
        if (x < layout.track_header_w || x >= layout.width || y < layout.tracks_y0 || y >= layout.tracks_y1) {
            set_status("Drop media on the timeline to add a clip.");
            return;
        }
        pause_playback();
        const auto& tracks = project_.timeline().tracks();
        const auto lane = static_cast<std::size_t>((y - layout.tracks_y0) / layout.lane_h + track_scroll_);
        const auto position = x_to_time_us(layout, x);
        if (lane >= tracks.size()) {
            selected_clip_id_ = project_.add_clip_on_new_track(completed.key, position);
            selected_track_id_ = project_.timeline().tracks().back().id();
        } else {
            const auto track_id = tracks[lane].id();
            selected_clip_id_ = project_.add_clip(track_id, completed.key, position);
            selected_track_id_ = track_id;
        }
        set_status("Media added. Drag edges to trim, Ctrl/Cmd+Z to undo.");
        return;
    }
    auto& drag = *drag_;
    drag.moved = drag.moved || std::abs(x - drag.start_x) >= 3 || std::abs(y - drag.start_y) >= 3;
    if (!drag.moved) { if (release) drag_.reset(); return; }
    const auto* asset = project_.media_bin().find(drag.original.source_path);
    const auto* text = project_.text_style(drag.from_track, drag.original.id);
    if (!asset && !text) { drag_.reset(); set_status("Dragged media is no longer available."); return; }
    auto delta = static_cast<std::int64_t>((x - drag.start_x) *
        static_cast<double>(visible_duration_us_) / std::max(1, layout.width - layout.track_header_w));
    drag.preview = preview_edit(drag.original, drag.kind, delta,
        text ? std::max<std::int64_t>(drag.original.out_point_us, 86'400'000'000) : asset->duration_us);
    if (drag.kind == DragKind::Move) {
        const int lane = (y - layout.tracks_y0) / layout.lane_h + track_scroll_;
        const auto& tracks = project_.timeline().tracks();
        if (y >= layout.tracks_y0 && lane >= 0 && lane < static_cast<int>(tracks.size()))
            drag.to_track = tracks[static_cast<std::size_t>(lane)].id();
        else drag.to_track = drag.from_track;
    }
    set_status(drag.kind == DragKind::Move ? "Move clip; release to apply, Esc to cancel." :
               "Trim clip edge; release to apply, Esc to cancel.");
    if (!release) return;
    const auto completed = drag;
    drag_.reset();
    try {
        if (completed.kind == DragKind::Move)
            project_.move_clip(completed.from_track, completed.original.id, completed.to_track, completed.preview.position_us);
        else project_.trim_clip(completed.from_track, completed.original.id, completed.preview.in_point_us,
                                completed.preview.out_point_us, completed.preview.position_us);
        selected_track_id_ = completed.to_track;
        set_status("Edit applied. Ctrl/Cmd+Z to undo.");
    } catch (const std::exception& error) { report_error(error.what()); }
}

void MovieEditorWindow::do_import_media() {
    const std::string path = pick_path(app_builder::os_generics::DialogAction::OpenFile, "Import Media",
        {{"Media", {"*.mp4", "*.mov", "*.m4v", "*.mkv", "*.webm", "*.avi", "*.gif",
                    "*.png", "*.jpg", "*.jpeg", "*.bmp", "*.wav"}}, {"All Files", {"*.*"}}});
    if (path.empty()) return;
    std::string ext = fs::path(path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    try {
        if (ext == ".wav") pending_media_key_ = project_.media_bin().import_audio(path);
        else if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp")
            pending_media_key_ = project_.media_bin().import_still_image(path);
        else {
            set_status("Importing video with FFmpeg... Please wait.");
            video_import_ = std::async(std::launch::async, [path] { return trekker::movie_editor::decode_ffmpeg_video(path); });
            return;
        }
        set_status("Media imported. Click Add Clip to start editing.");
    } catch (const std::exception& e) {
        set_status(std::string("Import failed: ") + e.what());
        window_.show_info_dialog("Media import failed", e.what());
    }
}

void MovieEditorWindow::add_selected_media() {
    if (!pending_media_key_) {
        set_status("Import media, then select a media item first.");
        return;
    }
    const auto* asset = project_.media_bin().find(*pending_media_key_);
    if (!asset) { set_status("The selected media is no longer available."); return; }
    const auto type = asset->type == trekker::movie_editor::MediaType::Audio
        ? timeline::TrackType::Audio : timeline::TrackType::Video;
    timeline::Track* destination = selected_track_id_ ? project_.timeline().track(*selected_track_id_) : nullptr;
    if (!destination || destination->type() != type) {
        destination = nullptr;
        for (const auto& track : project_.timeline().tracks()) {
            if (track.type() == type && !track.locked()) {
                destination = project_.timeline().track(track.id());
                break;
            }
        }
    }
    if (!destination) destination = type == timeline::TrackType::Audio
        ? &project_.add_audio_track("Audio") : &project_.add_video_track("Video");
    if (destination->locked()) { set_status("The selected track is locked."); return; }
    std::int64_t end = 0;
    for (const auto& clip : destination->clips()) end = std::max(end, clip.timeline_end_us());
    selected_track_id_ = destination->id();
    selected_clip_id_ = project_.add_clip(destination->id(), *pending_media_key_, end);
    playhead_us_ = end;
    visible_duration_us_ = std::max<std::int64_t>(10'000'000, project_.duration_us() + project_.duration_us() / 10);
    view_start_us_ = 0;
    set_status("Added to timeline. Use Play to preview; click the ruler to scrub.");
}

void MovieEditorWindow::do_import_still_image() {
    const std::string path = pick_path(app_builder::os_generics::DialogAction::OpenFile, "Select Image",
                                       {{"Images", {"*.png", "*.jpg", "*.jpeg", "*.bmp"}}, {"All Files", {"*.*"}}});
    if (path.empty()) return;
    try {
        const std::string key = project_.media_bin().import_still_image(path);
        pending_media_key_ = key;
        set_status("Imported still image: " + key);
    } catch (const std::exception& e) {
        set_status(std::string("Import failed: ") + e.what());
    }
}

void MovieEditorWindow::do_import_audio() {
    const std::string path = pick_path(app_builder::os_generics::DialogAction::OpenFile, "Select WAV Audio",
                                       {{"WAV Audio", {"*.wav"}}, {"All Files", {"*.*"}}});
    if (path.empty()) return;
    try {
        const std::string key = project_.media_bin().import_audio(path);
        pending_media_key_ = key;
        set_status("Imported audio: " + key);
    } catch (const std::exception& e) {
        set_status(std::string("Import failed: ") + e.what());
    }
}

void MovieEditorWindow::do_save_project() {
    const std::string path = pick_path(app_builder::os_generics::DialogAction::SaveFile, "Save Project",
                                       {{"Movie Editor Project", {"*.json"}}}, "project.json");
    if (path.empty()) return;
    try {
        project_.save_project(path);
        last_project_path_ = path;
        set_status("Saved project: " + path);
    } catch (const std::exception& e) {
        set_status(std::string("Save failed: ") + e.what());
    }
}

void MovieEditorWindow::do_open_project() {
    const std::string path = pick_path(app_builder::os_generics::DialogAction::OpenFile, "Open Project",
                                       {{"Movie Editor Project", {"*.json"}}, {"All Files", {"*.*"}}});
    if (path.empty()) return;
    try {
        project_.load_project(path);
        last_project_path_ = path;
        playhead_us_ = 0;
        selected_clip_id_.reset();
        selected_track_id_.reset();
        pending_media_key_.reset();
        media_scroll_ = track_scroll_ = 0;
        view_start_us_ = 0;
        visible_duration_us_ = std::max<std::int64_t>(10'000'000, project_.duration_us() + project_.duration_us() / 10);
        set_status("Opened project: " + path);
    } catch (const std::exception& e) {
        set_status(std::string("Open failed: ") + e.what());
    }
}

void MovieEditorWindow::do_export_avi() {
    const std::string path = pick_path(app_builder::os_generics::DialogAction::SaveFile, "Export AVI",
                                       {{"AVI Video", {"*.avi"}}}, "export.avi");
    if (path.empty()) return;
    try {
        project_.export_avi(path, 24.0, 640, 360);
        set_status("Exported AVI: " + path);
    } catch (const std::exception& e) {
        set_status(std::string("Export failed: ") + e.what());
    }
}

void MovieEditorWindow::do_export_gif() {
    const std::string path = pick_path(app_builder::os_generics::DialogAction::SaveFile, "Export GIF",
                                       {{"Animated GIF", {"*.gif"}}}, "export.gif");
    if (path.empty()) return;
    try {
        project_.export_gif(path, 0, project_.duration_us(), 12.0, 480, 270);
        set_status("Exported GIF: " + path);
    } catch (const std::exception& e) {
        set_status(std::string("Export failed: ") + e.what());
    }
}

void MovieEditorWindow::do_export_wav() {
    const std::string path = pick_path(app_builder::os_generics::DialogAction::SaveFile, "Export Audio",
                                       {{"WAV Audio", {"*.wav"}}}, "export.wav");
    if (path.empty()) return;
    try {
        project_.export_audio(path, trekker::audio::container::WavEncoding::PCM16);
        set_status("Exported audio: " + path);
    } catch (const std::exception& e) {
        set_status(std::string("Export failed: ") + e.what());
    }
}

void MovieEditorWindow::do_show_version() {
    const std::string message = "CoolBox Movie Editor\nVersion " + version_string();
    window_.show_info_dialog("About CoolBox Movie Editor", message);
    set_status("Version " + version_string());
}

void MovieEditorWindow::delete_selected_track() {
    if (!selected_track_id_) { set_status("Select a track first."); return; }
    pause_playback();
    if (!project_.delete_track(*selected_track_id_)) {
        set_status("The track cannot be deleted (missing or locked).");
        return;
    }
    selected_track_id_.reset();
    selected_clip_id_.reset();
    track_scroll_ = std::min(track_scroll_,
        std::max(0, static_cast<int>(project_.timeline().tracks().size()) - 1));
    playhead_us_ = std::min(playhead_us_, project_.duration_us());
    view_start_us_ = std::min(view_start_us_, playhead_us_);
    set_status("Track and its clips deleted. Ctrl/Cmd+Z to undo.");
}

void MovieEditorWindow::handle_context_click(int x, int y) {
    if (text_editor_) return;
    drag_.reset();
    media_drag_.reset();
    open_menu_.reset();
    track_context_menu_.reset();
    if (video_import_.valid() || export_job_.valid()) {
        set_status("Media operation in progress. Please wait.");
        return;
    }
    int width = 0, height = 0;
    window_.client_size(width, height);
    const auto layout = compute_layout(width, height);
    if (x < 0 || x >= width || y < layout.tracks_y0 || y >= layout.tracks_y1) return;
    const auto lane = static_cast<std::size_t>((y - layout.tracks_y0) / layout.lane_h + track_scroll_);
    const auto& tracks = project_.timeline().tracks();
    if (lane >= tracks.size()) return;
    selected_track_id_ = tracks[lane].id();
    selected_clip_id_.reset();
    track_context_menu_ = TrackContextMenu{tracks[lane].id(),
        std::clamp(x, 0, std::max(0, width - 220)),
        std::clamp(y, 0, std::max(0, layout.status_y0 - 36))};
}

void MovieEditorWindow::handle_pointer(
    const graphics::full_application_window::PointerPressEvent& event, bool release) {
    using graphics::full_application_window::PointerButton;
    try {
        if (text_editor_ && release) {
            text_editor_pointer(event.x, event.y, true, false);
            return;
        }
        if (event.button == PointerButton::Right) {
            if (!release) handle_context_click(event.x, event.y);
        } else if (release) {
            update_drag(event.x, event.y, true);
        } else {
            handle_click(event.x, event.y);
        }
    } catch (const std::exception& error) {
        drag_.reset();
        media_drag_.reset();
        pause_playback();
        report_error(error.what());
    }
}

void MovieEditorWindow::handle_click(int x, int y) {
    if (video_import_.valid() || export_job_.valid()) {
        set_status("Media operation in progress. Please wait.");
        return;
    }
    int cw = 1920, ch = 1200;
    window_.client_size(cw, ch);
    const Layout l = compute_layout(cw, ch);
    if (text_editor_) {
        const int left = (cw - 900) / 2, top = (ch - 570) / 2;
        text_editor_pointer(x, y, false, true);
        if (text_editor_->drag) return;
        if (x >= left + 160 && x < left + 410 && y >= top + 52 && y < top + 412) {
            text_editor_->active = static_cast<std::size_t>((y - top - 52) / 36);
            text_editor_->replace = true;
            if (text_editor_->active == 3 || text_editor_->active == 4) {
                text_editor_->picker_field = text_editor_->active;
                try {
                    const auto style = draft_text_style();
                    const auto rgb = text_editor_->active == 3 ? style.color : style.background_color;
                    text_editor_->picker.set_color(Color{static_cast<unsigned char>(rgb >> 16),
                        static_cast<unsigned char>(rgb >> 8), static_cast<unsigned char>(rgb),
                        text_editor_->active == 3 ? static_cast<unsigned char>(std::lround(style.opacity * 255)) :
                            static_cast<unsigned char>(std::lround(style.background_opacity * 255))});
                } catch (const std::exception& error) { text_editor_->error = error.what(); }
            }
        } else if (x >= left + 16 && x < left + 410 && y >= top + 420 && y < top + 452) {
            text_editor_->background = !text_editor_->background;
        } else if (y >= top + 520 && y < top + 560) {
            if (x >= left + 620 && x < left + 740) text_editor_.reset();
            else if (x >= left + 750 && x < left + 880) apply_text_editor();
        }
        return;
    }
    if (selected_track_id_ && selected_clip_id_ &&
        project_.text_style(*selected_track_id_, *selected_clip_id_) &&
        x >= l.inspector_x0 + 10 && x < l.inspector_x1 - 10 &&
        y >= l.inspector_y0 + 210 && y < l.inspector_y0 + 248) {
        open_text_editor(false, true);
        return;
    }
    if (track_context_menu_) {
        const auto menu = *track_context_menu_;
        track_context_menu_.reset();
        if (x >= menu.x && x < menu.x + 220 && y >= menu.y && y < menu.y + 36) {
            selected_track_id_ = menu.track_id;
            handle_menu_command(1, 8);
        }
        return;
    }

    // Topbar menu dropdown (File/Edit/Help) takes priority over everything
    // else: if a dropdown is open, resolve the click against it first (and
    // dismiss the dropdown either way — standard click-to-dismiss behaviour)
    // rather than letting the click "fall through" to whatever is drawn
    // underneath the dropdown panel.
    if (open_menu_) {
        const auto items = dropdown_item_rects(l, *open_menu_);
        for (const auto& item : items) {
            if (x >= item.x0 && x < item.x1 && y >= item.y0 && y < item.y1) {
                const std::size_t menu_index = *open_menu_;
                open_menu_.reset();
                handle_menu_command(menu_index, item.item_index);
                return;
            }
        }
        open_menu_.reset();
        // Fall through: a click outside the dropdown (but still possibly on
        // a topbar button, handled next) shouldn't be silently swallowed.
    }

    // Topbar menu buttons (File/Edit/Help).
    for (const auto& button : topbar_buttons(l)) {
        if (x >= button.x0 && x < button.x1 && y >= button.y0 && y < button.y1) {
            open_menu_ = (open_menu_ && *open_menu_ == button.menu_index) ? std::nullopt
                                                                          : std::make_optional(button.menu_index);
            return;
        }
    }
    // Media bin rows.
    if (x >= l.media_x0 && x < l.media_x1 && y >= l.media_y0 && y < l.media_y1) {
        if (y < l.media_y0 + 68) return;
        const int row_h = 48;
        const int row = (y - l.media_y0 - 68) / row_h + media_scroll_;
        const auto& assets = project_.media_bin().assets();
        if (row >= 0 && static_cast<std::size_t>(row) < assets.size()) {
            pending_media_key_ = assets[static_cast<std::size_t>(row)].path;
            media_drag_ = MediaDrag{*pending_media_key_, x, y, x, y};
            set_status("Drag media onto a matching track or empty timeline space.");
        }
        return;
    }

    // Transport buttons.
    if ((y >= l.transport_y0 && y < l.transport_y1) || y < l.topbar_y1) {
        for (const auto& button : transport_buttons(l)) {
            if (x >= button.x0 && x < button.x1 && y >= button.y0 && y < button.y1) {
                switch (button.id) {
                    case TransportButton::ImportMedia: pause_playback(); do_import_media(); break;
                    case TransportButton::AddMedia: pause_playback(); add_selected_media(); break;
                    case TransportButton::ZoomIn: visible_duration_us_ = std::max<std::int64_t>(1'000'000, visible_duration_us_ / 2); break;
                    case TransportButton::ZoomOut: visible_duration_us_ = std::min<std::int64_t>(86'400'000'000, visible_duration_us_ * 2); break;
                    case TransportButton::Fit: view_start_us_ = 0; visible_duration_us_ = std::max<std::int64_t>(10'000'000, project_.duration_us() + project_.duration_us() / 10); break;
                    case TransportButton::Undo: undo_redo(false); break;
                    case TransportButton::Redo: undo_redo(true); break;
                    case TransportButton::ExportMp4: do_export_mp4(); break;
                    case TransportButton::AddText: open_text_editor(false); break;
                    case TransportButton::GoToStart: pause_playback(); playhead_us_ = 0; break;
                    case TransportButton::PlayPause: toggle_playback(); break;
                    case TransportButton::Stop: pause_playback(); playhead_us_ = 0; break;
                    case TransportButton::GoToEnd: pause_playback(); playhead_us_ = project_.duration_us(); break;
                    case TransportButton::Split: handle_menu_command(1, 3); break;
                    case TransportButton::Delete: handle_menu_command(1, 4); break;
                    case TransportButton::AddVideoTrack: pause_playback(); project_.add_video_track("Video"); break;
                    case TransportButton::AddAudioTrack: pause_playback(); project_.add_audio_track("Audio"); break;
                }
                return;
            }
        }
        return;
    }

    // Ruler: scrub playhead.
    if (y >= l.ruler_y0 && y < l.ruler_y1) {
        pause_playback();
        playhead_us_ = x_to_time_us(l, x);
        return;
    }

    // Track lanes.
    if (y >= l.tracks_y0 && y < l.tracks_y1) {
        const int lane_index = (y - l.tracks_y0) / l.lane_h + track_scroll_;
        const auto& tracks = project_.timeline().tracks();
        if (lane_index < 0 || static_cast<std::size_t>(lane_index) >= tracks.size()) return;
        auto it = tracks.begin();
        std::advance(it, lane_index);
        const auto& track = *it;

        if (x < l.track_header_w) {
            selected_track_id_ = track.id();
            set_status("Selected track: " + track.name());
            return;
        }

        const std::int64_t clicked_time = x_to_time_us(l, x);
        const timeline::Clip* clip = track.clip_at(clicked_time);
        for (const auto& candidate : track.clips()) {
            if (candidate.timeline_end_us() <= view_start_us_ || candidate.position_us >= view_start_us_ + visible_duration_us_) continue;
            if (std::abs(x - time_to_x(l, candidate.position_us)) <= 7 ||
                std::abs(x - time_to_x(l, candidate.timeline_end_us())) <= 7) {
                clip = &candidate;
                break;
            }
        }
        if (clip) {
            pause_playback();
            selected_track_id_ = track.id();
            selected_clip_id_ = clip->id;
            const int left = time_to_x(l, clip->position_us);
            const int right = time_to_x(l, clip->timeline_end_us());
            const auto kind = std::abs(x - left) <= 9 && std::abs(x - left) <= std::abs(x - right) ? DragKind::TrimStart :
                              std::abs(x - right) <= 9 ? DragKind::TrimEnd : DragKind::Move;
            if (!track.locked()) drag_ = DragState{kind, *clip, *clip, track.id(), track.id(), x, y};
            set_status("Drag clip body to move; drag edges to trim. Ctrl/Cmd+Z to undo.");
        } else if (pending_media_key_ && !track.locked()) {
            pause_playback();
            const auto* asset = project_.media_bin().find(*pending_media_key_);
            if (!asset || ((asset->type == trekker::movie_editor::MediaType::Audio) !=
                           (track.type() == timeline::TrackType::Audio))) {
                set_status("Choose a matching video or audio track.");
                return;
            }
            project_.add_clip(track.id(), *pending_media_key_, clicked_time);
            selected_track_id_ = track.id();
            set_status("Placed clip on track " + track.name());
            pending_media_key_.reset();
        }
        return;
    }
}

void MovieEditorWindow::render_scene() {
    int cw = 1920, ch = 1200;
    window_.client_size(cw, ch);
    cw = std::max(1, cw);
    ch = std::max(1, ch);
    const Layout l = compute_layout(cw, ch);

    // A single unified dark background avoids a mismatched "leftover" colour
    // showing through in any area not explicitly covered by a panel below.
    EditorCanvas canvas(cw, ch, ui_font_);

    // Native menu backends only draw the action strip in the client area.
    canvas.draw_rect(0, 0, l.width, l.topbar_y1, toolbar_color, true);
    for (const auto& button : topbar_buttons(l)) {
        const bool is_open = open_menu_ && *open_menu_ == button.menu_index;
        if (is_open) canvas.draw_rect(button.x0, button.y0, button.x1 - button.x0, button.y1 - button.y0, Colors::Gray, true);
        canvas.draw_text(button.x0 + 10, button.y0 + (button.y1 - button.y0 - 16) / 2,
                         menu_definitions()[button.menu_index].title, Colors::White, 2);
    }
    // Media bin.
    canvas.draw_rect(l.media_x0, l.media_y0, l.media_x1 - l.media_x0, l.media_y1 - l.media_y0, panel_color, true);
    canvas.draw_text(l.media_x0 + 6, l.media_y0 + 6, "MEDIA BIN", Colors::Cyan, 2);
    canvas.draw_text(l.media_x0 + 8, l.media_y0 + 34, "Drag to timeline or Add Clip", Colors::LightGray, 1, l.media_x1 - 16);
    canvas.draw_line(l.media_x0, l.media_y0 + 60, l.media_x1, l.media_y0 + 60, Colors::DarkGray, 1);
    {
        int row_y = l.media_y0 + 68;
        const int row_h = 48;
        int index = 0;
        for (const auto& asset : project_.media_bin().assets()) {
            if (index++ < media_scroll_) continue;
            const bool selected = pending_media_key_ && *pending_media_key_ == asset.path;
            if (selected) canvas.draw_rect(l.media_x0, row_y - 3, l.media_x1 - l.media_x0, row_h, Colors::Blue, true);
            const char* type_tag = asset.type == trekker::movie_editor::MediaType::ImageSequence ? "[SEQ] "
                : asset.type == trekker::movie_editor::MediaType::StillImage ? "[IMG] "
                : asset.type == trekker::movie_editor::MediaType::ImportedVideo ? "[VID] " : "[WAV] ";
            std::string label = type_tag + fs::path(asset.path).filename().string();
            canvas.draw_text(l.media_x0 + 6, row_y, label, selected ? Colors::White : Colors::LightGray, 1, l.media_x1 - 18);
            canvas.draw_text(l.media_x0 + 6, row_y + 22, format_timecode(asset.duration_us), Colors::LightGray, 1);
            row_y += row_h;
            if (row_y > l.media_y1) break;
        }
        if (project_.media_bin().assets().empty()) {
            canvas.draw_text(l.media_x0 + 12, row_y + 16, "No media yet", Colors::White, 2);
            canvas.draw_text(l.media_x0 + 12, row_y + 50, "Click Import Media above.", Colors::LightGray, 1);
            canvas.draw_text(l.media_x0 + 12, row_y + 76, "Video, photos, or WAV audio", Colors::LightGray, 1);
        }
    }
    canvas.draw_line(l.media_x1, 0, l.media_x1, l.tracks_y1, Colors::DarkGray, 1);

    // Preview.
    canvas.draw_rect(l.preview_x0, l.preview_y0, l.preview_x1 - l.preview_x0, l.preview_y1 - l.preview_y0, Colors::Black, true);
    {
        const trekker::video::VideoFrame frame = project_.render_frame_at(playhead_us_, kPreviewWidth, kPreviewHeight);
        const int avail_w = l.preview_x1 - l.preview_x0 - 20;
        const int avail_h = l.preview_y1 - l.preview_y0 - 36;
        const double scale = std::min(static_cast<double>(avail_w) / kPreviewWidth, static_cast<double>(avail_h) / kPreviewHeight);
        const int draw_w = std::max(1, static_cast<int>(kPreviewWidth * scale));
        const int draw_h = std::max(1, static_cast<int>(kPreviewHeight * scale));
        const int off_x = l.preview_x0 + (l.preview_x1 - l.preview_x0 - draw_w) / 2;
        const int off_y = l.preview_y0 + (l.preview_y1 - l.preview_y0 - 26 - draw_h) / 2;
        const auto& plane = frame.planes[0];
        for (int y = 0; y < draw_h; ++y) {
            const std::size_t sy = std::min<std::size_t>(kPreviewHeight - 1, (static_cast<std::size_t>(y) * kPreviewHeight) / draw_h);
            for (int x = 0; x < draw_w; ++x) {
                const std::size_t sx = std::min<std::size_t>(kPreviewWidth - 1, (static_cast<std::size_t>(x) * kPreviewWidth) / draw_w);
                Color c{plane.at(sx * 3 + 0, sy), plane.at(sx * 3 + 1, sy), plane.at(sx * 3 + 2, sy), 255};
                canvas.set_pixel(off_x + x, off_y + y, c);
            }
        }
        canvas.draw_rect(off_x - 1, off_y - 1, draw_w + 2, draw_h + 2, Colors::Gray, false);
        if (project_.timeline().tracks().empty()) {
            canvas.draw_text(l.preview_x0 + 24, l.preview_y0 + 65, "Preview", Colors::White, 3, avail_w);
            canvas.draw_text(l.preview_x0 + 24, l.preview_y0 + 105, "Import media and add it to the timeline.", Colors::LightGray, 2, avail_w);
        }
        canvas.draw_text(l.preview_x0 + 6, l.preview_y1 - 28, format_timecode(playhead_us_) + " / " + format_timecode(project_.duration_us()), Colors::White, 3);
    }

    // Transport buttons (two rows of four).
    for (const auto& button : transport_buttons(l)) {
        const Color fill = button.id == TransportButton::ImportMedia ? Color{45, 107, 211, 255} : button_color;
        canvas.draw_rounded_rect(button.x0, button.y0, button.x1 - button.x0, button.y1 - button.y0, 5, fill, true);
        canvas.draw_text(button.x0 + 8, button.y0 + (button.y1 - button.y0 - 21) / 2,
                         button.label, Colors::White, button.y0 < l.topbar_y1 ? 1 : 2, button.x1 - button.x0 - 16);
    }

    // Inspector.
    canvas.draw_rect(l.inspector_x0, l.inspector_y0, l.inspector_x1 - l.inspector_x0, l.inspector_y1 - l.inspector_y0, panel_color, true);
    canvas.draw_line(l.inspector_x0, 0, l.inspector_x0, l.tracks_y1, Colors::DarkGray, 1);
    canvas.draw_text(l.inspector_x0 + 6, l.inspector_y0 + 6, "INSPECTOR", Colors::Cyan, 2);
    canvas.draw_line(l.inspector_x0, l.inspector_y0 + 34, l.inspector_x1, l.inspector_y0 + 34, Colors::DarkGray, 1);
    {
        int row_y = l.inspector_y0 + 48;
        const int row_h = 22;
        if (!selected_clip_id_) {
            canvas.draw_text(l.inspector_x0 + 10, row_y, "Select a timeline clip", Colors::LightGray, 1);
            canvas.draw_text(l.inspector_x0 + 10, row_y + 26, "to inspect its details.", Colors::LightGray, 1);
            row_y += 52;
        }
        if (selected_track_id_) {
            canvas.draw_text(l.inspector_x0 + 6, row_y, "Track: " + std::to_string(*selected_track_id_), Colors::LightGray, 1);
            row_y += row_h;
        }
        if (selected_track_id_ && selected_clip_id_) {
            const timeline::Track* track = project_.timeline().track(*selected_track_id_);
            if (track) {
                for (const auto& clip : track->clips()) {
                    if (clip.id != *selected_clip_id_) continue;
                    canvas.draw_text(l.inspector_x0 + 6, row_y, "Clip #" + std::to_string(clip.id), Colors::White, 1); row_y += row_h;
                    canvas.draw_text(l.inspector_x0 + 6, row_y, "src: " + fs::path(clip.source_path).filename().string(), Colors::LightGray, 1, l.inspector_x1 - l.inspector_x0 - 12); row_y += row_h;
                    canvas.draw_text(l.inspector_x0 + 6, row_y, "pos: " + format_timecode(clip.position_us), Colors::LightGray, 1); row_y += row_h;
                    canvas.draw_text(l.inspector_x0 + 6, row_y, "dur: " + format_timecode(clip.timeline_duration_us()), Colors::LightGray, 1); row_y += row_h;
                    canvas.draw_text(l.inspector_x0 + 6, row_y, "vol: " + std::to_string(clip.volume), Colors::LightGray, 1); row_y += row_h;
                    canvas.draw_text(l.inspector_x0 + 6, row_y, "speed: " + std::to_string(clip.speed), Colors::LightGray, 1); row_y += row_h;
                    break;
                }
            }
            if (project_.text_style(*selected_track_id_, *selected_clip_id_)) {
                const int y = l.inspector_y0 + 210;
                canvas.draw_rect(l.inspector_x0 + 10, y, l.inspector_x1 - l.inspector_x0 - 20, 38, button_color, true);
                canvas.draw_text(l.inspector_x0 + 20, y + 8, "Edit Text / Title", Colors::White, 1);
            }
        }
    }

    // Timeline ruler.
    canvas.draw_rect(0, l.ruler_y0, l.width, l.ruler_y1 - l.ruler_y0, toolbar_color, true);
    const auto tick_us = std::max<std::int64_t>(1'000'000, visible_duration_us_ / std::max(1, (l.width - l.track_header_w) / 140));
    for (std::int64_t t = view_start_us_; t < view_start_us_ + visible_duration_us_; t += tick_us) {
        const int x = time_to_x(l, t);
        canvas.draw_line(x, l.ruler_y0, x, l.ruler_y1, Colors::Gray, 1);
        canvas.draw_text(x + 5, l.ruler_y0 + (l.ruler_y1 - l.ruler_y0 - 21) / 2, format_timecode(t), Colors::White, 2);
    }

    // Track lanes: fill the whole area first so there is no mismatched
    // "empty" background showing through when there are zero/few tracks.
    canvas.draw_rect(0, l.tracks_y0, l.width, l.tracks_y1 - l.tracks_y0, panel_color, true);
    {
        int lane_y = l.tracks_y0;
        int index = 0;
        for (const auto& track : project_.timeline().tracks()) {
            if (index++ < track_scroll_) continue;
            const bool track_selected = selected_track_id_ && *selected_track_id_ == track.id();
            canvas.draw_rect(0, lane_y, l.track_header_w, l.lane_h, track_selected ? Colors::Blue : Colors::DarkGray, true);
            canvas.draw_text(6, lane_y + 6, track.name().empty() ? "(track)" : track.name(), Colors::White, 1, l.track_header_w - 12);
            canvas.draw_text(6, lane_y + 22, track.type() == timeline::TrackType::Video ? "video" : "audio", Colors::LightGray, 1);
            canvas.draw_line(0, lane_y + l.lane_h - 1, l.width, lane_y + l.lane_h - 1, Colors::Gray, 1);

            auto visible_clips = track.clips();
            if (drag_ && drag_->moved) {
                visible_clips.erase(std::remove_if(visible_clips.begin(), visible_clips.end(),
                    [this](const auto& clip) { return clip.id == drag_->original.id; }), visible_clips.end());
                if (drag_->to_track == track.id()) visible_clips.push_back(drag_->preview);
            }
            for (const auto& clip : visible_clips) {
                if (clip.timeline_end_us() <= view_start_us_ || clip.position_us >= view_start_us_ + visible_duration_us_) continue;
                const int x0 = time_to_x(l, clip.position_us);
                const int x1 = time_to_x(l, clip.timeline_end_us());
                const bool clip_selected = selected_clip_id_ && *selected_clip_id_ == clip.id;
                const Color clip_color = track.type() == timeline::TrackType::Video ? Colors::Purple : Colors::Green;
                canvas.draw_rect(x0, lane_y + 3, std::max(1, x1 - x0), l.lane_h - 6, clip_color, true);
                if (clip_selected) {
                    canvas.draw_rect(x0, lane_y + 3, std::max(1, x1 - x0), l.lane_h - 6, Colors::White, false);
                    canvas.draw_rect(x0, lane_y + 10, 5, l.lane_h - 20, Colors::White, true);
                    canvas.draw_rect(x1 - 5, lane_y + 10, 5, l.lane_h - 20, Colors::White, true);
                }
                const auto* style = project_.text_style(track.id(), clip.id);
                if (x1 - x0 > 30) canvas.draw_text(x0 + 3, lane_y + 6,
                    style ? style->text : fs::path(clip.source_path).filename().string(),
                    Colors::White, 1, x1 - x0 - 6);
            }
            lane_y += l.lane_h;
            if (lane_y > l.tracks_y1) break;
        }
        if (project_.timeline().tracks().empty()) {
            canvas.draw_text(l.track_header_w + 30, lane_y + 36, "Your story starts here", Colors::White, 3);
            canvas.draw_text(l.track_header_w + 30, lane_y + 80, "Import Media, then drag a media item here to start.", Colors::LightGray, 2);
        }
    }
    canvas.draw_line(l.track_header_w, l.tracks_y0, l.track_header_w, l.tracks_y1, Colors::Gray, 1);
    if (media_drag_ && media_drag_->moved) {
        const auto* asset = project_.media_bin().find(media_drag_->key);
        const auto& tracks = project_.timeline().tracks();
        const int drop_y = media_drag_->y;
        const int drop_x = media_drag_->x;
        if (asset && drop_x >= l.track_header_w && drop_x < l.width &&
            drop_y >= l.tracks_y0 && drop_y < l.tracks_y1) {
            const auto lane = static_cast<std::size_t>((drop_y - l.tracks_y0) / l.lane_h + track_scroll_);
            const bool valid = lane >= tracks.size() || (!tracks[lane].locked() &&
                ((asset->type == trekker::movie_editor::MediaType::Audio) ==
                    (tracks[lane].type() == timeline::TrackType::Audio)));
            const int lane_y = l.tracks_y0 + ((drop_y - l.tracks_y0) / l.lane_h) * l.lane_h;
            const auto position = x_to_time_us(l, drop_x);
            const auto remaining = std::numeric_limits<std::int64_t>::max() - position;
            const auto end = position + std::min(asset->duration_us, remaining);
            canvas.draw_rect(drop_x, lane_y + 3, std::max(2, time_to_x(l, end) - drop_x),
                std::min(l.lane_h - 6, l.tracks_y1 - lane_y - 3),
                valid ? Colors::Green : Colors::Red, false);
        }
        const int x = std::clamp(media_drag_->x, 0, std::max(0, l.width - 200));
        const int y = std::clamp(media_drag_->y, 0, std::max(0, l.status_y0 - 34));
        canvas.draw_rect(x, y, 200, 32, button_color, true);
        canvas.draw_text(x + 8, y + 4, "Drop media on a track", Colors::White, 1, 184);
    }

    // Playhead (drawn last so it's on top).
    const int playhead_x = time_to_x(l, playhead_us_);
    canvas.draw_line(playhead_x, l.ruler_y0, playhead_x, l.tracks_y1, Colors::Red, 2);

    // Status bar.
    canvas.draw_rect(0, l.status_y0, l.width, l.height - l.status_y0, toolbar_color, true);
    canvas.draw_text(6, l.status_y0 + 5, status_message_, Colors::White, 1);

    // Open topbar dropdown (drawn absolutely last so it overlays everything
    // else, like a real menu would).
    if (open_menu_) {
        const auto defs = menu_definitions();
        const auto items = dropdown_item_rects(l, *open_menu_);
        if (*open_menu_ < defs.size() && !items.empty()) {
            const auto& item_defs = defs[*open_menu_].items;
            const int panel_x0 = items.front().x0;
            const int panel_x1 = items.front().x1;
            const int panel_y0 = items.front().y0;
            const int panel_y1 = items.back().y1;
            canvas.draw_rect(panel_x0, panel_y0, panel_x1 - panel_x0, panel_y1 - panel_y0, Colors::Gray, true);
            canvas.draw_rect(panel_x0, panel_y0, panel_x1 - panel_x0, panel_y1 - panel_y0, Colors::Black, false);
            for (std::size_t i = 0; i < items.size() && i < item_defs.size(); ++i) {
                const auto& r = items[i];
                if (item_defs[i].divider) {
                    const int mid_y = (r.y0 + r.y1) / 2;
                    canvas.draw_line(r.x0 + 4, mid_y, r.x1 - 4, mid_y, Colors::DarkGray, 1);
                } else {
                    canvas.draw_text(r.x0 + 8, r.y0 + (r.y1 - r.y0 - 16) / 2, item_defs[i].label, Colors::Black, 2);
                }
            }
        }
    }

    if (track_context_menu_) {
        const auto& menu = *track_context_menu_;
        const auto* track = project_.timeline().track(menu.track_id);
        canvas.draw_rect(menu.x, menu.y, 220, 36, button_color, true);
        canvas.draw_rect(menu.x, menu.y, 220, 36, Colors::Gray, false);
        canvas.draw_text(menu.x + 10, menu.y + 7, track && track->locked() ?
            "Track is locked" : "Delete Track", Colors::White, 1, 200);
    }
    if (text_editor_) {
        auto& editor = *text_editor_;
        const int left = (cw - 900) / 2, top = (ch - 570) / 2;
        canvas.draw_rect(left, top, 900, 570, toolbar_color, true);
        canvas.draw_rect(left, top, 900, 570, Colors::LightGray, false);
        canvas.draw_text(left + 18, top + 12, editor.edit ? "Edit Text / Title" :
            "Add Text / Title", Colors::White, 2);
        const char* labels[] = {"Text", "Duration (sec)", "Font size (720p)", "Text #RRGGBB",
            "Background hex", "Horizontal %", "Vertical %", "Fade in (sec)", "Fade out (sec)", "Background %"};
        for (std::size_t i = 0; i < editor.fields.size(); ++i) {
            const int y = top + 52 + static_cast<int>(i) * 36;
            canvas.draw_text(left + 16, y + 6, labels[i], Colors::LightGray, 1, 140);
            canvas.draw_rect(left + 160, y, 250, 32, panel_color, true);
            canvas.draw_rect(left + 160, y, 250, 32, i == editor.active ? Colors::Cyan : Colors::Gray, false);
            std::string display = editor.fields[i];
            std::replace(display.begin(), display.end(), '\n', '|');
            canvas.draw_text(left + 168, y + 5, display + (i == editor.active ? "_" : ""), Colors::White, 1, 234);
        }
        canvas.draw_rect(left + 16, top + 420, 394, 32, button_color, true);
        canvas.draw_text(left + 24, top + 425, editor.background ?
            "Background: Color (click for transparent)" : "Background: Transparent (click for color)",
            Colors::White, 1, 380);
        const int px = left + 430, py = top + 52;
        try {
            const auto style = draft_text_style();
            if (editor.drag != 2) {
                const auto rgb = editor.picker_field == 3 ? style.color : style.background_color;
                const auto alpha = static_cast<unsigned char>(std::lround(
                    (editor.picker_field == 3 ? style.opacity : style.background_opacity) * 255));
                const Color selected{static_cast<unsigned char>(rgb >> 16),
                    static_cast<unsigned char>(rgb >> 8), static_cast<unsigned char>(rgb), alpha};
                const auto current = editor.picker.color();
                if (selected.r != current.r || selected.g != current.g || selected.b != current.b || selected.a != current.a)
                    editor.picker.set_color(selected);
            }
            const auto frame = project_.render_text_preview(style, editor.preview_us, 450, 253,
                editor.edit ? editor.track_id : 0, editor.edit ? editor.clip_id : 0);
            const auto& plane = frame.planes[0];
            for (int y = 0; y < 253; ++y) for (int x = 0; x < 450; ++x)
                canvas.set_pixel(px + x, py + y,
                    {plane.at(x * 3, y), plane.at(x * 3 + 1, y), plane.at(x * 3 + 2, y), 255});
            const auto bounds = trekker::movie_editor::text_layout(style, 450, 253);
            canvas.draw_rect(px + bounds.x, py + bounds.y,
                std::max(1, std::min(bounds.width, 450 - bounds.x)),
                std::max(1, std::min(bounds.height, 253 - bounds.y)), Colors::Cyan, false);
        } catch (const std::exception& error) {
            canvas.draw_rect(px, py, 450, 253, panel_color, true);
            canvas.draw_text(px + 10, py + 12, error.what(), Colors::Red, 1, 430);
        }
        canvas.draw_text(px, top + 310, editor.picker_field == 3 ?
            "Text color: hue, saturation/value, opacity" : "Background color and opacity",
            Colors::LightGray, 1, 450);
        editor.picker.draw(canvas, px, top + 334, 450, 180);
        canvas.draw_text(left + 16, top + 464, "Drag text in preview. Click color fields for picker.", Colors::LightGray, 1, 394);
        canvas.draw_text(left + 16, top + 490, editor.error.empty() ?
            "Tab: next. Shift+Enter: new line. Esc: cancel." : editor.error,
            editor.error.empty() ? Colors::LightGray : Colors::Red, 1, 394);
        canvas.draw_rect(left + 620, top + 520, 120, 40, button_color, true);
        canvas.draw_rect(left + 750, top + 520, 130, 40, Color{45,107,211,255}, true);
        canvas.draw_text(left + 635, top + 528, "Cancel", Colors::White, 1);
        canvas.draw_text(left + 770, top + 528, "Apply", Colors::White, 1);
    }
    window_.present_canvas(canvas);
}

void MovieEditorWindow::configure_window() {
    native_menus_ = window_.has_native_menus();
    if (native_menus_) {
        menu_bar_ = std::make_unique<graphics::os_generic::NativeMenuBar>(window_);
        std::vector<graphics::os_generic::Menu> menus;
        const auto definitions = menu_definitions();
        const std::vector<std::vector<std::string>> ids = {
            {"file.open", "file.save-as", "", "file.import-sequence", "file.import-video",
             "file.import-image", "file.import-audio", "", "file.export-avi", "file.export-gif",
             "file.export-wav", "", "file.exit", "file.export-mp4"},
            {"edit.add-video-track", "edit.add-audio-track", "", "edit.split", "edit.delete-clip", "",
             "edit.undo", "edit.redo", "edit.delete-track", "edit.add-text", "edit.edit-text"},
            {"help.about", "help.shortcuts"}
        };
        for (std::size_t m = 0; m < definitions.size(); ++m) {
            const auto& definition = definitions[m];
            graphics::os_generic::Menu menu{"&" + definition.title, {}};
            for (std::size_t i = 0; i < definition.items.size(); ++i) {
                const auto& item = definition.items[i];
                if (item.divider) menu.items.push_back(graphics::os_generic::MenuAction::divider());
                else menu.items.push_back({ids.at(m).at(i), item.label, "", [this, m, i] {
                    try { handle_menu_command(m, i); }
                    catch (const std::exception& error) { pause_playback(); report_error(error.what()); }
                }});
            }
            menus.push_back(std::move(menu));
        }
        menu_bar_->set_menus(std::move(menus));
    }
    RenderHooks hooks;
    hooks.on_render = [this](const RenderEvent&) { render_scene(); };
    hooks.on_key = [this](const graphics::full_application_window::KeyEvent& event) { handle_key(event); };
    hooks.on_text_input = [this](const std::string& text) { handle_text_input(text); };
    hooks.on_pointer_press = [this](const graphics::full_application_window::PointerPressEvent& event) {
        handle_pointer(event, false);
    };
    hooks.on_pointer_release = [this](const graphics::full_application_window::PointerPressEvent& event) {
        handle_pointer(event, true);
    };
    hooks.on_pointer_move = [this](const graphics::full_application_window::PointerPressEvent& event) {
        if (text_editor_) { text_editor_pointer(event.x, event.y, false, false); return; }
        try { update_drag(event.x, event.y, false); }
        catch (const std::exception& error) {
            drag_.reset();
            media_drag_.reset();
            pause_playback();
            report_error(error.what());
        }
    };
    hooks.on_scroll = [this](const graphics::full_application_window::ScrollEvent& event) {
        if (text_editor_) return;
        int width = 0, height = 0;
        window_.client_size(width, height);
        const auto layout = compute_layout(width, height);
        const int direction = event.delta > 0 ? -1 : 1;
        if (event.x < layout.media_x1 && event.y >= layout.media_y0 && event.y < layout.media_y1) {
            const int count = static_cast<int>(project_.media_bin().assets().size());
            const int visible = std::max(1, (layout.media_y1 - layout.media_y0 - 68) / 48);
            media_scroll_ = std::clamp(media_scroll_ + direction, 0, std::max(0, count - visible));
        } else if (event.y >= layout.tracks_y0 && event.y < layout.tracks_y1) {
            if (event.shift) {
                view_start_us_ = std::clamp(view_start_us_ + direction * visible_duration_us_ / 10,
                    std::int64_t{0}, std::max<std::int64_t>(0, project_.duration_us() - visible_duration_us_));
                return;
            }
            const int count = static_cast<int>(project_.timeline().tracks().size());
            const int visible = std::max(1, (layout.tracks_y1 - layout.tracks_y0) / layout.lane_h);
            track_scroll_ = std::clamp(track_scroll_ + direction, 0, std::max(0, count - visible));
        }
    };
    window_.set_render_hooks(std::move(hooks));
}

void MovieEditorWindow::process_pointer() {
    PointerState pointer{};
    if (!window_.query_pointer_state(pointer)) return;
    try {
        if (!pointer.focused) {
            drag_.reset(); media_drag_.reset();
            if (text_editor_) text_editor_->drag = 0;
        }
        if (drag_ || media_drag_) update_drag(pointer.x, pointer.y, !pointer.left_button_down);
    } catch (const std::exception& error) {
        drag_.reset();
        media_drag_.reset();
        pause_playback();
        report_error(error.what());
    }
}

bool MovieEditorWindow::run() {
    if (project_.duration_us() > 0) {
        visible_duration_us_ = std::max<std::int64_t>(10'000'000, project_.duration_us() + project_.duration_us() / 10);
    }
    if (!window_.create()) {
        std::cerr << "Failed to create movie editor window (backend: " << window_.backend_name() << ")\n";
        return false;
    }

    // On Linux, create() can "succeed" while silently falling back to a
    // no-op Headless backend (e.g. when full_application_window was built
    // without the X11 development headers installed). That would otherwise
    // manifest as "the app runs but the window shows nothing and never
    // responds to clicks" — detect it up front and fail loudly instead.
    if (window_.backend() == graphics::full_application_window::Backend::Headless) {
        std::cerr <<
            "\n"
            "Error: no native GUI backend is available on this system, so the movie\n"
            "editor window cannot actually be displayed (it would silently run with no\n"
            "visible content and no mouse input).\n"
            "\n"
            "On Linux this almost always means the X11 development headers were not\n"
            "installed when CoolBox's full_application_window library was built:\n"
            "  Debian/Ubuntu: sudo apt-get install libx11-dev\n"
            "  Fedora/RHEL:   sudo dnf install libX11-devel\n"
            "After installing, delete build/CMakeCache.txt (or re-run cmake's configure\n"
            "step) and rebuild full_application_window and movie_editor.\n"
            "\n"
            "For headless rendering/export without a window, use:\n"
            "  movie_editor --cli --project <file.json> --export-avi/--export-gif/--export-wav ...\n"
            "\n";
        window_.close();
        return false;
    }

    window_.show();
    window_.request_redraw();

    while (window_.is_open()) {
        if (!window_.pump_events()) break;
        finish_import();
        finish_export();

        if (playing_) {
            try {
                audio_playback_.check_error();
                playhead_us_ = audio_playback_.position_us();
                if (playhead_us_ < view_start_us_ || playhead_us_ > view_start_us_ + visible_duration_us_)
                    view_start_us_ = std::max<std::int64_t>(0, playhead_us_ - visible_duration_us_ / 3);
                if (audio_playback_.finished()) { pause_playback(); playhead_us_ = project_.duration_us(); }
            } catch (const std::exception& error) { pause_playback(); report_error(error.what()); }
        }

        process_pointer();

        window_.request_redraw();
        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }
    pause_playback();
    if (video_import_.valid()) { video_import_.wait(); finish_import(); }
    if (export_job_.valid()) { export_job_.wait(); finish_export(); }
    return true;
}

} // namespace movie_editor_app
