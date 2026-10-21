#include "editor_window.hpp"
#include "tyst_framework.hpp"
#include <filesystem>
#include <chrono>
#include <fstream>
#include <thread>
#include <atomic>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

namespace movie_editor_app {
struct MovieEditorWindowTestAccess {
    static void layout(MovieEditorWindow& editor) {
        trekker::movie_editor::TextStyle title;
        title.text = "A long title\nwith another line";
        editor.selected_clip_id_ = editor.project_.add_text_clip(title, 0, 3'000'000);
        editor.selected_track_id_ = editor.project_.timeline().tracks().back().id();
        for (int font = 0; font < 2; ++font) {
            if (font) editor.ui_font_.clear();
            for (const bool native : {true, false}) {
                editor.native_menus_ = native;
                for (const auto& size : std::vector<std::pair<int, int>>{{960, 640}, {1280, 800}, {1920, 1080}}) {
                    const auto l = editor.compute_layout(size.first, size.second);
                    TYST_EXPECT_EQ(l.preview_x1, size.first);
                    TYST_EXPECT_EQ(l.preview_x0, l.media_x1);
                    TYST_EXPECT_GE(l.preview_x1 - l.preview_x0, size.first / 2);
                    TYST_EXPECT_LT(l.preview_y0 + 52, l.preview_y1);
                    TYST_EXPECT_EQ((l.tracks_y1 - l.tracks_y0) % l.lane_h, 0);
                    const auto buttons = editor.transport_buttons(l);
                    bool edit_title = false;
                    for (std::size_t i = 0; i < buttons.size(); ++i) {
                        const auto& a = buttons[i];
                        TYST_EXPECT_GE(a.x0, 0);
                        TYST_EXPECT_LE(a.x1, l.width);
                        TYST_EXPECT_GE(a.y0, l.menu_y1);
                        TYST_EXPECT_LE(a.y1, l.ruler_y0);
                        TYST_EXPECT_GE(a.x1 - a.x0, editor.text_width(a.label, 1) + 24);
                        if (a.id == MovieEditorWindow::TransportButton::EditText) edit_title = true;
                        for (std::size_t j = i + 1; j < buttons.size(); ++j) {
                            const auto& b = buttons[j];
                            TYST_EXPECT_FALSE(a.x0 < b.x1 && b.x0 < a.x1 && a.y0 < b.y1 && b.y0 < a.y1);
                        }
                    }
                    TYST_EXPECT_TRUE(edit_title);
                    for (const auto& menu : editor.topbar_buttons(l)) TYST_EXPECT_LE(menu.y1, l.menu_y1);
                }
            }
        }
        editor.selected_clip_id_.reset();
        for (const auto& button : editor.transport_buttons(editor.compute_layout(960, 640)))
            TYST_EXPECT_NE(button.id, MovieEditorWindow::TransportButton::EditText);
    }

