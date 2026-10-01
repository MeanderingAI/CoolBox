#include "editor_window.hpp"

#include "graphics.h"
#include "os_dialog.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <thread>

namespace movie_editor_app {

using namespace trekker; // resolves timeline::/video::/audio:: nested namespaces
using graphics::Color;
namespace Colors = graphics::Colors;

namespace {
namespace fs = std::filesystem;
using graphics::full_application_window::FullApplicationWindow;
using graphics::full_application_window::PointerState;
using graphics::full_application_window::RenderEvent;
using graphics::full_application_window::RenderHooks;
using graphics::full_application_window::WindowConfig;

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

MovieEditorWindow::MovieEditorWindow(trekker::movie_editor::EditorProject project)
    : project_(std::move(project)),
      window_(WindowConfig("CoolBox Movie Editor", 1280, 800)) {}

MovieEditorWindow::Layout MovieEditorWindow::compute_layout(int width, int height) const {
    Layout l;
    l.width = width;
    l.height = height;

    const int top_h = static_cast<int>(height * 0.55);
    l.media_x0 = 0;
    l.media_x1 = static_cast<int>(width * 0.18);
    l.media_y0 = 28; // below a thin title strip
    l.media_y1 = top_h;

    l.inspector_x1 = width;
    l.inspector_x0 = width - static_cast<int>(width * 0.20);
    l.inspector_y0 = 28;
    l.inspector_y1 = top_h;

    l.preview_x0 = l.media_x1;
    l.preview_x1 = l.inspector_x0;
    l.preview_y0 = 28;
    l.transport_y0 = top_h - 72; // two rows of buttons
    l.transport_y1 = top_h;
    l.preview_y1 = l.transport_y0;

    l.ruler_y0 = top_h;
    l.ruler_y1 = top_h + 26;
    l.tracks_y0 = l.ruler_y1;
    l.status_y0 = height - 26;
    l.tracks_y1 = l.status_y0;

    return l;
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
    return buttons;
}

std::int64_t MovieEditorWindow::x_to_time_us(const Layout& l, int x) const {
    const int usable_w = std::max(1, (l.width - l.track_header_w));
    const double frac = std::clamp(static_cast<double>(x - l.track_header_w) / usable_w, 0.0, 1.0);
    return static_cast<std::int64_t>(frac * visible_duration_us_);
}

int MovieEditorWindow::time_to_x(const Layout& l, std::int64_t time_us) const {
    const int usable_w = std::max(1, (l.width - l.track_header_w));
    const double frac = std::clamp(static_cast<double>(time_us) / static_cast<double>(visible_duration_us_), 0.0, 1.0);
    return l.track_header_w + static_cast<int>(frac * usable_w);
}

void MovieEditorWindow::set_status(const std::string& message) {
    status_message_ = message;
}

void MovieEditorWindow::build_menu() {
    graphics::components::MenuModel file_menu("File");
    file_menu.add_item(graphics::components::MenuItem::action("Open Project..."));
    file_menu.add_item(graphics::components::MenuItem::action("Save Project As..."));
    file_menu.add_item(graphics::components::MenuItem::divider());
    file_menu.add_item(graphics::components::MenuItem::action("Import Image Sequence Folder..."));
    file_menu.add_item(graphics::components::MenuItem::action("Import Still Image..."));
    file_menu.add_item(graphics::components::MenuItem::action("Import Audio (WAV)..."));
    file_menu.add_item(graphics::components::MenuItem::divider());
    file_menu.add_item(graphics::components::MenuItem::action("Export AVI..."));
    file_menu.add_item(graphics::components::MenuItem::action("Export GIF..."));
    file_menu.add_item(graphics::components::MenuItem::action("Export Audio (WAV)..."));
    file_menu.add_item(graphics::components::MenuItem::divider());
    file_menu.add_item(graphics::components::MenuItem::action("Exit"));

    graphics::components::MenuModel edit_menu("Edit");
    edit_menu.add_item(graphics::components::MenuItem::action("Add Video Track"));
    edit_menu.add_item(graphics::components::MenuItem::action("Add Audio Track"));
    edit_menu.add_item(graphics::components::MenuItem::divider());
    edit_menu.add_item(graphics::components::MenuItem::action("Split Selected Track At Playhead"));
    edit_menu.add_item(graphics::components::MenuItem::action("Delete Selected Clip"));

    graphics::components::MenuBarModel menu_bar;
    menu_bar.add_menu(file_menu);
    menu_bar.add_menu(edit_menu);
    window_.set_menu_bar(menu_bar);

    window_.set_menu_command_handler([this](std::size_t menu_index, std::size_t item_index,
                                            const graphics::components::MenuModel&,
                                            const graphics::components::MenuItem&) {
        handle_menu_command(menu_index, item_index);
    });
}

void MovieEditorWindow::handle_menu_command(std::size_t menu_index, std::size_t item_index) {
    if (menu_index == 0) { // File
        switch (item_index) {
            case 0: do_open_project(); return;
            case 1: do_save_project(); return;
            case 2: return; // divider
            case 3: do_import_image_sequence(); return;
            case 4: do_import_still_image(); return;
            case 5: do_import_audio(); return;
            case 6: return; // divider
            case 7: do_export_avi(); return;
            case 8: do_export_gif(); return;
            case 9: do_export_wav(); return;
            case 10: return; // divider
            case 11: window_.close(); return;
            default: return;
        }
    } else if (menu_index == 1) { // Edit
        switch (item_index) {
            case 0: project_.add_video_track("Video"); set_status("Added video track."); return;
            case 1: project_.add_audio_track("Audio"); set_status("Added audio track."); return;
            case 2: return; // divider
            case 3:
                if (selected_track_id_) {
                    project_.split_clip(*selected_track_id_, playhead_us_);
                    set_status("Split at " + format_timecode(playhead_us_));
                } else {
                    set_status("Select a clip/track first.");
                }
                return;
            case 4:
                if (selected_track_id_ && selected_clip_id_) {
                    project_.delete_clip(*selected_track_id_, *selected_clip_id_);
                    selected_clip_id_.reset();
                    set_status("Deleted clip.");
                } else {
                    set_status("Select a clip first.");
                }
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
        set_status("Imported image sequence: " + key);
    } catch (const std::exception& e) {
        set_status(std::string("Import failed: ") + e.what());
    }
}

void MovieEditorWindow::do_import_still_image() {
    const std::string path = pick_path(app_builder::os_generics::DialogAction::OpenFile, "Select Image",
                                       {{"Images", {"*.png", "*.jpg", "*.jpeg", "*.bmp"}}, {"All Files", {"*.*"}}});
    if (path.empty()) return;
    try {
        const std::string key = project_.media_bin().import_still_image(path);
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

void MovieEditorWindow::handle_click(int x, int y) {
    int cw = 1280, ch = 800;
    window_.client_size(cw, ch);
    const Layout l = compute_layout(cw, ch);

    // Media bin rows.
    if (x >= l.media_x0 && x < l.media_x1 && y >= l.media_y0 && y < l.media_y1) {
        const int row_h = 26;
        const int row = (y - l.media_y0 - 22) / row_h; // 22px reserved for the "Media Bin" header
        const auto& assets = project_.media_bin().assets();
        if (row >= 0 && static_cast<std::size_t>(row) < assets.size()) {
            pending_media_key_ = assets[static_cast<std::size_t>(row)].path;
            set_status("Selected media: " + *pending_media_key_ + " (click a track to place it)");
        }
        return;
    }

    // Transport buttons.
    if (y >= l.transport_y0 && y < l.transport_y1) {
        for (const auto& button : transport_buttons(l)) {
            if (x >= button.x0 && x < button.x1 && y >= button.y0 && y < button.y1) {
                switch (button.id) {
                    case TransportButton::GoToStart: playhead_us_ = 0; break;
                    case TransportButton::PlayPause: playing_ = !playing_; last_tick_ = std::chrono::steady_clock::now(); break;
                    case TransportButton::Stop: playing_ = false; playhead_us_ = 0; break;
                    case TransportButton::GoToEnd: playhead_us_ = project_.duration_us(); break;
                    case TransportButton::Split:
                        if (selected_track_id_) project_.split_clip(*selected_track_id_, playhead_us_);
                        break;
                    case TransportButton::Delete:
                        if (selected_track_id_ && selected_clip_id_) {
                            project_.delete_clip(*selected_track_id_, *selected_clip_id_);
                            selected_clip_id_.reset();
                        }
                        break;
                    case TransportButton::AddVideoTrack: project_.add_video_track("Video"); break;
                    case TransportButton::AddAudioTrack: project_.add_audio_track("Audio"); break;
                }
                return;
            }
        }
        return;
    }

    // Ruler: scrub playhead.
    if (y >= l.ruler_y0 && y < l.ruler_y1) {
        playhead_us_ = x_to_time_us(l, x);
        return;
    }

    // Track lanes.
    if (y >= l.tracks_y0 && y < l.tracks_y1) {
        const int lane_index = (y - l.tracks_y0) / l.lane_h;
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
        if (clip) {
            selected_track_id_ = track.id();
            selected_clip_id_ = clip->id;
            set_status("Selected clip on track " + track.name());
        } else if (pending_media_key_) {
            project_.add_clip(track.id(), *pending_media_key_, clicked_time);
            selected_track_id_ = track.id();
            set_status("Placed clip on track " + track.name());
            pending_media_key_.reset();
        }
        return;
    }
}

void MovieEditorWindow::render_scene() {
    int cw = 1280, ch = 800;
    window_.client_size(cw, ch);
    cw = std::max(600, cw);
    ch = std::max(400, ch);
    const Layout l = compute_layout(cw, ch);

    // A single unified dark background avoids a mismatched "leftover" colour
    // showing through in any area not explicitly covered by a panel below.
    graphics::Canvas canvas(cw, ch, Colors::Black);

    // Title strip.
    canvas.draw_rect(0, 0, l.width, l.media_y0, Colors::DarkGray, true);
    canvas.draw_text(8, 6, "CoolBox Movie Editor", Colors::White, 2);

    // Media bin.
    canvas.draw_rect(l.media_x0, l.media_y0, l.media_x1 - l.media_x0, l.media_y1 - l.media_y0, Colors::Black, true);
    canvas.draw_text(l.media_x0 + 6, l.media_y0 + 6, "MEDIA BIN", Colors::Cyan, 2);
    canvas.draw_line(l.media_x0, l.media_y0 + 22, l.media_x1, l.media_y0 + 22, Colors::DarkGray, 1);
    {
        int row_y = l.media_y0 + 30;
        const int row_h = 26;
        for (const auto& asset : project_.media_bin().assets()) {
            const bool selected = pending_media_key_ && *pending_media_key_ == asset.path;
            if (selected) canvas.draw_rect(l.media_x0, row_y - 3, l.media_x1 - l.media_x0, row_h, Colors::Blue, true);
            const char* type_tag = asset.type == trekker::movie_editor::MediaType::ImageSequence ? "[SEQ] "
                : asset.type == trekker::movie_editor::MediaType::StillImage ? "[IMG] " : "[WAV] ";
            std::string label = type_tag + fs::path(asset.path).filename().string();
            canvas.draw_text(l.media_x0 + 6, row_y, label, selected ? Colors::White : Colors::LightGray, 1);
            row_y += row_h;
            if (row_y > l.media_y1) break;
        }
    }
    canvas.draw_line(l.media_x1, 0, l.media_x1, l.tracks_y1, Colors::DarkGray, 1);

    // Preview.
    canvas.draw_rect(l.preview_x0, l.preview_y0, l.preview_x1 - l.preview_x0, l.preview_y1 - l.preview_y0, Colors::Black, true);
    {
        const trekker::video::VideoFrame frame = project_.render_frame_at(playhead_us_, kPreviewWidth, kPreviewHeight);
        const int avail_w = l.preview_x1 - l.preview_x0 - 20;
        const int avail_h = l.preview_y1 - l.preview_y0 - 30;
        const double scale = std::min(static_cast<double>(avail_w) / kPreviewWidth, static_cast<double>(avail_h) / kPreviewHeight);
        const int draw_w = std::max(1, static_cast<int>(kPreviewWidth * scale));
        const int draw_h = std::max(1, static_cast<int>(kPreviewHeight * scale));
        const int off_x = l.preview_x0 + (l.preview_x1 - l.preview_x0 - draw_w) / 2;
        const int off_y = l.preview_y0 + (l.preview_y1 - l.preview_y0 - 20 - draw_h) / 2;
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
        canvas.draw_text(l.preview_x0 + 6, l.preview_y1 - 18, format_timecode(playhead_us_) + " / " + format_timecode(project_.duration_us()), Colors::White, 2);
    }

    // Transport buttons (two rows of four).
    for (const auto& button : transport_buttons(l)) {
        canvas.draw_rounded_rect(button.x0, button.y0, button.x1 - button.x0, button.y1 - button.y0, 5, Colors::LightGray, true);
        canvas.draw_text(button.x0 + 8, button.y0 + (button.y1 - button.y0 - 14) / 2, button.label, Colors::Black, 2);
    }

    // Inspector.
    canvas.draw_rect(l.inspector_x0, l.inspector_y0, l.inspector_x1 - l.inspector_x0, l.inspector_y1 - l.inspector_y0, Colors::Black, true);
    canvas.draw_line(l.inspector_x0, 0, l.inspector_x0, l.tracks_y1, Colors::DarkGray, 1);
    canvas.draw_text(l.inspector_x0 + 6, l.inspector_y0 + 6, "INSPECTOR", Colors::Cyan, 2);
    canvas.draw_line(l.inspector_x0, l.inspector_y0 + 22, l.inspector_x1, l.inspector_y0 + 22, Colors::DarkGray, 1);
    {
        int row_y = l.inspector_y0 + 32;
        const int row_h = 22;
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
                    canvas.draw_text(l.inspector_x0 + 6, row_y, "src: " + fs::path(clip.source_path).filename().string(), Colors::LightGray, 1); row_y += row_h;
                    canvas.draw_text(l.inspector_x0 + 6, row_y, "pos: " + format_timecode(clip.position_us), Colors::LightGray, 1); row_y += row_h;
                    canvas.draw_text(l.inspector_x0 + 6, row_y, "dur: " + format_timecode(clip.timeline_duration_us()), Colors::LightGray, 1); row_y += row_h;
                    canvas.draw_text(l.inspector_x0 + 6, row_y, "vol: " + std::to_string(clip.volume), Colors::LightGray, 1); row_y += row_h;
                    canvas.draw_text(l.inspector_x0 + 6, row_y, "speed: " + std::to_string(clip.speed), Colors::LightGray, 1); row_y += row_h;
                    break;
                }
            }
        }
    }

    // Timeline ruler.
    canvas.draw_rect(0, l.ruler_y0, l.width, l.ruler_y1 - l.ruler_y0, Colors::DarkGray, true);
    for (std::int64_t t = 0; t <= visible_duration_us_; t += 5'000'000) {
        const int x = time_to_x(l, t);
        canvas.draw_line(x, l.ruler_y0, x, l.ruler_y1, Colors::Gray, 1);
        canvas.draw_text(x + 3, l.ruler_y0 + 5, format_timecode(t), Colors::White, 1);
    }

    // Track lanes: fill the whole area first so there is no mismatched
    // "empty" background showing through when there are zero/few tracks.
    canvas.draw_rect(0, l.tracks_y0, l.width, l.tracks_y1 - l.tracks_y0, Colors::Black, true);
    {
        int lane_y = l.tracks_y0;
        for (const auto& track : project_.timeline().tracks()) {
            const bool track_selected = selected_track_id_ && *selected_track_id_ == track.id();
            canvas.draw_rect(0, lane_y, l.track_header_w, l.lane_h, track_selected ? Colors::Blue : Colors::DarkGray, true);
            canvas.draw_text(6, lane_y + 6, track.name().empty() ? "(track)" : track.name(), Colors::White, 1);
            canvas.draw_text(6, lane_y + 22, track.type() == timeline::TrackType::Video ? "video" : "audio", Colors::LightGray, 1);
            canvas.draw_line(0, lane_y + l.lane_h - 1, l.width, lane_y + l.lane_h - 1, Colors::Gray, 1);

            for (const auto& clip : track.clips()) {
                const int x0 = time_to_x(l, clip.position_us);
                const int x1 = time_to_x(l, clip.timeline_end_us());
                const bool clip_selected = selected_clip_id_ && *selected_clip_id_ == clip.id;
                const Color clip_color = track.type() == timeline::TrackType::Video ? Colors::Purple : Colors::Green;
                canvas.draw_rect(x0, lane_y + 3, std::max(1, x1 - x0), l.lane_h - 6, clip_color, true);
                if (clip_selected) canvas.draw_rect(x0, lane_y + 3, std::max(1, x1 - x0), l.lane_h - 6, Colors::White, false);
                canvas.draw_text(x0 + 3, lane_y + 6, fs::path(clip.source_path).filename().string(), Colors::White, 1);
            }
            lane_y += l.lane_h;
            if (lane_y > l.tracks_y1) break;
        }
    }
    canvas.draw_line(l.track_header_w, l.tracks_y0, l.track_header_w, l.tracks_y1, Colors::Gray, 1);

    // Playhead (drawn last so it's on top).
    const int playhead_x = time_to_x(l, playhead_us_);
    canvas.draw_line(playhead_x, l.ruler_y0, playhead_x, l.tracks_y1, Colors::Red, 2);

    // Status bar.
    canvas.draw_rect(0, l.status_y0, l.width, l.height - l.status_y0, Colors::DarkGray, true);
    canvas.draw_text(6, l.status_y0 + 5, status_message_, Colors::White, 1);

    window_.present_canvas(canvas);
}

bool MovieEditorWindow::run() {
    build_menu();

    RenderHooks hooks;
    hooks.on_render = [this](const RenderEvent&) { render_scene(); };
    window_.set_render_hooks(std::move(hooks));

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

    bool prev_left_down = false;
    last_tick_ = std::chrono::steady_clock::now();

    while (window_.is_open()) {
        if (!window_.pump_events()) break;

        const auto now = std::chrono::steady_clock::now();
        const double dt_s = std::chrono::duration<double>(now - last_tick_).count();
        last_tick_ = now;
        if (playing_) {
            playhead_us_ += static_cast<std::int64_t>(dt_s * 1'000'000.0);
            const std::int64_t dur = project_.duration_us();
            if (dur > 0 && playhead_us_ >= dur) {
                playhead_us_ = dur;
                playing_ = false;
            }
        }

        PointerState pointer{};
        if (window_.query_pointer_state(pointer)) {
            const bool left_pressed = pointer.left_button_down && !prev_left_down;
            prev_left_down = pointer.left_button_down;
            if (left_pressed && pointer.inside) {
                handle_click(pointer.x, pointer.y);
            }
        }

        window_.request_redraw();
        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }
    return true;
}

} // namespace movie_editor_app
