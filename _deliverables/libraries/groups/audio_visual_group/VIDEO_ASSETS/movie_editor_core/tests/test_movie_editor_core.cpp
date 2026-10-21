#include "tyst_framework.hpp"
#include "movie_editor_core.h"
#include "graphics.h"
#include "json.h"
#include "../source/ffmpeg_tools.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <limits>
#include <set>
#include <string>
#include <vector>

using namespace trekker::movie_editor;
namespace fs = std::filesystem;

namespace {

constexpr double pi = 3.14159265358979323846;

std::string temp_dir_path(const std::string& suffix) {
    static int counter = 0;
    return (fs::temp_directory_path() /
            ("movie_editor_core_test_" + std::to_string(++counter) + suffix)).string();
}

// Writes a solid-color BMP frame (via the existing charts::Canvas encoder)
// so MediaBin::import_image_sequence() has real image files to read back
// through graphics::loadTextureFromFile().
void write_solid_bmp(const std::string& path, int w, int h, graphics::Color color) {
    graphics::Canvas canvas(w, h, color);
    canvas.fill(color);
    TYST_ASSERT_TRUE(canvas.save_bmp(path));
}

std::string make_image_sequence(int frame_count, int w = 8, int h = 8) {
    const std::string dir = temp_dir_path("_frames");
    fs::create_directories(dir);
    // One distinct color per frame (red channel encodes the frame index) so
    // tests can verify which frame was selected for a given timestamp.
    for (int i = 0; i < frame_count; ++i) {
        char name[64];
        std::snprintf(name, sizeof(name), "/frame_%04d.bmp", i);
        write_solid_bmp(dir + name, w, h, graphics::Color{static_cast<uint8_t>(i * 10 + 5), 0, 0, 255});
    }
    return dir;
}

std::string make_still_image(graphics::Color color, int w = 8, int h = 8) {
    const std::string path = temp_dir_path("_still.bmp");
    write_solid_bmp(path, w, h, color);
    return path;
}

std::string make_wav(std::uint32_t sample_rate, std::uint32_t channels, std::size_t num_samples, double freq_hz) {
    const std::string path = temp_dir_path("_audio.wav");
    trekker::audio::AudioBuffer buf = trekker::audio::AudioBuffer::silent(
        sample_rate, channels, num_samples, trekker::audio::SampleFormat::F32);
    for (std::uint32_t c = 0; c < channels; ++c) {
        for (std::size_t i = 0; i < num_samples; ++i) {
            buf.planes[c][i] = static_cast<float>(0.5 * std::sin(2.0 * pi * freq_hz * i / sample_rate));
        }
    }
    trekker::audio::container::write_wav(path, buf, trekker::audio::container::WavEncoding::PCM16);
    return path;
}

trekker::video::VideoFrame make_solid_video_frame(std::size_t w, std::size_t h, std::uint8_t r, std::uint8_t g, std::uint8_t b) {
    trekker::video::VideoFrame frame = trekker::video::VideoFrame::blank(trekker::video::PixelFormat::RGB24, w, h);
    trekker::video::Plane& plane = frame.planes[0];
    for (std::size_t y = 0; y < h; ++y) {
        for (std::size_t x = 0; x < w; ++x) {
            plane.at(x * 3 + 0, y) = r;
            plane.at(x * 3 + 1, y) = g;
            plane.at(x * 3 + 2, y) = b;
        }
    }
    return frame;
}

std::string make_test_gif(std::size_t w, std::size_t h) {
    const std::string path = temp_dir_path("_test.gif");
    trekker::video::container::GifEncoder encoder(path, w, h);
    encoder.write_frame(make_solid_video_frame(w, h, 220, 20, 20), 200'000);  // red, 200ms
    encoder.write_frame(make_solid_video_frame(w, h, 20, 20, 220), 300'000); // blue, 300ms
    encoder.finish();
    return path;
}

std::string make_test_avi(std::size_t w, std::size_t h, double fps) {
    const std::string path = temp_dir_path("_test.avi");
    trekker::video::container::AviWriter writer(path, w, h, fps);
    writer.write_frame(make_solid_video_frame(w, h, 30, 150, 60));
    writer.write_frame(make_solid_video_frame(w, h, 60, 30, 150));
    writer.finish();
    return path;
}

void remove_all(const std::string& path) {
    std::error_code ec;
    fs::remove_all(path, ec);
}

} // namespace

TYST_TEST(MovieEditorCoreTests, DeleteTrackRestoresClipsOrderAndPropertiesWithUndo) {
    EditorProject project;
    MediaAsset asset;
    asset.path = "track-deletion-media";
    asset.duration_us = 5'000'000;
    const auto key = project.media_bin().add_asset(asset);
    const auto first = project.add_video_track("First").id();
    const auto deleted = project.add_video_track("Middle").id();
    const auto last = project.add_audio_track("Last").id();
    const auto clip = project.add_clip(deleted, key, 1'000'000);
    project.timeline().track(deleted)->set_muted(true);
    TYST_ASSERT_TRUE(project.delete_track(deleted));
    TYST_EXPECT_EQ(project.timeline().tracks().size(), 2U);
    TYST_EXPECT_EQ(project.timeline().tracks()[0].id(), first);
    TYST_EXPECT_EQ(project.timeline().tracks()[1].id(), last);
    TYST_EXPECT_TRUE(project.media_bin().find(key) != nullptr);
    TYST_ASSERT_TRUE(project.undo());
    const auto& restored = project.timeline().tracks()[1];
    TYST_EXPECT_EQ(restored.id(), deleted);
    TYST_EXPECT_EQ(restored.name(), "Middle");
    TYST_EXPECT_TRUE(restored.muted());
    TYST_ASSERT_EQ(restored.clips().size(), 1U);
    TYST_EXPECT_EQ(restored.clips().front().id, clip);
    TYST_EXPECT_EQ(restored.clips().front().position_us, 1'000'000);
    project.timeline().track(deleted)->set_locked(true);
    TYST_EXPECT_FALSE(project.delete_track(deleted));
    TYST_EXPECT_FALSE(project.delete_track(99999));
    TYST_EXPECT_TRUE(project.can_redo());
    TYST_ASSERT_TRUE(project.redo());
    TYST_EXPECT_TRUE(project.timeline().track(deleted) == nullptr);
}

TYST_TEST(MovieEditorCoreTests, DropOnNewTrackIsOneUndoableEditAndValidatesBeforeMutation) {
    EditorProject project;
    MediaAsset asset;
    asset.path = "dropped-audio";
    asset.type = MediaType::Audio;
    asset.duration_us = 5'000'000;
    const auto key = project.media_bin().add_asset(asset);
    TYST_EXPECT_THROW(project.add_clip_on_new_track("unknown", 0), std::invalid_argument);
    TYST_EXPECT_THROW(project.add_clip_on_new_track(key, -1), std::invalid_argument);
    TYST_EXPECT_THROW(project.add_clip_on_new_track(key, std::numeric_limits<std::int64_t>::max()),
                      std::invalid_argument);
    TYST_EXPECT_TRUE(project.timeline().tracks().empty());
    TYST_EXPECT_FALSE(project.can_undo());
    const auto clip = project.add_clip_on_new_track(key, 2'000'000);
    TYST_ASSERT_EQ(project.timeline().tracks().size(), 1U);
    TYST_EXPECT_EQ(project.timeline().tracks().front().type(), trekker::timeline::TrackType::Audio);
    TYST_EXPECT_EQ(project.timeline().tracks().front().clips().front().id, clip);
    TYST_ASSERT_TRUE(project.undo());
    TYST_EXPECT_TRUE(project.timeline().tracks().empty());
    TYST_EXPECT_FALSE(project.can_undo());
    TYST_ASSERT_TRUE(project.redo());
    TYST_EXPECT_EQ(project.timeline().tracks().front().clips().front().position_us, 2'000'000);
}

TYST_TEST(MovieEditorCoreTests, MoveAndTrimAreUndoableAndPreserveMedia) {
    EditorProject project;
    MediaAsset asset;
    asset.path = "editing-test";
    asset.duration_us = 5'000'000;
    const auto key = project.media_bin().add_asset(asset);
    const auto from = project.add_video_track("Original").id();
    const auto to = project.add_video_track("Destination").id();
    const auto clip = project.add_clip(from, key, 0);
    project.move_clip(from, clip, to, 1'000'000);
    TYST_EXPECT_TRUE(project.timeline().track(from)->clips().empty());
    TYST_EXPECT_EQ(project.timeline().track(to)->clips().front().id, clip);
    project.trim_clip(to, clip, 500'000, 4'000'000, 1'500'000);
    TYST_EXPECT_EQ(project.duration_us(), 5'000'000);
    TYST_ASSERT_TRUE(project.undo());
    TYST_EXPECT_EQ(project.timeline().track(to)->clips().front().in_point_us, 0);
    TYST_ASSERT_TRUE(project.undo());
    TYST_EXPECT_EQ(project.timeline().track(from)->clips().front().position_us, 0);
    TYST_ASSERT_TRUE(project.redo());
    TYST_ASSERT_TRUE(project.redo());
    TYST_EXPECT_EQ(project.timeline().track(to)->clips().front().out_point_us, 4'000'000);
    TYST_EXPECT_TRUE(project.media_bin().find(key) != nullptr);
}

TYST_TEST(MovieEditorCoreTests, RejectsInvalidEditsWithoutConsumingHistory) {
    EditorProject project;
    MediaAsset asset;
    asset.path = "validation-test";
    asset.duration_us = 5'000'000;
    const auto key = project.media_bin().add_asset(asset);
    const auto track = project.add_video_track().id();
    const auto other = project.add_video_track().id();
    const auto audio = project.add_audio_track().id();
    const auto clip = project.add_clip(track, key, 0);
    TYST_EXPECT_THROW(project.move_clip(track, clip, audio, 0), std::invalid_argument);
    TYST_EXPECT_THROW(project.move_clip(track, clip, other, -1), std::invalid_argument);
    TYST_EXPECT_THROW(project.move_clip(track, clip, other, std::numeric_limits<std::int64_t>::max()), std::invalid_argument);
    TYST_EXPECT_THROW(project.trim_clip(track, clip, 0, 6'000'000, 0), std::invalid_argument);
    TYST_EXPECT_THROW(project.trim_clip(track, clip, 500'000, 500'000, 0), std::invalid_argument);
    project.timeline().track(other)->set_locked(true);
    TYST_EXPECT_THROW(project.move_clip(track, clip, other, 0), std::invalid_argument);
    project.timeline().track(track)->set_locked(true);
    TYST_EXPECT_THROW(project.trim_clip(track, clip, 0, 1'000'000, 0), std::invalid_argument);
    TYST_EXPECT_FALSE(project.delete_clip(track, clip));
    TYST_EXPECT_FALSE(project.split_clip(track, 1'000'000));
    project.timeline().track(track)->set_locked(false);
    project.move_clip(track, clip, track, 0);
    project.trim_clip(track, clip, 0, 5'000'000, 0);
    TYST_EXPECT_FALSE(project.delete_clip(track, clip + 1));
    TYST_EXPECT_FALSE(project.split_clip(track, 0));
    TYST_EXPECT_FALSE(project.split_clip(track, 5'000'000));
    TYST_ASSERT_TRUE(project.undo());
    TYST_EXPECT_TRUE(project.timeline().track(track)->clips().empty());
}

TYST_TEST(MovieEditorCoreTests, SplitIdsRemainUniqueAfterUndoBranchingAndNewEdits) {
    EditorProject project;
    MediaAsset asset;
    asset.path = "split-id-test";
    asset.duration_us = 5'000'000;
    const auto key = project.media_bin().add_asset(asset);
    const auto track = project.add_video_track().id();
    const auto initial_id = project.add_clip(track, key, 0);
    TYST_ASSERT_TRUE(project.split_clip(track, 2'000'000));
    const auto first_right_id = project.timeline().track(track)->clips().back().id;
    TYST_EXPECT_NE(first_right_id, initial_id);
    TYST_ASSERT_TRUE(project.undo());
    TYST_EXPECT_EQ(project.timeline().track(track)->clips().front().id, initial_id);
    TYST_ASSERT_TRUE(project.split_clip(track, 1'000'000));
    TYST_EXPECT_FALSE(project.can_redo());
    TYST_EXPECT_GT(project.timeline().track(track)->clips().back().id, first_right_id);
    project.add_clip(track, key, 6'000'000);
    std::set<std::uint64_t> ids;
    for (const auto& clip : project.timeline().track(track)->clips())
        TYST_EXPECT_TRUE(ids.insert(clip.id).second);
}

TYST_TEST(MovieEditorCoreTests, HistoryIsBoundedAndRedoRestoresLastEdit) {
    EditorProject project;
    MediaAsset asset;
    asset.path = "history-bound-test";
    asset.duration_us = 1'000'000;
    const auto key = project.media_bin().add_asset(asset);
    const auto track = project.add_video_track().id();
    const auto clip = project.add_clip(track, key, 0);
    for (int i = 1; i <= 105; ++i) project.move_clip(track, clip, track, i * 1'000'000LL);
    int count = 0;
    while (project.undo()) ++count;
    TYST_EXPECT_EQ(count, 100);
    TYST_EXPECT_EQ(project.timeline().track(track)->clips().front().position_us, 5'000'000);
    while (project.redo()) {}
    TYST_EXPECT_EQ(project.timeline().track(track)->clips().front().position_us, 105'000'000);
}

TYST_TEST(MovieEditorCoreTests, EditedProjectRoundTripsExistingFormatAndResetsHistory) {
    const auto path = make_still_image(graphics::Color{200, 10, 10, 255});
    const auto saved = temp_dir_path("_edited.json");
    EditorProject project;
    const auto key = project.media_bin().import_still_image(path);
    const auto from = project.add_video_track("Original").id();
    const auto to = project.add_video_track("Destination").id();
    const auto clip = project.add_clip(from, key, 0);
    project.trim_clip(from, clip, 500'000, 2'000'000, 1'000'000);
    project.move_clip(from, clip, to, 2'000'000);
    project.save_project(saved);
    project.load_project(saved);
    TYST_EXPECT_FALSE(project.can_undo());
    TYST_EXPECT_FALSE(project.can_redo());
    TYST_EXPECT_EQ(project.duration_us(), 3'500'000);
    TYST_ASSERT_TRUE(project.split_clip(to, 2'500'000));
    TYST_EXPECT_GT(project.timeline().track(to)->clips().back().id, clip);
    TYST_EXPECT_EQ(project.render_frame_at(2'600'000, 4, 4).planes[0].at(0, 0), 200);
    remove_all(path);
    remove_all(saved);
}

TYST_TEST(MovieEditorCoreTests, Mp4RejectsEmptyInvalidSettingsAndSourceOverwrite) {
    EditorProject project;
    TYST_EXPECT_THROW(project.export_mp4("unused.mp4"), std::invalid_argument);
    const auto image = make_still_image(graphics::Color{200, 10, 10, 255});
    const auto key = project.media_bin().import_still_image(image, 250'000);
    const auto track = project.add_video_track().id();
    project.add_clip(track, key, 0);
    TYST_EXPECT_THROW(project.export_mp4(image), std::invalid_argument);
    TYST_EXPECT_THROW(project.export_mp4("unused.mp4", 0, 64, 48), std::invalid_argument);
    TYST_EXPECT_THROW(project.export_mp4("unused.mp4", std::numeric_limits<double>::quiet_NaN(), 64, 48), std::invalid_argument);
    TYST_EXPECT_THROW(project.export_mp4("unused.mp4", 24, 63, 48), std::invalid_argument);
    TYST_EXPECT_THROW(project.export_mp4("unused.mp4", 24, 0, 48), std::invalid_argument);
    TYST_EXPECT_THROW(project.export_mp4("unused.mp4", 24, 8000, 48), std::invalid_argument);
    TYST_EXPECT_TRUE(fs::exists(image));
    remove_all(image);
}

TYST_TEST(MovieEditorCoreTests, Mp4ExportsWithFfmpegAndRetainsExistingOutputOnFailure) {
    const char* ffmpeg = std::getenv("COOLBOX_FFMPEG");
    if (!ffmpeg || !*ffmpeg) TYST_SKIP() << "Set COOLBOX_FFMPEG to run the real encoder integration test.";
    const std::string executable(ffmpeg);
    const auto image = make_still_image(graphics::Color{200, 10, 10, 255});
    const auto output = temp_dir_path("_export & spaces.mp4");
    EditorProject project;
    const auto key = project.media_bin().import_still_image(image, 250'000);
    project.add_clip(project.add_video_track().id(), key, 0);
    project.export_mp4(output, 24, 64, 48);
    TYST_EXPECT_GT(fs::file_size(output), 500);
    std::ifstream encoded(output, std::ios::binary);
    const std::string before((std::istreambuf_iterator<char>(encoded)), std::istreambuf_iterator<char>());
    encoded.close();
    TYST_EXPECT_EQ(before.substr(4, 4), "ftyp");
#ifdef _WIN32
    _putenv_s("COOLBOX_FFMPEG", "coolbox_missing_ffmpeg_for_test.exe");
#else
    setenv("COOLBOX_FFMPEG", "coolbox_missing_ffmpeg_for_test", 1);
#endif
    TYST_EXPECT_THROW(project.export_mp4(output, 24, 64, 48), std::runtime_error);
#ifdef _WIN32
    _putenv_s("COOLBOX_FFMPEG", executable.c_str());
#else
    setenv("COOLBOX_FFMPEG", executable.c_str(), 1);
#endif
    std::ifstream retained(output, std::ios::binary);
    const std::string after((std::istreambuf_iterator<char>(retained)), std::istreambuf_iterator<char>());
    TYST_EXPECT_EQ(after, before);
    retained.close();
    remove_all(image);
    remove_all(output);
}

TYST_TEST(MovieEditorCoreTests, FfmpegStreamsInputAndReportsEarlyExitAndProducerFailure) {
    if (!std::getenv("COOLBOX_FFMPEG")) TYST_SKIP() << "Set COOLBOX_FFMPEG to test streaming.";
    const auto cache = detail::make_video_cache();
    const auto directory = fs::path(cache->directory);
    const std::vector<std::string> arguments{"-nostdin", "-v", "error", "-f", "rawvideo",
        "-pixel_format", "rgb24", "-video_size", "64x48", "-framerate", "24",
        "-i", "pipe:0", "-f", "null", "-"};
    std::vector<unsigned char> frame(64 * 48 * 3, 127);
    std::size_t frames_sent = 0;
    detail::run_ffmpeg(arguments, directory / "stream.log", [&](const detail::ByteWriter& write) {
        for (int i = 0; i < 24; ++i) { write(frame.data(), frame.size()); ++frames_sent; }
    });
    TYST_EXPECT_EQ(frames_sent, std::size_t{24});
    TYST_EXPECT_FALSE(fs::exists(directory / "frames.rgb"));
    TYST_EXPECT_THROW(detail::run_ffmpeg({"-coolbox-invalid-option"}, directory / "early-exit.log",
        [&](const detail::ByteWriter& write) {
            for (int i = 0; i < 4096; ++i) write(frame.data(), frame.size());
        }), std::runtime_error);
    TYST_EXPECT_THROW(detail::run_ffmpeg(arguments, directory / "producer-error.log",
        [&](const detail::ByteWriter& write) {
            write(frame.data(), frame.size());
            throw std::runtime_error("Test frame producer failed.");
        }), std::runtime_error);
}

#ifndef _WIN32
TYST_TEST(MovieEditorCoreTests, ClosedEncoderInputDoesNotRaiseSigpipe) {
    const char* configured = std::getenv("COOLBOX_FFMPEG");
    const bool had_override = configured != nullptr;
    const std::string previous = configured ? configured : "";
    const auto cache = detail::make_video_cache();
    TYST_ASSERT_EQ(setenv("COOLBOX_FFMPEG", "/bin/false", 1), 0);
    TYST_EXPECT_THROW(detail::run_ffmpeg({}, fs::path(cache->directory) / "closed-input.log",
        [](const detail::ByteWriter& write) {
            const std::vector<unsigned char> bytes(1024 * 1024);
            for (int i = 0; i < 16; ++i) write(bytes.data(), bytes.size());
        }), std::runtime_error);
    if (had_override) TYST_EXPECT_EQ(setenv("COOLBOX_FFMPEG", previous.c_str(), 1), 0);
    else TYST_EXPECT_EQ(unsetenv("COOLBOX_FFMPEG"), 0);
}
#endif

TYST_TEST(MovieEditorCoreTests, TextureLoaderPreservesPngPixelsWithoutFfmpeg) {
    const auto path = temp_dir_path("_texture.png");
    graphics::Canvas image(16, 8);
    image.fill(graphics::Color{210, 80, 35, 255});
    TYST_ASSERT_TRUE(image.save_png(path));
    EditorProject project;
    const auto key = project.media_bin().import_still_image(path);
    project.add_clip(project.add_video_track().id(), key, 0);
    const auto frame = project.render_frame_at(0, 16, 8);
    TYST_EXPECT_EQ(frame.planes[0].at(0, 0), 210);
    TYST_EXPECT_EQ(frame.planes[0].at(1, 0), 80);
    TYST_EXPECT_EQ(frame.planes[0].at(2, 0), 35);
    TYST_EXPECT_EQ(frame.planes[0].at(15 * 3, 7), 210);
    remove_all(path);
}

TYST_TEST(MovieEditorCoreTests, PngEditingCacheIsLosslessAndSmallerThanRawFrames) {
    if (!std::getenv("COOLBOX_FFMPEG")) TYST_SKIP() << "Set COOLBOX_FFMPEG to test lossless cache compression.";
    const auto files = detail::make_video_cache();
    const auto directory = fs::path(files->directory);
    const auto source_image = directory / "pattern.bmp";
    const auto source_video = directory / "lossless.mkv";
    graphics::Canvas image(320, 180);
    for (int y = 0; y < 180; ++y)
        for (int x = 0; x < 320; ++x)
            image.set_pixel(x, y, graphics::Color{static_cast<std::uint8_t>(x % 256),
                static_cast<std::uint8_t>(y), static_cast<std::uint8_t>((x / 16 + y / 16) % 2 * 255), 255});
    TYST_ASSERT_TRUE(image.save_bmp(source_image.string()));
    detail::run_ffmpeg({"-nostdin", "-v", "error", "-y", "-loop", "1", "-framerate", "24",
        "-i", source_image.string(), "-t", "1", "-c:v", "ffv1", "-pix_fmt", "bgr0",
        source_video.string()}, directory / "source.log");
    std::string cache_directory;
    {
        auto asset = decode_ffmpeg_video(source_video.string());
        cache_directory = asset.import_cache->directory;
        TYST_EXPECT_EQ(asset.frame_files.size(), std::size_t{24});
        std::uintmax_t compressed_bytes = 0;
        for (const auto& frame : asset.frame_files) {
            TYST_EXPECT_EQ(fs::path(frame).extension().string(), ".png");
            compressed_bytes += fs::file_size(frame);
        }
        const auto raw_bytes = std::uintmax_t{320 * 180 * 3 * 24};
        TYST_EXPECT_LT(compressed_bytes, raw_bytes / 4);
        std::cout << "Lossless PNG cache: " << compressed_bytes << " bytes; raw RGB: " << raw_bytes << " bytes.\n";
        EditorProject imported;
        const auto key = imported.media_bin().add_asset(std::move(asset));
        imported.add_clip(imported.add_video_track().id(), key, 0);
        EditorProject reference;
        const auto original = reference.media_bin().import_still_image(source_image.string(), 1'000'000);
        reference.add_clip(reference.add_video_track().id(), original, 0);
        const auto expected = reference.render_frame_at(0, 320, 180);
        for (const auto timestamp : {0, 500'000, 999'999}) {
            const auto actual = imported.render_frame_at(timestamp, 320, 180);
            TYST_EXPECT_TRUE(actual.planes[0].data == expected.planes[0].data);
        }
    }
    TYST_EXPECT_FALSE(fs::exists(cache_directory));
}

// ── MediaBin ──────────────────────────────────────────────────────────────────

TYST_TEST(MovieEditorCoreTests, ImportImageSequenceFindsAllFrames) {
    const std::string dir = make_image_sequence(5);
    MediaBin bin;
    const std::string& key = bin.import_image_sequence(dir, 10.0);
    const MediaAsset* asset = bin.find(key);
    TYST_ASSERT_TRUE(asset != nullptr);
    TYST_EXPECT_EQ(asset->type, MediaType::ImageSequence);
    TYST_EXPECT_EQ(asset->frame_files.size(), static_cast<std::size_t>(5));
    TYST_EXPECT_EQ(asset->duration_us, static_cast<std::int64_t>(500'000)); // 5 frames @ 10fps
    remove_all(dir);
}

TYST_TEST(MovieEditorCoreTests, ImportImageSequenceThrowsOnEmptyDirectory) {
    const std::string dir = temp_dir_path("_empty");
    fs::create_directories(dir);
    MediaBin bin;
    TYST_EXPECT_THROW(bin.import_image_sequence(dir, 24.0), std::runtime_error);
    remove_all(dir);
}

TYST_TEST(MovieEditorCoreTests, ImportStillImageDefaultsToFiveSeconds) {
    const std::string path = make_still_image(graphics::Color{200, 100, 50, 255});
    MediaBin bin;
    const std::string& key = bin.import_still_image(path);
    const MediaAsset* asset = bin.find(key);
    TYST_ASSERT_TRUE(asset != nullptr);
    TYST_EXPECT_EQ(asset->type, MediaType::StillImage);
    TYST_EXPECT_EQ(asset->duration_us, static_cast<std::int64_t>(5'000'000));
    remove_all(path);
}

TYST_TEST(MovieEditorCoreTests, ImportAudioReadsWavMetadata) {
    const std::string path = make_wav(16000, 2, 8000, 220.0);
    MediaBin bin;
    const std::string& key = bin.import_audio(path);
    const MediaAsset* asset = bin.find(key);
    TYST_ASSERT_TRUE(asset != nullptr);
    TYST_EXPECT_EQ(asset->type, MediaType::Audio);
    TYST_EXPECT_EQ(asset->audio_sample_rate, static_cast<std::uint32_t>(16000));
    TYST_EXPECT_EQ(asset->audio_channels, static_cast<std::uint32_t>(2));
    remove_all(path);
}

TYST_TEST(MovieEditorCoreTests, ImportVideoFileDecodesGif) {
    const std::string path = make_test_gif(4, 4);
    MediaBin bin;
    const std::string key = bin.import_video_file(path);
    const MediaAsset* asset = bin.find(key);
    TYST_ASSERT_TRUE(asset != nullptr);
    TYST_EXPECT_EQ(asset->type, MediaType::ImportedVideo);
    TYST_ASSERT_EQ(asset->decoded_frames.size(), static_cast<std::size_t>(2));
    TYST_ASSERT_EQ(asset->frame_start_us.size(), static_cast<std::size_t>(2));
    TYST_EXPECT_EQ(asset->frame_start_us[0], static_cast<std::int64_t>(0));
    TYST_EXPECT_EQ(asset->frame_start_us[1], static_cast<std::int64_t>(200'000));
    TYST_EXPECT_EQ(asset->duration_us, static_cast<std::int64_t>(500'000));
    TYST_EXPECT_EQ(asset->decoded_frames[0].planes[0].at(0, 0), static_cast<std::uint8_t>(220));
    TYST_EXPECT_EQ(asset->decoded_frames[1].planes[0].at(2, 0), static_cast<std::uint8_t>(220));
    std::remove(path.c_str());
}

TYST_TEST(MovieEditorCoreTests, ImportVideoFileDecodesAvi) {
    const std::string path = make_test_avi(4, 4, 10.0);
    MediaBin bin;
    const std::string key = bin.import_video_file(path);
    const MediaAsset* asset = bin.find(key);
    TYST_ASSERT_TRUE(asset != nullptr);
    TYST_EXPECT_EQ(asset->type, MediaType::ImportedVideo);
    TYST_ASSERT_EQ(asset->decoded_frames.size(), static_cast<std::size_t>(2));
    TYST_EXPECT_EQ(asset->duration_us, static_cast<std::int64_t>(200'000)); // 2 frames @ 10fps
    TYST_EXPECT_EQ(asset->decoded_frames[0].planes[0].at(1, 0), static_cast<std::uint8_t>(150));
    TYST_EXPECT_EQ(asset->decoded_frames[1].planes[0].at(0, 0), static_cast<std::uint8_t>(60));
    std::remove(path.c_str());
}

TYST_TEST(MovieEditorCoreTests, ImportVideoFileThrowsOnUnsupportedExtension) {
    const std::string path = temp_dir_path("_fake.mp4");
    { std::ofstream touch(path, std::ios::binary); touch << "not really a video"; }
    MediaBin bin;
    TYST_EXPECT_THROW(bin.import_video_file(path), std::runtime_error);
    std::remove(path.c_str());
}

TYST_TEST(MovieEditorCoreTests, RenderFrameAtUsesImportedVideoFrames) {
    const std::string path = make_test_gif(4, 4);
    EditorProject project;
    const std::string key = project.media_bin().import_video_file(path);
    auto& track = project.add_video_track();
    project.add_clip(track.id(), key, 0);

    // First GIF frame (red) is visible for [0, 200ms); second (blue) for
    // [200ms, 500ms).
    const auto frame_early = project.render_frame_at(50'000, 4, 4);
    TYST_EXPECT_EQ(frame_early.planes[0].at(0, 0), static_cast<std::uint8_t>(220));
    const auto frame_late = project.render_frame_at(300'000, 4, 4);
    TYST_EXPECT_EQ(frame_late.planes[0].at(2, 0), static_cast<std::uint8_t>(220));

    std::remove(path.c_str());
}

TYST_TEST(MovieEditorCoreTests, JsonRoundTripsWindowsPathsAndQuotedNames) {
    using dataformats::json::Object;
    using dataformats::json::Parser;
    using dataformats::json::Value;

    const std::string path = R"(C:\new\takes\render\clip.gif)";
    const std::string name = "Track \"one\"\nNext\tline\r";
    TYST_EXPECT_EQ(Value(path).to_string(), R"("C:\\new\\takes\\render\\clip.gif")");

    Object object;
    object.set(path, Value(name));
    const auto decoded = Parser::parse(Value(object).to_string()).as_object();
    TYST_EXPECT_EQ(decoded.get(path).as_string(), name);
    TYST_EXPECT_EQ(Parser::parse(R"("clip \u0026 take")").as_string(), "clip & take");
    TYST_EXPECT_EQ(Parser::parse(R"("\u00e9")").as_string(), std::string("\xc3\xa9"));
    TYST_EXPECT_EQ(Parser::parse(R"("\ud83d\udcf9")").as_string(), std::string("\xf0\x9f\x93\xb9"));
    TYST_EXPECT_EQ(Parser::parse(Value(std::string("\b\f\x01")).to_string()).as_string(), std::string("\b\f\x01"));
    TYST_EXPECT_THROW(Parser::parse(R"("\uZZZZ")"), std::runtime_error);
    TYST_EXPECT_THROW(Parser::parse(R"("\ud800")"), std::runtime_error);
}

TYST_TEST(MovieEditorCoreTests, FfmpegImportsVideoAndAudioAndCleansDiskCache) {
    const char* fixture = std::getenv("COOLBOX_TEST_VIDEO");
    if (!fixture) TYST_SKIP() << "Set COOLBOX_TEST_VIDEO to a one-second red MP4 with audio";
    std::string cache_directory;
    {
        EditorProject project;
        const auto key = project.media_bin().import_video_file(fixture);
        const auto* asset = project.media_bin().find(key);
        TYST_ASSERT_TRUE(asset != nullptr);
        TYST_ASSERT_TRUE(asset->import_cache != nullptr);
        cache_directory = asset->import_cache->directory;
        TYST_EXPECT_TRUE(fs::is_directory(cache_directory));
        TYST_EXPECT_EQ(asset->frame_files.size(), std::size_t(24));
        TYST_EXPECT_TRUE(asset->decoded_frames.empty());
        TYST_EXPECT_TRUE(fs::is_regular_file(asset->audio_path));
        auto& track = project.add_video_track("Camera video");
        project.add_clip(track.id(), key, 0);
        const auto frame = project.render_frame_at(0, 64, 48);
        TYST_EXPECT_GT(frame.planes[0].at(0, 0), 200);
        TYST_EXPECT_LT(frame.planes[0].at(1, 0), 30);
        const auto mixed = project.render_audio_mix(0, 1'000'000, 48000, 2);
        double energy = 0.0;
        for (const float sample : mixed.planes[0]) energy += sample * sample;
        TYST_EXPECT_GT(energy, 1.0);
        const auto saved = temp_dir_path("_ffmpeg_project.json");
        project.save_project(saved);
        EditorProject reloaded;
        reloaded.load_project(saved);
        TYST_EXPECT_EQ(reloaded.render_frame_at(0, 64, 48).planes[0].at(0, 0), frame.planes[0].at(0, 0));
        std::remove(saved.c_str());
    }
    TYST_EXPECT_FALSE(fs::exists(cache_directory));
}

TYST_TEST(MovieEditorCoreTests, SaveAndLoadProjectRoundTripsImportedVideo) {
    const std::string path = make_test_gif(4, 4);
    EditorProject project;
    const std::string key = project.media_bin().import_video_file(path);
    auto& track = project.add_video_track("v");
    project.add_clip(track.id(), key, 0);

    const std::string project_path = temp_dir_path("_video_project.json");
    project.save_project(project_path);

    const auto before = project.render_frame_at(50'000, 4, 4);

    EditorProject reloaded;
    reloaded.load_project(project_path);
    const auto* asset = reloaded.media_bin().find(key);
    TYST_ASSERT_TRUE(asset != nullptr);
    TYST_EXPECT_EQ(asset->type, MediaType::ImportedVideo);
    const auto after = reloaded.render_frame_at(50'000, 4, 4);
    TYST_EXPECT_EQ(after.planes[0].at(0, 0), before.planes[0].at(0, 0));

    std::remove(path.c_str());
    std::remove(project_path.c_str());
}

// ── EditorProject timeline editing ───────────────────────────────────────────

TYST_TEST(MovieEditorCoreTests, AddClipAndRenderFrameSelectsCorrectSource) {
    const std::string red_dir = make_image_sequence(1, 4, 4);   // top track, frame red
    const std::string blue_path = make_still_image(graphics::Color{0, 0, 200, 255});

    EditorProject project;
    const std::string& red_key = project.media_bin().import_image_sequence(red_dir, 24.0);
    const std::string& blue_key = project.media_bin().import_still_image(blue_path, 2'000'000);

    auto& bottom = project.add_video_track("bottom");
    auto& top = project.add_video_track("top");

    project.add_clip(bottom.id(), blue_key, 0);
    project.add_clip(top.id(), red_key, 500'000, 0, 1'000'000);

    // At t=0, only the bottom (blue) track has a clip; top track's clip
    // starts at 500ms, so bottom wins.
    const auto frame0 = project.render_frame_at(0, 4, 4);
    TYST_EXPECT_EQ(frame0.planes[0].at(2, 0), static_cast<std::uint8_t>(200)); // blue channel

    // At t=700ms, the top track's clip covers this point and should win
    // (higher track index = higher priority).
    const auto frame700 = project.render_frame_at(700'000, 4, 4);
    TYST_EXPECT_EQ(frame700.planes[0].at(0, 0), static_cast<std::uint8_t>(5)); // red channel, frame 0

    remove_all(red_dir);
    remove_all(blue_path);
}

TYST_TEST(MovieEditorCoreTests, RenderFrameAtReturnsBlackWhenNoClipCovers) {
    EditorProject project;
    project.add_video_track("v1");
    const auto frame = project.render_frame_at(0, 4, 4);
    TYST_EXPECT_EQ(frame.planes[0].at(0, 0), static_cast<std::uint8_t>(0));
    TYST_EXPECT_EQ(frame.planes[0].at(1, 0), static_cast<std::uint8_t>(0));
    TYST_EXPECT_EQ(frame.planes[0].at(2, 0), static_cast<std::uint8_t>(0));
}

TYST_TEST(MovieEditorCoreTests, PreviewAndExportPreserveSourceAspectRatio) {
    const auto path = make_still_image(graphics::Color{220, 0, 0, 255}, 4, 4);
    EditorProject project;
    const auto key = project.media_bin().import_still_image(path);
    auto& track = project.add_video_track("Square");
    project.add_clip(track.id(), key, 0);
    const auto frame = project.render_frame_at(0, 8, 4);
    TYST_EXPECT_EQ(frame.planes[0].at(0, 0), std::uint8_t(0));
    TYST_EXPECT_EQ(frame.planes[0].at(2 * 3, 0), std::uint8_t(220));
    TYST_EXPECT_EQ(frame.planes[0].at(5 * 3, 3), std::uint8_t(220));
    TYST_EXPECT_EQ(frame.planes[0].at(7 * 3, 0), std::uint8_t(0));
    std::remove(path.c_str());
}

TYST_TEST(MovieEditorCoreTests, SplitAndDeleteClip) {
    const std::string path = make_still_image(graphics::Color{10, 20, 30, 255});
    EditorProject project;
    const std::string& key = project.media_bin().import_still_image(path, 2'000'000);
    auto& track = project.add_video_track();
    const auto clip_id = project.add_clip(track.id(), key, 0);

    TYST_EXPECT_TRUE(project.split_clip(track.id(), 1'000'000));
    TYST_EXPECT_EQ(project.timeline().track(track.id())->clips().size(), static_cast<std::size_t>(2));

    // The original clip id no longer exists post-split (split replaces it
    // with two new clips), but the second half should still be removable by
    // its own id.
    const auto& clips = project.timeline().track(track.id())->clips();
    TYST_EXPECT_TRUE(project.delete_clip(track.id(), clips.back().id));
    TYST_EXPECT_EQ(project.timeline().track(track.id())->clips().size(), static_cast<std::size_t>(1));
    TYST_EXPECT_FALSE(project.delete_clip(track.id(), clip_id)); // already gone via split
    remove_all(path);
}

// ── Audio mixing ──────────────────────────────────────────────────────────────

TYST_TEST(MovieEditorCoreTests, AudioSpeedAndSegmentedResamplingStayOnTheVideoTimeline) {
    const auto path = temp_dir_path("_ramp.wav");
    auto source = trekker::audio::AudioBuffer::silent(1000, 1, 1000);
    for (std::size_t i = 0; i < source.num_samples(); ++i) source.planes[0][i] = static_cast<float>(i / 2000.0);
    trekker::audio::container::write_wav(path, source, trekker::audio::container::WavEncoding::PCM16);
    EditorProject project;
    const auto key = project.media_bin().import_audio(path);
    auto& track = project.add_audio_track();
    const auto id = project.add_clip(track.id(), key, 0);
    auto clip = track.clips().front();
    track.remove_clip(id);
    clip.speed = 2.0f;
    track.add_clip(clip);
    const auto full = project.render_audio_mix(0, 500'000, 48000, 2);
    TYST_EXPECT_NEAR(full.planes[0][4800], 0.1f, 0.0001f);
    TYST_EXPECT_EQ(project.duration_us(), 500'000);
    const auto part = project.render_audio_mix(100'000, 200'000, 48000, 2);
    for (std::size_t i = 0; i < part.num_samples(); ++i)
        TYST_EXPECT_NEAR(part.planes[0][i], full.planes[0][4800 + i], 0.000001f);
    track.remove_clip(id);
    clip.speed = 0.5f;
    track.add_clip(clip);
    const auto slow = project.render_audio_mix(0, 200'000, 48000, 2);
    TYST_EXPECT_NEAR(slow.planes[0][4800], 0.025f, 0.0001f);
    remove_all(path);
}

TYST_TEST(MovieEditorCoreTests, RenderAudioMixCombinesOverlappingClips) {
    const std::string wav_a = make_wav(8000, 1, 8000, 200.0);
    const std::string wav_b = make_wav(8000, 1, 8000, 200.0);

    EditorProject project;
    const std::string& key_a = project.media_bin().import_audio(wav_a);
    const std::string& key_b = project.media_bin().import_audio(wav_b);
    auto& track = project.add_audio_track();
    project.add_clip(track.id(), key_a, 0);
    project.add_clip(track.id(), key_b, 0);

    const auto mix = project.render_audio_mix(0, 500'000, 8000, 1);
    TYST_ASSERT_TRUE(mix.num_samples() > 0);
    // Two identical in-phase sine waves summed should be louder (roughly 2x,
    // before any clipping) than a single source at the same point in time.
    trekker::audio::AudioBuffer single;
    single.format = trekker::audio::SampleFormat::F32;
    single.sample_rate = 8000;
    single.num_channels = 1;
    single.planes = {std::vector<float>(mix.num_samples())};
    for (std::size_t i = 0; i < mix.num_samples(); ++i) {
        single.planes[0][i] = static_cast<float>(0.5 * std::sin(2.0 * pi * 200.0 * i / 8000));
    }
    double sum_mix = 0.0, sum_single = 0.0;
    for (std::size_t i = 0; i < mix.num_samples(); ++i) {
        sum_mix += std::fabs(mix.planes[0][i]);
        sum_single += std::fabs(single.planes[0][i]);
    }
    TYST_EXPECT_GT(sum_mix, sum_single * 1.5);

    remove_all(wav_a);
    remove_all(wav_b);
}

TYST_TEST(MovieEditorCoreTests, RenderAudioMixAppliesClipVolume) {
    const std::string wav_a = make_wav(8000, 1, 8000, 200.0);
    EditorProject project;
    const std::string& key_a = project.media_bin().import_audio(wav_a);
    auto& track = project.add_audio_track();
    const auto clip_id = project.add_clip(track.id(), key_a, 0);

    auto* t = project.timeline().track(track.id());
    // Mutate the clip's volume directly via remove+re-add (Track exposes no
    // in-place mutator), mirroring how the app layer would adjust a clip.
    std::vector<trekker::timeline::Clip> clips(t->clips().begin(), t->clips().end());
    TYST_ASSERT_EQ(clips.size(), static_cast<std::size_t>(1));
    trekker::timeline::Clip quiet = clips[0];
    quiet.volume = 0.1f;
    t->remove_clip(clip_id);
    t->add_clip(quiet);

    const auto loud_mix = project.render_audio_mix(0, 500'000, 8000, 1);
    double energy = 0.0;
    for (auto v : loud_mix.planes[0]) energy += std::fabs(v);
    const double avg = energy / loud_mix.num_samples();
    TYST_EXPECT_LT(avg, 0.5 * 0.1 + 0.02); // scaled-down sine's mean abs value

    remove_all(wav_a);
}

// ── Export ────────────────────────────────────────────────────────────────────

TYST_TEST(MovieEditorCoreTests, ExportAviProducesPlayableFile) {
    const std::string dir = make_image_sequence(4, 4, 4);
    const std::string wav = make_wav(8000, 1, 4000, 300.0);

    EditorProject project;
    const std::string& vkey = project.media_bin().import_image_sequence(dir, 4.0); // 1s total
    const std::string& akey = project.media_bin().import_audio(wav);
    auto& vtrack = project.add_video_track();
    auto& atrack = project.add_audio_track();
    project.add_clip(vtrack.id(), vkey, 0);
    project.add_clip(atrack.id(), akey, 0);

    const std::string out_path = temp_dir_path("_export.avi");
    project.export_avi(out_path, 4.0, 4, 4, 8000, 1);

    std::ifstream in(out_path, std::ios::binary);
    std::vector<char> header(12);
    in.read(header.data(), 12);
    TYST_EXPECT_EQ(std::string(header.data(), 4), std::string("RIFF"));
    TYST_EXPECT_EQ(std::string(header.data() + 8, 4), std::string("AVI "));

    remove_all(dir);
    remove_all(wav);
    std::remove(out_path.c_str());
}

TYST_TEST(MovieEditorCoreTests, ExportGifProducesValidFile) {
    const std::string dir = make_image_sequence(3, 4, 4);
    EditorProject project;
    const std::string& vkey = project.media_bin().import_image_sequence(dir, 3.0);
    auto& vtrack = project.add_video_track();
    project.add_clip(vtrack.id(), vkey, 0);

    const std::string out_path = temp_dir_path("_export.gif");
    project.export_gif(out_path, 0, project.duration_us(), 3.0, 4, 4);

    std::ifstream in(out_path, std::ios::binary);
    std::vector<char> header(6);
    in.read(header.data(), 6);
    TYST_EXPECT_EQ(std::string(header.data(), 6), std::string("GIF89a"));

    remove_all(dir);
    std::remove(out_path.c_str());
}

TYST_TEST(MovieEditorCoreTests, ExportAudioProducesValidWav) {
    const std::string wav = make_wav(8000, 1, 8000, 250.0);
    EditorProject project;
    const std::string& akey = project.media_bin().import_audio(wav);
    auto& atrack = project.add_audio_track();
    project.add_clip(atrack.id(), akey, 0);

    const std::string out_path = temp_dir_path("_export.wav");
    project.export_audio(out_path, trekker::audio::container::WavEncoding::PCM16, 8000, 1);

    const auto reloaded = trekker::audio::container::read_wav(out_path);
    TYST_EXPECT_EQ(reloaded.sample_rate, static_cast<std::uint32_t>(8000));
    TYST_ASSERT_TRUE(reloaded.num_samples() > 0);

    remove_all(wav);
    std::remove(out_path.c_str());
}

// ── Project persistence ───────────────────────────────────────────────────────

TYST_TEST(MovieEditorCoreTests, SaveAndLoadProjectRoundTripsRenderedFrame) {
    const std::string dir = make_image_sequence(2, 4, 4);
    EditorProject project;
    const std::string& vkey = project.media_bin().import_image_sequence(dir, 2.0);
    auto& vtrack = project.add_video_track("v");
    project.add_clip(vtrack.id(), vkey, 0);

    const std::string project_path = temp_dir_path("_project.json");
    project.save_project(project_path);

    const auto before = project.render_frame_at(0, 4, 4);

    EditorProject reloaded;
    reloaded.load_project(project_path);
    TYST_EXPECT_EQ(reloaded.timeline().tracks().size(), static_cast<std::size_t>(1));
    const auto after = reloaded.render_frame_at(0, 4, 4);
    TYST_EXPECT_EQ(after.planes[0].at(0, 0), before.planes[0].at(0, 0));

    remove_all(dir);
    std::remove(project_path.c_str());
}

namespace {
void add_color_video(EditorProject& project, std::uint8_t r, std::uint8_t g, std::uint8_t b) {
    MediaAsset asset;
    asset.path = "memory-color-" + std::to_string(project.timeline().tracks().size());
    asset.type = MediaType::ImportedVideo;
    asset.duration_us = 10'000'000;
    asset.decoded_frames.push_back(make_solid_video_frame(320, 180, r, g, b));
    asset.frame_start_us.push_back(0);
    const auto key = project.media_bin().add_asset(std::move(asset));
    project.add_clip_on_new_track(key, 0);
}

std::size_t nonblack_pixels(const trekker::video::VideoFrame& frame) {
    std::size_t count = 0;
    for (std::size_t y = 0; y < frame.height; ++y)
        for (std::size_t x = 0; x < frame.width; ++x)
            if (frame.planes[0].at(x * 3, y) || frame.planes[0].at(x * 3 + 1, y) ||
                frame.planes[0].at(x * 3 + 2, y)) ++count;
    return count;
}

void expect_same_pixels(const trekker::video::VideoFrame& a, const trekker::video::VideoFrame& b) {
    TYST_ASSERT_EQ(a.width, b.width);
    TYST_ASSERT_EQ(a.height, b.height);
    for (std::size_t y = 0; y < a.height; ++y)
        for (std::size_t x = 0; x < a.width * 3; ++x)
            TYST_EXPECT_EQ(a.planes[0].at(x, y), b.planes[0].at(x, y));
}
}

TYST_TEST(MovieEditorCoreTests, TextCreationIsOneUndoEntryAndValidatesBeforeMutation) {
    EditorProject project;
    TextStyle style;
    const auto id = project.add_text_clip(style, 1'000'000, 2'000'000);
    const auto track = project.timeline().tracks().back().id();
    TYST_EXPECT_EQ(project.duration_us(), 3'000'000);
    TYST_ASSERT_TRUE(project.text_style(track, id));
    TYST_EXPECT_EQ(project.text_style(track, id)->text, "Title");
    TYST_ASSERT_TRUE(project.undo());
    TYST_EXPECT_TRUE(project.timeline().tracks().empty());
    TYST_EXPECT_FALSE(project.can_undo());
    TYST_ASSERT_TRUE(project.redo());
    TYST_ASSERT_TRUE(project.text_style(track, id));
    TYST_EXPECT_THROW(project.add_text_clip(style, -1, 1), std::invalid_argument);
    TYST_EXPECT_THROW(project.add_text_clip(style, 0, 0), std::invalid_argument);
    TYST_EXPECT_THROW(project.add_text_clip(style, std::numeric_limits<std::int64_t>::max(), 1),
                      std::invalid_argument);
    std::vector<TextStyle> invalid(11, style);
    invalid[0].text.clear();
    invalid[1].text = std::string(16385, 'a');
    invalid[2].color = 0x1000000;
    invalid[3].background_color = 0xffffffff;
    invalid[4].font_size = 0;
    invalid[5].font_size = std::numeric_limits<double>::quiet_NaN();
    invalid[6].x = -0.1;
    invalid[7].y = 1.1;
    invalid[8].x = std::numeric_limits<double>::infinity();
    invalid[9].fade_in_us = -1;
    invalid[10].fade_out_us = -1;
    for (const auto& bad : invalid)
        TYST_EXPECT_THROW(project.add_text_clip(bad, 0, 1'000'000), std::invalid_argument);
    TYST_EXPECT_EQ(project.timeline().tracks().size(), 1U);
    TYST_ASSERT_TRUE(project.undo());
    TYST_EXPECT_FALSE(project.can_undo());
}

TYST_TEST(MovieEditorCoreTests, BackgroundOpacityAndDraftPreviewMatchCommittedRenderingWithoutMutation) {
    EditorProject project;
    add_color_video(project, 200, 100, 50);
    TextStyle style;
    style.background_color = 0x0000ff;
    style.background_opacity = 0.5;
    style.opacity = 0;
    style.fade_in_us = style.fade_out_us = 0;
    const auto before = project.timeline().tracks().size();
    const auto preview = project.render_text_preview(style, 1'000'000, 320, 180);
    TYST_EXPECT_EQ(project.timeline().tracks().size(), before);
    TYST_EXPECT_EQ(preview.planes[0].at(0, 0), 100);
    TYST_EXPECT_EQ(preview.planes[0].at(1, 0), 50);
    TYST_EXPECT_EQ(preview.planes[0].at(2, 0), 153);
    const auto clip = project.add_text_clip(style, 0, 5'000'000);
    const auto track = project.timeline().tracks().back().id();
    expect_same_pixels(preview, project.render_frame_at(1'000'000, 320, 180));
    style.background_opacity = 0.25;
    const auto revised = project.render_text_preview(style, 1'000'000, 320, 180, track, clip);
    TYST_EXPECT_EQ(revised.planes[0].at(0, 0), 150);
    TYST_EXPECT_EQ(revised.planes[0].at(1, 0), 75);
    TYST_EXPECT_EQ(revised.planes[0].at(2, 0), 101);
    TYST_EXPECT_EQ(project.text_style(track, clip)->background_opacity, 0.5);
    style.background = false;
    const auto transparent = project.render_text_preview(style, 1'000'000, 320, 180, track, clip);
    TYST_EXPECT_EQ(transparent.planes[0].at(0, 0), 200);
    TYST_EXPECT_EQ(transparent.planes[0].at(1, 0), 100);
    TYST_EXPECT_EQ(transparent.planes[0].at(2, 0), 50);
    style.background_opacity = 1.1;
    TYST_EXPECT_THROW(project.add_text_clip(style, 0, 1'000'000), std::invalid_argument);
    style.background_opacity = 0;
    style.opacity = -0.1;
    TYST_EXPECT_THROW(project.add_text_clip(style, 0, 1'000'000), std::invalid_argument);
}

TYST_TEST(MovieEditorCoreTests, TextOpacityRoundTripsAndDefaultsForExistingProjects) {
    EditorProject project;
    TextStyle style;
    style.background_opacity = 0.25;
    style.opacity = 0.75;
    style.fade_in_us = style.fade_out_us = 0;
    project.add_text_clip(style, 0, 1'000'000);
    const auto path = temp_dir_path("_text_opacity.json");
    project.save_project(path);
    EditorProject loaded;
    loaded.load_project(path);
    const auto& clip = loaded.timeline().tracks().front().clips().front();
    TYST_ASSERT_TRUE(clip.text.has_value());
    TYST_EXPECT_EQ(clip.text->background_opacity, 0.25);
    TYST_EXPECT_EQ(clip.text->opacity, 0.75);
    expect_same_pixels(project.render_frame_at(500'000, 160, 90), loaded.render_frame_at(500'000, 160, 90));
    std::ifstream in(path);
    std::ostringstream contents;
    contents << in.rdbuf();
    in.close();
    std::string json = contents.str();
    for (const auto& key : {std::string("\"background_opacity\":"), std::string("\"opacity\":")}) {
        const auto start = json.find(key);
        TYST_ASSERT_NE(start, std::string::npos);
        auto end = json.find(',', start);
        TYST_ASSERT_NE(end, std::string::npos);
        json.erase(start, end - start + 1);
    }
    std::ofstream out(path, std::ios::trunc);
    out << json;
    out.close();
    loaded.load_project(path);
    const auto& legacy = *loaded.timeline().tracks().front().clips().front().text;
    TYST_EXPECT_EQ(legacy.background_opacity, 1.0);
    TYST_EXPECT_EQ(legacy.opacity, 1.0);
    std::remove(path.c_str());
}

TYST_TEST(MovieEditorCoreTests, TextEditMoveTrimSplitAndLockedChecksPreserveHistory) {
    EditorProject project;
    TextStyle style;
    const auto id = project.add_text_clip(style, 1'000'000, 2'000'000);
    const auto original = project.timeline().tracks().back().id();
    style.text = "Updated\nSubtitle";
    style.background = false;
    style.color = 0x123456;
    project.edit_text_clip(original, id, style, 4'000'000);
    TYST_EXPECT_EQ(project.duration_us(), 5'000'000);
    TYST_EXPECT_EQ(project.text_style(original, id)->text, style.text);
    TYST_ASSERT_TRUE(project.undo());
    TYST_EXPECT_EQ(project.text_style(original, id)->text, "Title");
    TYST_EXPECT_EQ(project.duration_us(), 3'000'000);
    TYST_ASSERT_TRUE(project.redo());
    project.timeline().track(original)->set_locked(true);
    TYST_EXPECT_THROW(project.edit_text_clip(original, id, style, 1'000'000), std::invalid_argument);
    TYST_EXPECT_THROW(project.trim_clip(original, id, 0, 8'000'000, 0), std::invalid_argument);
    TYST_EXPECT_FALSE(project.split_clip(original, 2'000'000));
    TYST_EXPECT_FALSE(project.delete_clip(original, id));
    project.timeline().track(original)->set_locked(false);
    const auto other = project.add_video_track("Other").id();
    const auto audio = project.add_audio_track().id();
    TYST_EXPECT_THROW(project.move_clip(original, id, audio, 0), std::invalid_argument);
    project.move_clip(original, id, other, 0);
    TYST_EXPECT_FALSE(project.text_style(original, id));
    TYST_ASSERT_TRUE(project.text_style(other, id));
    project.trim_clip(other, id, 1'000'000, 9'000'000, 2'000'000);
    TYST_EXPECT_EQ(project.duration_us(), 10'000'000);
    TYST_ASSERT_TRUE(project.split_clip(other, 5'000'000));
    const auto* split = project.timeline().track(other);
    TYST_ASSERT_EQ(split->clips().size(), 2U);
    TYST_EXPECT_EQ(split->clips()[0].timeline_duration_us(), 3'000'000);
    TYST_EXPECT_EQ(split->clips()[1].timeline_duration_us(), 5'000'000);
    for (const auto& clip : split->clips()) {
        TYST_ASSERT_TRUE(clip.text.has_value());
        TYST_EXPECT_EQ(clip.text->text, style.text);
        TYST_EXPECT_EQ(clip.text->color, style.color);
    }
    TYST_EXPECT_EQ(nonblack_pixels(project.render_frame_at(2'000'000, 160, 90)), 0U);
    TYST_EXPECT_TRUE(nonblack_pixels(project.render_frame_at(3'000'000, 160, 90)) > 0);
    TYST_EXPECT_EQ(nonblack_pixels(project.render_frame_at(5'000'000, 160, 90)), 0U);
    TYST_ASSERT_TRUE(project.undo());
    TYST_ASSERT_TRUE(project.text_style(other, id));
    TYST_ASSERT_TRUE(project.undo());
    TYST_EXPECT_EQ(project.timeline().track(other)->clips().front().timeline_duration_us(), 4'000'000);
    TYST_EXPECT_THROW(project.edit_text_clip(other, id, style, 0), std::invalid_argument);
    TYST_EXPECT_THROW(project.edit_text_clip(other, 0, style, 1), std::invalid_argument);
}

TYST_TEST(MovieEditorCoreTests, TitleBackgroundFadesAsOneLayerOverVideoAtExactTimes) {
    EditorProject project;
    add_color_video(project, 200, 40, 80);
    TextStyle style;
    style.text = "Fade";
    style.background_color = 0x1464b4;
    style.color = 0xeeeedd;
    const auto id = project.add_text_clip(style, 1'000'000, 2'000'000);
    const auto track = project.timeline().tracks().back().id();
    const auto full = project.render_frame_at(1'500'000, 160, 90);
    const auto half_in = project.render_frame_at(1'250'000, 160, 90);
    const auto half_out = project.render_frame_at(2'750'000, 160, 90);
    TYST_EXPECT_EQ(full.planes[0].at(0, 0), 20);
    TYST_EXPECT_EQ(full.planes[0].at(1, 0), 100);
    TYST_EXPECT_EQ(full.planes[0].at(2, 0), 180);
    TYST_EXPECT_EQ(half_in.planes[0].at(0, 0), 110);
    TYST_EXPECT_EQ(half_in.planes[0].at(1, 0), 70);
    TYST_EXPECT_EQ(half_in.planes[0].at(2, 0), 130);
    const std::uint8_t base[] = {200, 40, 80};
    for (std::size_t y = 0; y < 90; ++y)
        for (std::size_t x = 0; x < 160 * 3; ++x) {
            TYST_EXPECT_EQ(half_in.planes[0].at(x, y),
                static_cast<std::uint8_t>(std::lround((base[x % 3] + full.planes[0].at(x, y)) / 2.0)));
            TYST_EXPECT_EQ(half_out.planes[0].at(x, y), half_in.planes[0].at(x, y));
        }
    for (auto pts : {999'999LL, 1'000'000LL, 3'000'000LL})
        TYST_EXPECT_EQ(project.render_frame_at(pts, 160, 90).planes[0].at(0, 0), 200);
    project.timeline().track(track)->set_muted(true);
    TYST_EXPECT_EQ(project.render_frame_at(1'500'000, 160, 90).planes[0].at(0, 0), 200);
    project.timeline().track(track)->set_muted(false);
    style.fade_in_us = style.fade_out_us = 0;
    project.edit_text_clip(track, id, style, 2'000'000);
    expect_same_pixels(project.render_frame_at(1'000'000, 160, 90), full);
    add_color_video(project, 7, 8, 9);
    TYST_EXPECT_EQ(project.render_frame_at(1'500'000, 160, 90).planes[0].at(0, 0), 7);
}

TYST_TEST(MovieEditorCoreTests, MutedTextAndMissingStyleDoNotHideLowerVideo) {
    EditorProject project;
    add_color_video(project, 77, 88, 99);
    TextStyle style;
    style.fade_in_us = style.fade_out_us = 0;
    const auto id = project.add_text_clip(style, 0, 1'000'000);
    auto& track = *project.timeline().track(project.timeline().tracks().back().id());
    auto clip = track.clips().front();
    clip.muted = true;
    track.remove_clip(id);
    track.add_clip(clip);
    TYST_EXPECT_EQ(project.render_frame_at(500'000, 160, 90).planes[0].at(0, 0), 77);
    TYST_EXPECT_FALSE(project.text_style(0, id));
    TYST_EXPECT_FALSE(project.text_style(track.id(), 0));
    TYST_EXPECT_FALSE(project.text_style(project.timeline().tracks().front().id(),
                                         project.timeline().tracks().front().clips().front().id));
}

TYST_TEST(MovieEditorCoreTests, TextFadesUseCurrentTimelineBoundsAfterSpeedTrimAndSplit) {
    EditorProject project;
    TextStyle style;
    style.background_color = 0x640000;
    const auto id = project.add_text_clip(style, 0, 2'000'000);
    auto& track = *project.timeline().track(project.timeline().tracks().back().id());
    auto clip = track.clips().front();
    clip.speed = 2;
    track.remove_clip(id);
    track.add_clip(clip);
    TYST_EXPECT_EQ(project.duration_us(), 1'000'000);
    TYST_EXPECT_EQ(project.render_frame_at(0, 160, 90).planes[0].at(0, 0), 0);
    TYST_EXPECT_EQ(project.render_frame_at(250'000, 160, 90).planes[0].at(0, 0), 50);
    TYST_EXPECT_EQ(project.render_frame_at(500'000, 160, 90).planes[0].at(0, 0), 100);
    TYST_EXPECT_EQ(project.render_frame_at(750'000, 160, 90).planes[0].at(0, 0), 50);
    TYST_EXPECT_EQ(project.render_frame_at(1'000'000, 160, 90).planes[0].at(0, 0), 0);
    project.trim_clip(track.id(), id, 1'000'000, 3'000'000, 2'000'000);
    TYST_EXPECT_EQ(project.render_frame_at(2'000'000, 160, 90).planes[0].at(0, 0), 0);
    TYST_EXPECT_EQ(project.render_frame_at(2'250'000, 160, 90).planes[0].at(0, 0), 50);
    TYST_ASSERT_TRUE(project.split_clip(track.id(), 2'500'000));
    TYST_EXPECT_EQ(project.render_frame_at(2'250'000, 160, 90).planes[0].at(0, 0), 50);
    TYST_EXPECT_EQ(project.render_frame_at(2'500'000, 160, 90).planes[0].at(0, 0), 0);
    TYST_EXPECT_EQ(project.render_frame_at(2'750'000, 160, 90).planes[0].at(0, 0), 50);
    TYST_EXPECT_EQ(project.render_frame_at(3'000'000, 160, 90).planes[0].at(0, 0), 0);
}

TYST_TEST(MovieEditorCoreTests, TransparentTextUsesTrueTypeAntialiasAndNormalizedMultilineLayout) {
    EditorProject project;
    TextStyle style;
    style.text = "Ag\nAg";
    style.background = false;
    style.font_size = 96;
    style.fade_in_us = style.fade_out_us = 0;
    const auto id = project.add_text_clip(style, 0, 2'000'000);
    const auto track = project.timeline().tracks().back().id();
    const auto centered = project.render_frame_at(0, 640, 360);
    TYST_EXPECT_EQ(centered.planes[0].at(0, 0), 0);
    TYST_EXPECT_TRUE(nonblack_pixels(centered) > 100);
    bool antialias = false;
    std::size_t upper = 0, lower = 0;
    for (std::size_t y = 0; y < centered.height; ++y)
        for (std::size_t x = 0; x < centered.width; ++x) {
            const auto value = centered.planes[0].at(x * 3, y);
            antialias = antialias || (value > 0 && value < 255);
            if (value) {
                if (y < centered.height / 2) ++upper;
                else ++lower;
            }
        }
    TYST_EXPECT_TRUE(antialias);
    TYST_EXPECT_TRUE(upper > 50);
    TYST_EXPECT_TRUE(lower > 50);
    const auto small = project.render_frame_at(0, 320, 180);
    TYST_EXPECT_TRUE(nonblack_pixels(centered) > nonblack_pixels(small) * 2);
    style.x = style.y = 0;
    project.edit_text_clip(track, id, style, 2'000'000);
    const auto corner = project.render_frame_at(0, 640, 360);
    std::size_t bottom_right = 0, top_left = 0;
    for (std::size_t y = 0; y < corner.height; ++y)
        for (std::size_t x = 0; x < corner.width; ++x)
            if (corner.planes[0].at(x * 3, y)) {
                if (x < 320 && y < 180) ++top_left;
                else ++bottom_right;
            }
    TYST_EXPECT_TRUE(top_left > 100);
    TYST_EXPECT_EQ(bottom_right, 0U);
}

TYST_TEST(MovieEditorCoreTests, OverlayFadeBlendsRealRgbAndShortOverlappingFadesStayBounded) {
    EditorProject project;
    add_color_video(project, 20, 100, 200);
    TextStyle style;
    style.background = false;
    style.color = 0xf01464;
    style.font_size = 120;
    const auto id = project.add_text_clip(style, 0, 2'000'000);
    const auto track = project.timeline().tracks().back().id();
    const auto full = project.render_frame_at(500'000, 320, 180);
    const auto half = project.render_frame_at(250'000, 320, 180);
    TYST_EXPECT_EQ(half.planes[0].at(0, 0), 20);
    const std::uint8_t base[] = {20, 100, 200};
    bool changed = false;
    for (std::size_t y = 0; y < full.height; ++y)
        for (std::size_t x = 0; x < full.width * 3; ++x) {
            changed = changed || (full.planes[0].at(x, y) != base[x % 3]);
            TYST_EXPECT_EQ(half.planes[0].at(x, y),
                static_cast<std::uint8_t>(std::lround((base[x % 3] + full.planes[0].at(x, y)) / 2.0)));
        }
    TYST_EXPECT_TRUE(changed);
    style.background = true;
    style.background_color = 0xffffff;
    project.edit_text_clip(track, id, style, 250'000);
    const auto middle = project.render_frame_at(125'000, 320, 180);
    TYST_EXPECT_EQ(middle.planes[0].at(0, 0), 79);
    TYST_EXPECT_EQ(middle.planes[0].at(1, 0), 139);
    TYST_EXPECT_EQ(middle.planes[0].at(2, 0), 214);
}

TYST_TEST(MovieEditorCoreTests, TextSaveLoadPreservesAllStyleFieldsAndRejectsInvalidTextAtomically) {
    const auto path = (fs::current_path() / "movie_core_text_roundtrip.json").string();
    EditorProject project;
    TextStyle style;
    style.text = "Title \"quoted\"\nCaf\xc3\xa9";
    style.background = false;
    style.color = 0x23a4f5;
    style.background_color = 0x785634;
    style.font_size = 88.123456789;
    style.x = 0.21234567890123456;
    style.y = 0.81234567890123456;
    style.fade_in_us = 1'234'567;
    style.fade_out_us = 2'345'678;
    const auto id = project.add_text_clip(style, 3'456'789, 2'345'678);
    project.save_project(path);
    EditorProject loaded;
    loaded.load_project(path);
    const auto track = loaded.timeline().tracks().back().id();
    const auto* text = loaded.text_style(track, id);
    TYST_ASSERT_TRUE(text);
    TYST_EXPECT_EQ(text->text, style.text);
    TYST_EXPECT_EQ(text->background, style.background);
    TYST_EXPECT_EQ(text->color, style.color);
    TYST_EXPECT_EQ(text->background_color, style.background_color);
    TYST_EXPECT_EQ(text->font_size, style.font_size);
    TYST_EXPECT_EQ(text->x, style.x);
    TYST_EXPECT_EQ(text->y, style.y);
    TYST_EXPECT_EQ(text->fade_in_us, style.fade_in_us);
    TYST_EXPECT_EQ(text->fade_out_us, style.fade_out_us);
    TYST_EXPECT_FALSE(loaded.can_undo());
    TYST_EXPECT_EQ(loaded.duration_us(), project.duration_us());
    TYST_EXPECT_EQ(loaded.timeline().tracks().back().clips().front().position_us, 3'456'789);
    TYST_EXPECT_EQ(loaded.timeline().tracks().back().clips().front().out_point_us, 2'345'678);
    expect_same_pixels(project.render_frame_at(4'000'000, 160, 90), loaded.render_frame_at(4'000'000, 160, 90));
    const auto next = loaded.add_text_clip(TextStyle{}, 0, 1'000'000);
    TYST_EXPECT_TRUE(next > id);
    {
        std::ofstream file(path);
        file << R"({"tracks":[{"type":"video","clips":[{"id":1,"source_path":"","in_point_us":0,"out_point_us":1000000,"position_us":0,"text":{"color":16777216}}]}]})";
    }
    TYST_EXPECT_THROW(loaded.load_project(path), std::invalid_argument);
    TYST_ASSERT_TRUE(loaded.text_style(track, id));
    TYST_EXPECT_EQ(loaded.text_style(track, id)->text, style.text);
    std::remove(path.c_str());
}

TYST_TEST(MovieEditorCoreTests, TextAviExportMatchesPreviewPixelsAndGifIncludesFadeTiming) {
    const auto avi_path = (fs::current_path() / "movie_core_text_export.avi").string();
    const auto gif_path = (fs::current_path() / "movie_core_text_export.gif").string();
    EditorProject project;
    TextStyle style;
    style.text = "Export";
    style.background_color = 0x602010;
    style.font_size = 120;
    project.add_text_clip(style, 0, 1'000'000);
    project.export_avi(avi_path, 4, 160, 90, 8000, 1);
    const auto avi = trekker::video::container::read_avi(avi_path);
    TYST_ASSERT_EQ(avi.frames.size(), 4U);
    for (std::size_t i = 0; i < avi.frames.size(); ++i)
        expect_same_pixels(avi.frames[i], project.render_frame_at(static_cast<std::int64_t>(i) * 250'000, 160, 90));
    project.export_gif(gif_path, 0, 1'000'000, 4, 160, 90);
    const auto gif = trekker::video::container::read_gif(gif_path);
    TYST_ASSERT_EQ(gif.frames.size(), 4U);
    TYST_EXPECT_EQ(nonblack_pixels(gif.frames.front()), 0U);
    TYST_EXPECT_TRUE(nonblack_pixels(gif.frames[2]) > 100);
    for (const auto delay : gif.frame_delay_us) TYST_EXPECT_EQ(delay, 250'000);
    std::remove(avi_path.c_str());
    std::remove(gif_path.c_str());
}