    static void media_previews(MovieEditorWindow& editor) {
        if (!editor.window_.create()) TYST_SKIP() << "A native display is required for linked audio card tests.";
        const auto path = std::filesystem::temp_directory_path() /
            ("coolbox_ui_waveform_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".wav");
        auto buffer = trekker::audio::AudioBuffer::silent(1024, 2, 1024);
        buffer.planes[0][0] = 0.25f;
        buffer.planes[1][512] = -0.75f;
        buffer.planes[0][1023] = 1.0f;
        trekker::audio::container::write_wav(path.string(), buffer);
        trekker::movie_editor::MediaAsset asset;
        asset.path = "video-with-linked-audio";
        asset.type = trekker::movie_editor::MediaType::ImportedVideo;
        asset.duration_us = 1'000'000;
        asset.audio_path = path.string();
        auto frame = trekker::video::VideoFrame::blank(trekker::video::PixelFormat::RGB24, 2, 1);
        frame.planes[0].at(0, 0) = 255;
        frame.planes[0].at(5, 0) = 255;
        asset.decoded_frames.push_back(frame);
        TYST_EXPECT_TRUE(editor.media_visual(asset) == nullptr);
        TYST_ASSERT_TRUE(editor.visual_job_.valid());
        editor.visual_job_.wait();
        editor.finish_media_visual();
        const auto* visual = editor.media_visual(asset);
        TYST_ASSERT_TRUE(visual != nullptr);
        TYST_EXPECT_TRUE(visual->error.empty());
        TYST_ASSERT_EQ(visual->peaks.size(), 1024U);
        TYST_EXPECT_EQ(visual->audio_duration_us, 1'000'000);
        TYST_EXPECT_GT(visual->peaks[0], 0.24f);
        TYST_EXPECT_LT(visual->peaks[0], 0.26f);
        TYST_EXPECT_GT(visual->peaks[512], 0.74f);
        TYST_EXPECT_GT(visual->peaks[1023], 0.99f);
        TYST_EXPECT_EQ(visual->peaks[256], 0.0f);
        TYST_ASSERT_EQ(visual->thumbnail.size(), 96U * 54U);
        TYST_EXPECT_EQ(visual->thumbnail.front().r, 255);
        TYST_EXPECT_EQ(visual->thumbnail.front().b, 0);
        TYST_EXPECT_EQ(visual->thumbnail.back().b, 255);
        TYST_EXPECT_EQ(visual->thumbnail.back().r, 0);
        TYST_EXPECT_FALSE(editor.visual_job_.valid());
        const auto video_key = editor.project_.media_bin().add_asset(asset);
        const auto video_track = editor.project_.add_video_track("Story").id();
        const auto video_clip = editor.project_.add_clip(video_track, video_key, 0);

        asset.type = trekker::movie_editor::MediaType::Audio;
        asset.path = path.string();
        asset.audio_path.clear();
        asset.decoded_frames.clear();
        const auto audio = editor.build_media_visual(asset);
        TYST_EXPECT_TRUE(audio.thumbnail.empty());
        TYST_EXPECT_EQ(audio.peaks, visual->peaks);
        const auto audio_key = editor.project_.media_bin().add_asset(asset);
        const auto audio_track = editor.project_.add_audio_track("Music").id();
        editor.project_.add_clip(audio_track, audio_key, 0);
        editor.visible_duration_us_ = 4'000'000;
        editor.render_scene();
        if (editor.visual_job_.valid()) {
            editor.visual_job_.wait();
            editor.finish_media_visual();
        }
        editor.render_scene();
        int width = 0, height = 0;
        editor.window_.client_size(width, height);
        const auto l = editor.compute_layout(width, height);
        const int wave_y = l.tracks_y0 + 70;
        const int body_x = editor.time_to_x(l, 500'000);
        editor.handle_click(body_x, wave_y);
        TYST_ASSERT_TRUE(editor.drag_.has_value());
        TYST_EXPECT_EQ(editor.drag_->kind, DragKind::Move);
        editor.update_drag(body_x + 100, wave_y, true);
        const auto moved = editor.project_.timeline().track(video_track)->clips().front();
        TYST_EXPECT_GT(moved.position_us, 0);
        TYST_EXPECT_EQ(moved.id, video_clip);
        TYST_EXPECT_EQ(editor.project_.media_bin().find(moved.source_path)->audio_path, path.string());
        TYST_EXPECT_EQ(editor.project_.timeline().track(audio_track)->clips().front().position_us, 0);
        editor.render_scene();
        editor.handle_key({'Z', true});
        editor.handle_click(editor.time_to_x(l, 0), wave_y);
        editor.update_drag(editor.time_to_x(l, 250'000), wave_y, true);
        TYST_EXPECT_GT(editor.project_.timeline().track(video_track)->clips().front().in_point_us, 0);
        editor.render_scene();
        editor.handle_key({'Z', true});
        editor.handle_click(body_x, wave_y);
        editor.update_drag(body_x, wave_y, true);
        editor.playhead_us_ = 500'000;
        editor.handle_menu_command(1, 3);
        TYST_EXPECT_EQ(editor.project_.timeline().track(video_track)->clips().size(), 2U);
        for (const auto& clip : editor.project_.timeline().track(video_track)->clips())
            TYST_EXPECT_EQ(editor.project_.media_bin().find(clip.source_path)->audio_path, path.string());
        editor.render_scene();
        editor.window_.close();
        drain_close_message();
        std::filesystem::remove(path);
        const auto missing = editor.build_media_visual(asset);
        TYST_EXPECT_FALSE(missing.error.empty());
        TYST_EXPECT_TRUE(missing.peaks.empty());
    }

    static void drain_close_message() {
#ifdef _WIN32
        MSG quit{};
        PeekMessage(&quit, nullptr, WM_QUIT, WM_QUIT, PM_REMOVE);
#endif
    }

    static void exercise(MovieEditorWindow& editor) {
        if (!editor.window_.create()) TYST_SKIP() << "A native display is required for UI gesture tests.";
        int width = 0, height = 0;
        TYST_ASSERT_TRUE(editor.window_.client_size(width, height));
        const auto layout = editor.compute_layout(width, height);
        editor.visible_duration_us_ = 10'000'000;
        auto& project = editor.project_;
        const auto from = project.timeline().tracks()[0].id();
        const auto to = project.timeline().tracks()[1].id();
        const auto original = project.timeline().track(from)->clips().front();
        const int body_x = editor.time_to_x(layout, 4'000'000);
        const int lane_y = layout.tracks_y0 + layout.lane_h / 2;

        editor.handle_click(body_x, lane_y);
        editor.update_drag(body_x + 100, lane_y, false);
        TYST_EXPECT_EQ(project.timeline().track(from)->clips().front().position_us, original.position_us);
        TYST_ASSERT_TRUE(editor.drag_.has_value());
        editor.update_drag(body_x + 100, lane_y, true);
        TYST_EXPECT_GT(project.timeline().track(from)->clips().front().position_us, original.position_us);
        editor.handle_key({'Z', true});
        TYST_EXPECT_EQ(project.timeline().track(from)->clips().front().position_us, original.position_us);
        editor.handle_key({'Z', true, true});
        TYST_EXPECT_GT(project.timeline().track(from)->clips().front().position_us, original.position_us);
        editor.handle_key({'Z', true});

        editor.handle_click(body_x, lane_y);
        editor.update_drag(body_x, lane_y + layout.lane_h, true);
        TYST_EXPECT_TRUE(project.timeline().track(from)->clips().empty());
        TYST_EXPECT_EQ(project.timeline().track(to)->clips().front().id, original.id);
        editor.handle_key({'Z', true});

        const int left = editor.time_to_x(layout, original.position_us);
        editor.handle_click(left, lane_y);
        editor.update_drag(left + 80, lane_y, true);
        const auto trimmed = project.timeline().track(from)->clips().front();
        TYST_EXPECT_GT(trimmed.in_point_us, original.in_point_us);
        TYST_EXPECT_EQ(trimmed.timeline_end_us(), original.timeline_end_us());
        editor.handle_key({'Z', true});

        editor.handle_click(body_x, lane_y);
        editor.update_drag(body_x + 150, lane_y, false);
        editor.handle_key({static_cast<int>(graphics::full_application_window::Key::Escape)});
        TYST_EXPECT_FALSE(editor.drag_.has_value());
        TYST_EXPECT_EQ(project.timeline().track(from)->clips().front().position_us, original.position_us);

        editor.handle_click(20, layout.media_y0 + 80);
        editor.update_drag(editor.time_to_x(layout, 7'000'000), lane_y + layout.lane_h, true);
        TYST_EXPECT_EQ(project.timeline().track(to)->clips().size(), std::size_t{1});
        editor.handle_key({'Z', true});
        TYST_EXPECT_TRUE(project.timeline().track(to)->clips().empty());

        editor.selected_track_id_ = from;
        editor.selected_clip_id_ = original.id;
        editor.playhead_us_ = 3'000'000;
        editor.handle_key({'B', true});
        TYST_EXPECT_EQ(project.timeline().track(from)->clips().size(), std::size_t{2});
        editor.handle_key({'Z', true});
        editor.handle_click(body_x, lane_y);
        editor.update_drag(body_x, lane_y, true);
        editor.handle_key({static_cast<int>(graphics::full_application_window::Key::Delete)});
        TYST_EXPECT_TRUE(project.timeline().track(from)->clips().empty());
        editor.handle_key({'Z', true});
        editor.handle_key({static_cast<int>(graphics::full_application_window::Key::End)});
        TYST_EXPECT_EQ(editor.playhead_us_, project.duration_us());
        editor.handle_key({static_cast<int>(graphics::full_application_window::Key::Home)});
        TYST_EXPECT_EQ(editor.playhead_us_, 0);
        editor.handle_key({static_cast<int>(graphics::full_application_window::Key::Right)});
        TYST_EXPECT_GT(editor.playhead_us_, 0);
        const auto position = editor.playhead_us_;
        editor.handle_key({static_cast<int>(graphics::full_application_window::Key::Right), false, false, true});
        TYST_EXPECT_EQ(editor.playhead_us_, position);

        const auto compact_layout = editor.compute_layout(960, 640);
        for (const auto& button : editor.transport_buttons(compact_layout)) {
            TYST_EXPECT_LE(button.x1, 960);
            TYST_EXPECT_GT(button.x1, button.x0);
        }
        editor.render_scene();
        const auto saved = std::filesystem::temp_directory_path() /
            ("coolbox_ui_save_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".json");
        editor.last_project_path_ = saved.string();
        editor.handle_key({'S', true});
        TYST_EXPECT_TRUE(std::filesystem::is_regular_file(saved));
        std::ifstream saved_file(saved);
        const std::string contents((std::istreambuf_iterator<char>(saved_file)), std::istreambuf_iterator<char>());
        TYST_EXPECT_NE(contents.find("ui-test-media"), std::string::npos);
        saved_file.close();
        std::filesystem::remove(saved);
        editor.window_.close();
        drain_close_message();
    }

    static void menus(MovieEditorWindow& editor) {
        if (!editor.window_.create()) TYST_SKIP() << "A native display is required for menu tests.";
        int width = 0, height = 0;
        TYST_ASSERT_TRUE(editor.window_.client_size(width, height));
        const auto layout = editor.compute_layout(width, height);
        const auto before = editor.project_.timeline().tracks().size();
        auto select = [&](std::size_t menu, std::size_t item) {
#ifdef _WIN32
            const auto handle = reinterpret_cast<HWND>(editor.window_.native_handle());
            const auto bar = GetMenu(handle);
            TYST_ASSERT_TRUE(bar != nullptr);
            TYST_EXPECT_EQ(GetMenuItemCount(bar), 3);
            TYST_EXPECT_TRUE(editor.topbar_buttons(layout).empty());
            const auto popup = GetSubMenu(bar, static_cast<int>(menu));
            const auto command = GetMenuItemID(popup, static_cast<int>(item));
            TYST_ASSERT_NE(command, static_cast<UINT>(-1));
            PostMessage(handle, WM_COMMAND, command, 0);
            TYST_ASSERT_TRUE(editor.window_.pump_events());
#else
            if (editor.window_.has_native_menus()) {
                const auto& bar = editor.window_.menu_bar();
                TYST_EXPECT_EQ(bar.menus.size(), 3U);
                TYST_EXPECT_TRUE(editor.topbar_buttons(layout).empty());
                editor.window_.menu_command_handler()(menu, item, bar.menus[menu], bar.menus[menu].items[item]);
                return;
            }
            const auto titles = editor.topbar_buttons(layout);
            TYST_ASSERT_TRUE(menu < titles.size());
            editor.window_.render_hooks().on_pointer_press({titles[menu].x0 + 5, titles[menu].y0 + 5});
            const auto choices = editor.dropdown_item_rects(layout, menu);
            editor.window_.render_hooks().on_pointer_press({choices[item].x0 + 8, choices[item].y0 + 8});
#endif
        };
        select(1, 0);
        TYST_EXPECT_EQ(editor.project_.timeline().tracks().size(), before + 1);
        select(1, 6);
        TYST_EXPECT_EQ(editor.project_.timeline().tracks().size(), before);
        select(1, 7);
        TYST_EXPECT_EQ(editor.project_.timeline().tracks().size(), before + 1);
        select(1, 1);
        TYST_EXPECT_EQ(editor.project_.timeline().tracks().back().type(), trekker::timeline::TrackType::Audio);
        const auto count = editor.project_.timeline().tracks().size();
        std::promise<void> completion;
        editor.export_job_ = completion.get_future();
        select(1, 0);
        TYST_EXPECT_EQ(editor.project_.timeline().tracks().size(), count);
        TYST_EXPECT_NE(editor.status_message_.find("Please wait"), std::string::npos);
        completion.set_value();
        editor.finish_export();
        editor.window_.close();
        drain_close_message();
    }

    static void track_context_and_drop(MovieEditorWindow& editor) {
        if (!editor.window_.create()) TYST_SKIP() << "A native display is required for pointer tests.";
        int width = 0, height = 0;
        editor.window_.client_size(width, height);
        const auto layout = editor.compute_layout(width, height);
        auto& project = editor.project_;
        auto press = [&](int x, int y, bool right = false) {
            editor.window_.render_hooks().on_pointer_press({x, y, right ?
                graphics::full_application_window::PointerButton::Right :
                graphics::full_application_window::PointerButton::Left});
        };
        auto release = [&](int x, int y) {
            editor.window_.render_hooks().on_pointer_release({x, y});
        };
        const int lane_y = layout.tracks_y0 + 20;
        const int drop_x = editor.time_to_x(layout, 2'000'000);
        const auto key = project.media_bin().assets().front().path;
        // No frame/poll between press and release: a quick drop must still commit.
#ifdef _WIN32
        const auto handle = reinterpret_cast<HWND>(editor.window_.native_handle());
        PostMessage(handle, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(20, layout.media_y0 + 80));
        PostMessage(handle, WM_LBUTTONUP, 0, MAKELPARAM(drop_x, lane_y));
        TYST_ASSERT_TRUE(editor.window_.pump_events());
#else
        press(20, layout.media_y0 + 80);
        release(drop_x, lane_y);
#endif
        TYST_ASSERT_EQ(project.timeline().tracks().size(), 1U);
        const auto track_id = project.timeline().tracks().front().id();
        TYST_ASSERT_EQ(project.timeline().tracks().front().clips().size(), 1U);
        TYST_EXPECT_EQ(project.timeline().tracks().front().clips().front().source_path, key);
        editor.handle_key({'Z', true});
        TYST_EXPECT_TRUE(project.timeline().tracks().empty());
        editor.handle_key({'Z', true, true});
        press(20, layout.media_y0 + 80);
        editor.window_.render_hooks().on_pointer_move({drop_x + 100, lane_y});
        editor.render_scene();
        release(drop_x + 100, lane_y);
        TYST_ASSERT_EQ(project.timeline().tracks().front().clips().size(), 2U);

        press(20, lane_y, true);
        TYST_ASSERT_TRUE(editor.track_context_menu_.has_value());
        editor.render_scene();
        editor.handle_key({static_cast<int>(graphics::full_application_window::Key::Escape)});
        TYST_EXPECT_FALSE(editor.track_context_menu_.has_value());
        TYST_EXPECT_EQ(project.timeline().tracks().size(), 1U);
        press(20, lane_y, true);
        auto menu = *editor.track_context_menu_;
        press(menu.x + 10, menu.y + 10);
        TYST_EXPECT_TRUE(project.timeline().tracks().empty());
        TYST_EXPECT_FALSE(editor.selected_clip_id_.has_value());
        editor.handle_key({'Z', true});
        TYST_ASSERT_EQ(project.timeline().tracks().size(), 1U);
        TYST_EXPECT_EQ(project.timeline().tracks().front().id(), track_id);
        TYST_EXPECT_EQ(project.timeline().tracks().front().clips().size(), 2U);
        project.timeline().track(track_id)->set_locked(true);
        press(20, lane_y, true);
        menu = *editor.track_context_menu_;
        press(menu.x + 10, menu.y + 10);
        TYST_EXPECT_EQ(project.timeline().tracks().size(), 1U);
        TYST_EXPECT_NE(editor.status_message_.find("locked"), std::string::npos);
        press(20, layout.media_y0 + 80);
        TYST_EXPECT_THROW(editor.update_drag(drop_x, lane_y, true), std::invalid_argument);
        TYST_EXPECT_FALSE(editor.media_drag_.has_value());
        TYST_EXPECT_EQ(project.timeline().tracks().front().clips().size(), 2U);
        project.timeline().track(track_id)->set_locked(false);

        press(20, layout.media_y0 + 80);
        release(-20, lane_y);
        TYST_EXPECT_EQ(project.timeline().tracks().front().clips().size(), 2U);
        TYST_EXPECT_FALSE(editor.media_drag_.has_value());
        press(20, lane_y + layout.lane_h, true);
        TYST_EXPECT_FALSE(editor.track_context_menu_.has_value());
        const auto audio_id = project.add_audio_track("Audio").id();
        press(20, layout.media_y0 + 80);
        TYST_EXPECT_THROW(editor.update_drag(drop_x, lane_y + layout.lane_h, true), std::invalid_argument);
        TYST_EXPECT_TRUE(project.timeline().track(audio_id)->clips().empty());
        int empty_lane_y = lane_y + 2 * layout.lane_h;
        if (empty_lane_y >= layout.tracks_y1) {
            editor.window_.render_hooks().on_scroll({drop_x, lane_y, -120});
            TYST_EXPECT_EQ(editor.track_scroll_, 1);
            empty_lane_y -= layout.lane_h;
        }
        press(20, layout.media_y0 + 80);
        release(drop_x, empty_lane_y);
        TYST_EXPECT_EQ(project.timeline().tracks().size(), 3U);
        editor.handle_key({'Z', true});
        TYST_EXPECT_EQ(project.timeline().tracks().size(), 2U);
        editor.track_scroll_ = 1;
        press(20, lane_y, true);
        TYST_ASSERT_TRUE(editor.track_context_menu_.has_value());
        TYST_EXPECT_EQ(editor.track_context_menu_->track_id, audio_id);
        menu = *editor.track_context_menu_;
        press(menu.x + 10, menu.y + 10);
        TYST_EXPECT_TRUE(project.timeline().track(audio_id) == nullptr);
        TYST_EXPECT_EQ(project.timeline().tracks().size(), 1U);
        editor.handle_key({'Z', true});
        TYST_EXPECT_TRUE(project.timeline().track(audio_id) != nullptr);
        editor.window_.close();
        drain_close_message();
    }

    static void title_editor(MovieEditorWindow& editor) {
        if (!editor.window_.create()) TYST_SKIP() << "A native display is required for title UI tests.";
        editor.playhead_us_ = 2'000'000;
        editor.handle_menu_command(1, 9);
        TYST_ASSERT_TRUE(editor.text_editor_.has_value());
        editor.text_editor_->background = true;
        editor.render_scene();
        const auto type = editor.window_.render_hooks().on_text_input;
#ifdef _WIN32
        const auto handle = reinterpret_cast<HWND>(editor.window_.native_handle());
        for (const auto character : std::string("Opening"))
            PostMessage(handle, WM_CHAR, character, 1);
        TYST_ASSERT_TRUE(editor.window_.pump_events());
#else
        type("Opening");
#endif
        editor.handle_key({13, false, true});
        type("Scene");
        TYST_EXPECT_EQ(editor.text_editor_->fields[0], "Opening\nScene");
        editor.handle_key({9});
        type("4");
        editor.handle_key({9});
        type("72");
        editor.handle_key({9});
        type("FFDD00");
        editor.handle_key({9});
        type("102030");
        editor.handle_key({9});
        type("25");
        editor.handle_key({9});
        type("75");
        editor.handle_key({9});
        type("0.25");
        editor.handle_key({9});
        type("0.75");
        editor.handle_key({13});
        TYST_EXPECT_FALSE(editor.text_editor_.has_value());
        TYST_ASSERT_TRUE(editor.selected_track_id_.has_value());
        const auto track = *editor.selected_track_id_;
        const auto clip = *editor.selected_clip_id_;
        const auto* style = editor.project_.text_style(track, clip);
        TYST_ASSERT_TRUE(style != nullptr);
        TYST_EXPECT_EQ(style->text, "Opening\nScene");
        TYST_EXPECT_TRUE(style->background);
        TYST_EXPECT_EQ(style->font_size, 72);
        TYST_EXPECT_EQ(style->color, 0xffdd00U);
        TYST_EXPECT_EQ(style->background_color, 0x102030U);
        TYST_EXPECT_EQ(style->x, 0.25);
        TYST_EXPECT_EQ(style->y, 0.75);
        TYST_EXPECT_EQ(style->fade_in_us, 250'000);
        TYST_EXPECT_EQ(style->fade_out_us, 750'000);
        const auto original = editor.project_.timeline().track(track)->clips().front();
        TYST_EXPECT_EQ(original.position_us, 2'000'000);
        TYST_EXPECT_EQ(original.timeline_duration_us(), 4'000'000);
        bool clicked_edit = false;
        int control_width = 0, control_height = 0;
        editor.window_.client_size(control_width, control_height);
        for (const auto& button : editor.transport_buttons(editor.compute_layout(control_width, control_height))) {
            if (button.id != MovieEditorWindow::TransportButton::EditText) continue;
            editor.handle_click(button.x0 + 5, button.y0 + 5);
            clicked_edit = true;
            break;
        }
        TYST_EXPECT_TRUE(clicked_edit);
        TYST_ASSERT_TRUE(editor.text_editor_.has_value());
        type("Changed");
        editor.text_editor_->active = 1;
        editor.text_editor_->replace = true;
        type("invalid");
        editor.handle_key({13});
        TYST_ASSERT_TRUE(editor.text_editor_.has_value());
        TYST_EXPECT_FALSE(editor.text_editor_->error.empty());
        TYST_EXPECT_EQ(editor.project_.text_style(track, clip)->text, "Opening\nScene");
        editor.text_editor_->replace = true;
        type("3");
        editor.handle_key({13});
        TYST_EXPECT_EQ(editor.project_.text_style(track, clip)->text, "Changed");
        editor.handle_key({'Z', true});
        TYST_EXPECT_EQ(editor.project_.text_style(track, clip)->text, "Opening\nScene");
        editor.handle_key({'Z', true});
        TYST_EXPECT_TRUE(editor.project_.timeline().tracks().empty());
        editor.handle_menu_command(1, 9);
        TYST_ASSERT_TRUE(editor.text_editor_.has_value());
        TYST_EXPECT_FALSE(editor.text_editor_->background);
        int width = 0, height = 0;
        editor.window_.client_size(width, height);
        const int left = (width - 900) / 2, top = (height - 570) / 2;
        auto press = [&](int x, int y) { editor.window_.render_hooks().on_pointer_press({x, y}); };
        auto move = [&](int x, int y) { editor.window_.render_hooks().on_pointer_move({x, y}); };
        auto release = [&](int x, int y) { editor.window_.render_hooks().on_pointer_release({x, y}); };
        press(left + 450, top + 420);
        move(left + 475, top + 445);
        release(left + 480, top + 450);
        TYST_EXPECT_FALSE(editor.text_editor_->background);
        press(left + 25, top + 430);
        TYST_EXPECT_TRUE(editor.text_editor_->background);
        editor.text_editor_->fields[9] = "50";
        const auto prior = editor.draft_text_style();
        press(left + 655, top + 178);
        TYST_EXPECT_EQ(editor.text_editor_->drag, 1);
        move(left + 855, top + 85);
        release(left + 855, top + 85);
        TYST_EXPECT_EQ(editor.text_editor_->drag, 0);
        const auto positioned = editor.draft_text_style();
        TYST_EXPECT_GT(positioned.x, prior.x);
        TYST_EXPECT_LT(positioned.y, prior.y);
        TYST_EXPECT_EQ(positioned.background_opacity, 0.5);
        TYST_EXPECT_TRUE(editor.project_.timeline().tracks().empty());
        editor.render_scene();
        // Clicking the background color field switches the reusable picker target.
        press(left + 180, top + 52 + 4 * 36 + 5);
        TYST_EXPECT_EQ(editor.text_editor_->picker_field, 4U);
        const auto before_color = editor.text_editor_->fields[4];
        press(left + 440, top + 344);
        move(left + 510, top + 370);
        release(left + 510, top + 370);
        TYST_EXPECT_NE(editor.text_editor_->fields[4], before_color);
        TYST_EXPECT_TRUE(editor.project_.timeline().tracks().empty());
        editor.handle_key({27});
        TYST_EXPECT_FALSE(editor.text_editor_.has_value());
        TYST_EXPECT_TRUE(editor.project_.timeline().tracks().empty());
        editor.handle_menu_command(1, 9);
        editor.text_editor_->background = true;
        editor.text_editor_->fields[9] = "35";
        press(left + 650, top + 178);
        move(left + 820, top + 100);
        release(left + 820, top + 100);
        const auto saved_position = editor.draft_text_style();
        editor.handle_key({13});
        TYST_ASSERT_TRUE(editor.selected_track_id_.has_value());
        const auto* committed = editor.project_.text_style(*editor.selected_track_id_, *editor.selected_clip_id_);
        TYST_ASSERT_TRUE(committed != nullptr);
        TYST_EXPECT_EQ(committed->x, saved_position.x);
        TYST_EXPECT_EQ(committed->y, saved_position.y);
        TYST_EXPECT_EQ(committed->background_opacity, 0.35);
        editor.handle_menu_command(1, 10);
        TYST_ASSERT_TRUE(editor.text_editor_.has_value());
        TYST_EXPECT_EQ(editor.draft_text_style().background_opacity, 0.35);
        editor.handle_key({27});
        editor.handle_key({'Z', true});
        TYST_EXPECT_TRUE(editor.project_.timeline().tracks().empty());
        editor.window_.close();
        drain_close_message();
    }

    static void event_loop(MovieEditorWindow& editor) {
        if (!editor.window_.create()) TYST_SKIP() << "A native display is required for event-loop tests.";
        int ticks = 0;
        std::atomic<bool> stop_worker{false};
        std::atomic<int> observed_ticks{0};
        std::atomic<bool> observed_early_wait{false};
        auto hooks = editor.window_.render_hooks();
        hooks.on_tick = [&](const auto&) {
            ++ticks;
            observed_ticks.store(ticks);
            if (ticks == 1) {
                editor.audio_playback_.start(editor.project_, 0, AudioPlayback::Device::NullForTests);
                editor.playing_ = true;
            }
            if (ticks == 3) {
                TYST_EXPECT_TRUE(editor.playing_);
                editor.export_job_ = std::async(std::launch::async, [&] {
                    while (!stop_worker.load()) std::this_thread::sleep_for(std::chrono::milliseconds(2));
                });
            }
            if (ticks == 6) {
                TYST_EXPECT_TRUE(editor.playing_);
                TYST_EXPECT_TRUE(editor.export_job_.valid());
                TYST_EXPECT_EQ(editor.export_job_.wait_for(std::chrono::seconds(0)), std::future_status::timeout);
                stop_worker.store(true);
                editor.window_.close();
            }
        };
        editor.window_.set_render_hooks(std::move(hooks));
        // Unblock a regressed wait in the event loop so this test fails instead of hanging.
        auto watchdog = std::thread([&] {
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
            while (!stop_worker.load() && std::chrono::steady_clock::now() < deadline)
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            if (!stop_worker.exchange(true)) observed_early_wait.store(observed_ticks.load() < 6);
        });
        TYST_EXPECT_TRUE(editor.run());
        stop_worker.store(true);
        watchdog.join();
        TYST_EXPECT_EQ(ticks, 6);
        TYST_EXPECT_FALSE(observed_early_wait.load());
        TYST_EXPECT_FALSE(editor.playing_);
        TYST_EXPECT_FALSE(editor.export_job_.valid());
        drain_close_message();
    }
};
}

TYST_TEST(MovieEditorUi, WorkspaceControlsFitWithoutOverlapsAtSupportedSizes) {
    movie_editor_app::MovieEditorWindow editor;
    movie_editor_app::MovieEditorWindowTestAccess::layout(editor);
}

TYST_TEST(MovieEditorUi, CachedPreviewsUseRealThumbnailsAndAllAudioChannels) {
    movie_editor_app::MovieEditorWindow editor;
    movie_editor_app::MovieEditorWindowTestAccess::media_previews(editor);
}

TYST_TEST(MovieEditorUi, MenuSelectionsMutateTimelineAndRespectBackgroundJobs) {
    movie_editor_app::MovieEditorWindow editor;
    movie_editor_app::MovieEditorWindowTestAccess::menus(editor);
}

TYST_TEST(MovieEditorUi, TitlesCanBeCreatedEditedValidatedAndUndoneFromControls) {
    movie_editor_app::MovieEditorWindow editor;
    movie_editor_app::MovieEditorWindowTestAccess::title_editor(editor);
}

TYST_TEST(MovieEditorUi, DirectDropsAndTrackContextDeletionAreUndoable) {
    trekker::movie_editor::EditorProject project;
    trekker::movie_editor::MediaAsset asset;
    asset.path = "pointer-drop-media";
    asset.duration_us = 5'000'000;
    project.media_bin().add_asset(asset);
    movie_editor_app::MovieEditorWindow editor(std::move(project));
    movie_editor_app::MovieEditorWindowTestAccess::track_context_and_drop(editor);
}

TYST_TEST(MovieEditorUi, EventLoopKeepsPlayingAndDoesNotWaitForBackgroundJobsUntilClose) {
    trekker::movie_editor::EditorProject project;
    trekker::movie_editor::MediaAsset asset;
    asset.path = "event-loop-test";
    asset.duration_us = 60'000'000;
    const auto key = project.media_bin().add_asset(asset);
    project.add_clip(project.add_video_track().id(), key, 0);
    movie_editor_app::MovieEditorWindow editor(std::move(project));
    movie_editor_app::MovieEditorWindowTestAccess::event_loop(editor);
}

TYST_TEST(MovieEditorUi, MouseGesturesAndShortcutsUseUndoableEditingOperations) {
    trekker::movie_editor::EditorProject project;
    trekker::movie_editor::MediaAsset asset;
    asset.path = "ui-test-media";
    asset.duration_us = 10'000'000;
    const auto key = project.media_bin().add_asset(asset);
    const auto from = project.add_video_track("Video 1").id();
    project.add_video_track("Video 2");
    project.add_clip(from, key, 2'000'000, 1'000'000, 5'000'000);
    movie_editor_app::MovieEditorWindow editor(std::move(project));
    movie_editor_app::MovieEditorWindowTestAccess::exercise(editor);
}
