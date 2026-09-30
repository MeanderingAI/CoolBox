#include "tyst_framework.hpp"
#include "movie_editor_core.h"
#include "graphics.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace trekker::movie_editor;
namespace fs = std::filesystem;

namespace {

std::string temp_dir_path(const std::string& suffix) {
    static int counter = 0;
    const char* tmp = std::getenv("TMPDIR");
    const std::string base = tmp ? tmp : "/tmp/";
    return base + (base.back() == '/' ? "" : "/") + "movie_editor_core_test_" +
           std::to_string(++counter) + suffix;
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
            buf.planes[c][i] = static_cast<float>(0.5 * std::sin(2.0 * M_PI * freq_hz * i / sample_rate));
        }
    }
    trekker::audio::container::write_wav(path, buf, trekker::audio::container::WavEncoding::PCM16);
    return path;
}

void remove_all(const std::string& path) {
    std::error_code ec;
    fs::remove_all(path, ec);
}

} // namespace

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
        single.planes[0][i] = static_cast<float>(0.5 * std::sin(2.0 * M_PI * 200.0 * i / 8000));
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
