#include "../headers/movie_editor_core.h"

#include <algorithm>
#include <cmath>

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

} // namespace movie_editor
} // namespace trekker
