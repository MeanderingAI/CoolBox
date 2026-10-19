#include "../headers/movie_editor_core.h"

#include <algorithm>
#include <cmath>
#include "ffmpeg_tools.hpp"
#include <fstream>
#include <iostream>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

namespace trekker {
namespace movie_editor {

namespace {

std::vector<std::int16_t> to_interleaved_s16(const audio::AudioBuffer& buffer) {
    std::vector<std::int16_t> out(buffer.num_samples() * buffer.num_channels);
    for (std::size_t i = 0; i < buffer.num_samples(); ++i) {
        for (std::uint32_t c = 0; c < buffer.num_channels; ++c) {
            const float clamped = std::clamp(buffer.planes[c][i], -1.0f, 1.0f);
            out[i * buffer.num_channels + c] = static_cast<std::int16_t>(std::lround(clamped * 32767.0f));
        }
    }
    return out;
}

} // namespace

void EditorProject::export_avi(const std::string& path, double fps, std::size_t width, std::size_t height,
                               std::uint32_t audio_sample_rate, std::uint32_t audio_channels) const {
    if (fps <= 0.0) throw std::invalid_argument("export_avi: fps must be positive");

    const std::int64_t total_us = duration_us();
    const std::int64_t frame_duration_us = static_cast<std::int64_t>(1'000'000.0 / fps);
    const std::size_t num_frames = total_us <= 0 ? 0
        : static_cast<std::size_t>(std::ceil(total_us / static_cast<double>(frame_duration_us)));

    video::container::AviAudioConfig audio_cfg;
    audio_cfg.sample_rate = audio_sample_rate;
    audio_cfg.num_channels = static_cast<std::uint16_t>(audio_channels);
    audio_cfg.bits_per_sample = 16;

    video::container::AviWriter writer(path, width, height, fps, audio_cfg);
    for (std::size_t i = 0; i < num_frames; ++i) {
        const std::int64_t pts_us = static_cast<std::int64_t>(i) * frame_duration_us;
        writer.write_frame(render_frame_at(pts_us, width, height));

        const std::int64_t segment_end_us = pts_us + frame_duration_us;
        const audio::AudioBuffer mix = render_audio_mix(pts_us, segment_end_us, audio_sample_rate, audio_channels);
        writer.write_audio(to_interleaved_s16(mix));
    }
    writer.finish();
}

void EditorProject::export_gif(const std::string& path, std::int64_t start_us, std::int64_t end_us,
                               double fps, std::size_t width, std::size_t height,
                               int max_colors, int loop_count) const {
    if (fps <= 0.0) throw std::invalid_argument("export_gif: fps must be positive");
    if (end_us <= start_us) throw std::invalid_argument("export_gif: end_us must be after start_us");

    const std::int64_t frame_duration_us = static_cast<std::int64_t>(1'000'000.0 / fps);
    const std::size_t num_frames = static_cast<std::size_t>(
        std::ceil((end_us - start_us) / static_cast<double>(frame_duration_us)));

    video::container::GifOptions options;
    options.max_colors = max_colors;
    options.loop_count = loop_count;

    video::container::GifEncoder encoder(path, width, height, options);
    for (std::size_t i = 0; i < num_frames; ++i) {
        const std::int64_t pts_us = start_us + static_cast<std::int64_t>(i) * frame_duration_us;
        encoder.write_frame(render_frame_at(pts_us, width, height), frame_duration_us);
    }
    encoder.finish();
}

void EditorProject::export_audio(const std::string& path, audio::container::WavEncoding encoding,
                                 std::uint32_t sample_rate, std::uint32_t channels) const {
    const audio::AudioBuffer mix = render_audio_mix(0, duration_us(), sample_rate, channels);
    audio::container::write_wav(path, mix, encoding);
}

void EditorProject::export_mp4(const std::string& path, double fps, std::size_t width, std::size_t height) const {
    if (path.empty() || !std::isfinite(fps) || fps < 1 || fps > 120 || width < 2 || height < 2 ||
        width > 7680 || height > 4320 || width % 2 || height % 2 || duration_us() <= 0)
        throw std::invalid_argument("MP4 export requires a non-empty timeline, 1-120 fps and even dimensions up to 7680x4320.");
    namespace fs = std::filesystem;
    const auto output = fs::absolute(path);
    if (!fs::is_directory(output.parent_path()) || fs::is_directory(output))
        throw std::invalid_argument("Choose an MP4 output file in an existing directory.");
    for (const auto& asset : bin_.assets()) {
        if (fs::absolute(asset.path).lexically_normal() == output.lexically_normal() ||
            (fs::exists(output) && fs::exists(asset.path) && fs::equivalent(asset.path, output)))
            throw std::invalid_argument("MP4 output must not overwrite source media.");
    }
    const auto cache = detail::make_video_cache();
    const fs::path directory(cache->directory);
    const auto frames = static_cast<std::int64_t>(std::ceil(duration_us() * fps / 1'000'000.0));
    const auto audio_path = directory / "audio.s16";
    std::ofstream audio_file(audio_path, std::ios::binary);
    if (!audio_file) throw std::runtime_error("Cannot create temporary MP4 audio file.");
    for (std::int64_t start = 0; start < duration_us(); start += 100'000) {
        const auto samples = to_interleaved_s16(render_audio_mix(start,
            std::min(duration_us(), start + 100'000), 48000, 2));
        audio_file.write(reinterpret_cast<const char*>(samples.data()),
                         static_cast<std::streamsize>(samples.size() * sizeof(std::int16_t)));
        if (!audio_file) throw std::runtime_error("MP4 temporary audio write failed. Check free disk space.");
    }
    audio_file.close();
    if (!audio_file) throw std::runtime_error("Cannot finish MP4 audio file.");
    // Encode beside the destination so publication is atomic on the same filesystem.
    const auto staged = output.parent_path() / (output.filename().string() + "." + directory.filename().string() + ".mp4");
    struct StagedOutput {
        fs::path path;
        ~StagedOutput() {
            std::error_code error;
            fs::remove(path, error);
            if (error) std::cerr << "Cannot clean MP4 staging file: " << error.message() << '\n';
        }
    } staging{staged};
    detail::run_ffmpeg({"-nostdin", "-v", "error", "-y", "-f", "rawvideo", "-pix_fmt", "rgb24",
        "-s:v", std::to_string(width) + "x" + std::to_string(height), "-r", std::to_string(fps),
        "-i", "pipe:0", "-f", "s16le", "-ar", "48000", "-ac", "2",
        "-i", audio_path.string(), "-map", "0:v:0", "-map", "1:a:0",
        "-c:v", "libx264", "-preset", "veryfast", "-crf", "20", "-pix_fmt", "yuv420p",
        "-c:a", "aac", "-b:a", "192k", "-t", std::to_string(duration_us() / 1'000'000.0),
        "-movflags", "+faststart", staged.string()}, directory / "encode.log",
        [&](const detail::ByteWriter& write) {
            for (std::int64_t i = 0; i < frames; ++i) {
                const auto frame = render_frame_at(static_cast<std::int64_t>(i * 1'000'000.0 / fps), width, height);
                const auto& plane = frame.planes[0];
                if (plane.stride == width * 3) write(plane.data.data(), width * height * 3);
                else for (std::size_t y = 0; y < height; ++y)
                    write(plane.data.data() + y * plane.stride, width * 3);
            }
        });
#ifdef _WIN32
    if (!MoveFileExW(staged.c_str(), output.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        throw std::runtime_error("Cannot publish MP4 output (Windows error " + std::to_string(GetLastError()) + ").");
#else
    fs::rename(staged, output);
#endif
}

} // namespace movie_editor
} // namespace trekker
