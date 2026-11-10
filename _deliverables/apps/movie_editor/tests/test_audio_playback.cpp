#include "audio_playback.hpp"
#include "timeline_gesture.hpp"
#include "tyst_framework.hpp"
#include <chrono>
#include <thread>
#include <filesystem>
#include <cstdlib>
#include <cmath>

TYST_TEST(MovieEditorPlayback, StreamsAudioAndUsesDeviceClockAcrossSeekAndStop) {
    trekker::movie_editor::EditorProject project;
    trekker::movie_editor::MediaAsset asset;
    asset.path = "silent-test-frame";
    asset.duration_us = 500'000;
    const auto key = project.media_bin().add_asset(std::move(asset));
    auto& track = project.add_video_track("Video");
    project.add_clip(track.id(), key, 0);
    movie_editor_app::AudioPlayback playback;
    playback.start(project, 0, movie_editor_app::AudioPlayback::Device::NullForTests);
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    TYST_ASSERT_TRUE(playback.position_us() > 50'000);
    TYST_ASSERT_TRUE(playback.position_us() < 400'000);
    playback.stop();
    const auto paused = playback.position_us();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    TYST_ASSERT_EQ(playback.position_us(), paused);
    playback.start(project, 400'000, movie_editor_app::AudioPlayback::Device::NullForTests);
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (!playback.finished() && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    playback.check_error();
    TYST_ASSERT_TRUE(playback.finished());
    playback.stop();
    TYST_ASSERT_THROW(playback.start(project, -1), std::invalid_argument);
}

TYST_TEST(MovieEditorPlayback, SystemDevicePlaysMixedAudioWhenRequested) {
    if (!std::getenv("COOLBOX_TEST_SYSTEM_AUDIO")) TYST_SKIP() << "Enable explicitly to exercise the real audio device.";
    const auto path = std::filesystem::temp_directory_path() /
        ("coolbox_playback_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".wav");
    auto audio = trekker::audio::AudioBuffer::silent(48000, 2, 24000);
    for (std::size_t i = 0; i < audio.num_samples(); ++i)
        audio.planes[0][i] = audio.planes[1][i] = static_cast<float>(0.05 * std::sin(i * 440.0 * 6.283185307179586 / 48000));
    trekker::audio::container::write_wav(path.string(), audio, trekker::audio::container::WavEncoding::PCM16);
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() { std::error_code error; std::filesystem::remove(path, error); }
    } cleanup{path};
    trekker::movie_editor::EditorProject project;
    const auto key = project.media_bin().import_audio(path.string());
    project.add_clip(project.add_audio_track().id(), key, 0);
    movie_editor_app::AudioPlayback playback;
    playback.start(project, 100'000);
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (!playback.finished() && std::chrono::steady_clock::now() < deadline) {
        playback.check_error();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    TYST_EXPECT_TRUE(playback.finished());
    playback.stop();
}

TYST_TEST(MovieEditorPlayback, TrimAndMovePreviewClampWithoutMutatingOriginal) {
    trekker::timeline::Clip original;
    original.in_point_us = 1'000'000;
    original.out_point_us = 4'000'000;
    original.position_us = 2'000'000;
    const auto moved = movie_editor_app::preview_edit(original, movie_editor_app::DragKind::Move, -10'000'000, 5'000'000);
    TYST_EXPECT_EQ(moved.position_us, 0);
    const auto trimmed = movie_editor_app::preview_edit(original, movie_editor_app::DragKind::TrimStart, 1'000'000, 5'000'000);
    TYST_EXPECT_NEAR(trimmed.in_point_us, 2'000'000, 100);
    TYST_EXPECT_EQ(trimmed.timeline_end_us(), original.timeline_end_us());
    const auto extended = movie_editor_app::preview_edit(original, movie_editor_app::DragKind::TrimEnd, 10'000'000, 5'000'000);
    TYST_EXPECT_EQ(extended.out_point_us, 5'000'000);
    const auto shortened = movie_editor_app::preview_edit(original, movie_editor_app::DragKind::TrimEnd, -10'000'000, 5'000'000);
    TYST_EXPECT_GT(shortened.out_point_us, shortened.in_point_us);
    TYST_EXPECT_EQ(original.in_point_us, 1'000'000);
    TYST_EXPECT_EQ(original.position_us, 2'000'000);
}
