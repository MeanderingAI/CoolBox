#include "../headers/movie_editor_core.h"

#include <algorithm>
#include <cmath>

namespace trekker {
namespace movie_editor {

namespace {

const graphics::Texture* load_cached_texture(const std::string& path,
                                             std::unordered_map<std::string, graphics::Texture>& cache) {
    const auto it = cache.find(path);
    if (it != cache.end()) return &it->second;

    graphics::Texture texture;
    if (!graphics::loadTextureFromFile(path, texture) || texture.width <= 0 || texture.height <= 0) {
        return nullptr;
    }
    auto [inserted, ok] = cache.emplace(path, std::move(texture));
    (void)ok;
    return &inserted->second;
}

// Nearest-neighbour blit of an (possibly RGB/RGBA/gray) Texture into an
// RGB24 VideoFrame at the requested output dimensions.
video::VideoFrame blit_to_frame(const graphics::Texture& tex, std::size_t out_w, std::size_t out_h) {
    video::VideoFrame frame = video::VideoFrame::blank(video::PixelFormat::RGB24, out_w, out_h);
    video::Plane& plane = frame.planes[0];
    for (std::size_t y = 0; y < out_h; ++y) {
        const std::size_t sy = std::min<std::size_t>(tex.height - 1, (y * static_cast<std::size_t>(tex.height)) / out_h);
        for (std::size_t x = 0; x < out_w; ++x) {
            const std::size_t sx = std::min<std::size_t>(tex.width - 1, (x * static_cast<std::size_t>(tex.width)) / out_w);
            const std::size_t src_idx = (sy * static_cast<std::size_t>(tex.width) + sx) * static_cast<std::size_t>(tex.channels);
            std::uint8_t r, g, b;
            if (tex.channels >= 3) {
                r = tex.pixels[src_idx + 0];
                g = tex.pixels[src_idx + 1];
                b = tex.pixels[src_idx + 2];
            } else {
                r = g = b = tex.pixels[src_idx];
            }
            plane.at(x * 3 + 0, y) = r;
            plane.at(x * 3 + 1, y) = g;
            plane.at(x * 3 + 2, y) = b;
        }
    }
    return frame;
}

video::VideoFrame black_frame(std::size_t w, std::size_t h) {
    return video::VideoFrame::blank(video::PixelFormat::RGB24, w, h);
}

// Resolves which frame file within `asset` is visible at clip-local source
// time `source_us` (image sequences advance by fps; stills always show
// their single frame).
const std::string* resolve_frame_file(const MediaAsset& asset, std::int64_t source_us) {
    if (asset.frame_files.empty()) return nullptr;
    if (asset.type == MediaType::StillImage) return &asset.frame_files.front();

    std::int64_t index = static_cast<std::int64_t>((source_us / 1'000'000.0) * asset.fps);
    index = std::max<std::int64_t>(0, std::min<std::int64_t>(index, static_cast<std::int64_t>(asset.frame_files.size()) - 1));
    return &asset.frame_files[static_cast<std::size_t>(index)];
}

} // namespace

video::VideoFrame EditorProject::render_frame_at(std::int64_t pts_us, std::size_t out_width, std::size_t out_height) const {
    const auto& tracks = timeline_.tracks();
    for (auto it = tracks.rbegin(); it != tracks.rend(); ++it) {
        const timeline::Track& track = *it;
        if (track.type() != timeline::TrackType::Video || track.muted()) continue;
        const timeline::Clip* clip = track.clip_at(pts_us);
        if (!clip || clip->muted) continue;

        const MediaAsset* asset = bin_.find(clip->source_path);
        if (!asset || (asset->type != MediaType::ImageSequence && asset->type != MediaType::StillImage)) continue;

        const std::int64_t elapsed_timeline_us = pts_us - clip->position_us;
        const std::int64_t source_us = clip->in_point_us +
            static_cast<std::int64_t>(elapsed_timeline_us * clip->speed);

        const std::string* frame_file = resolve_frame_file(*asset, source_us);
        if (!frame_file) continue;

        const graphics::Texture* tex = load_cached_texture(*frame_file, frame_cache_);
        if (!tex) continue;

        return blit_to_frame(*tex, out_width, out_height);
    }
    return black_frame(out_width, out_height);
}

audio::AudioBuffer EditorProject::render_audio_mix(std::int64_t start_us, std::int64_t end_us,
                                                   std::uint32_t sample_rate, std::uint32_t channels) const {
    const std::int64_t window_us = std::max<std::int64_t>(0, end_us - start_us);
    const std::size_t out_samples = static_cast<std::size_t>(
        std::llround((window_us / 1'000'000.0) * sample_rate));

    std::vector<audio::AudioBuffer> layers;

    for (const auto& track : timeline_.tracks()) {
        if (track.type() != timeline::TrackType::Audio || track.muted()) continue;

        for (const auto& clip : track.clips()) {
            if (clip.muted) continue;
            const std::int64_t clip_end = clip.timeline_end_us();
            if (clip_end <= start_us || clip.position_us >= end_us) continue;

            const MediaAsset* asset = bin_.find(clip.source_path);
            if (!asset || asset->type != MediaType::Audio) continue;

            const auto cache_it = audio_cache_.find(clip.source_path);
            const audio::AudioBuffer* source = nullptr;
            if (cache_it != audio_cache_.end()) {
                source = &cache_it->second;
            } else {
                auto [inserted, ok] = audio_cache_.emplace(clip.source_path,
                                                           audio::container::read_wav(clip.source_path));
                (void)ok;
                source = &inserted->second;
            }

            // Overlap of this clip with the render window, in timeline time.
            const std::int64_t overlap_start = std::max(start_us, clip.position_us);
            const std::int64_t overlap_end = std::min(end_us, clip_end);
            if (overlap_end <= overlap_start) continue;

            // Map to source sample indices (speed is not applied to audio).
            const std::int64_t src_start_us = clip.in_point_us + (overlap_start - clip.position_us);
            const std::int64_t src_end_us = clip.in_point_us + (overlap_end - clip.position_us);
            const std::int64_t src_rate = source->sample_rate;
            std::int64_t src_start_sample = static_cast<std::int64_t>((src_start_us / 1'000'000.0) * src_rate);
            std::int64_t src_end_sample = static_cast<std::int64_t>((src_end_us / 1'000'000.0) * src_rate);
            src_start_sample = std::clamp<std::int64_t>(src_start_sample, 0, static_cast<std::int64_t>(source->num_samples()));
            src_end_sample = std::clamp<std::int64_t>(src_end_sample, 0, static_cast<std::int64_t>(source->num_samples()));
            if (src_end_sample <= src_start_sample) continue;

            audio::AudioBuffer segment;
            segment.format = audio::SampleFormat::F32;
            segment.sample_rate = source->sample_rate;
            segment.num_channels = source->num_channels;
            segment.planes.resize(source->num_channels);
            for (std::uint32_t c = 0; c < source->num_channels; ++c) {
                segment.planes[c].assign(source->planes[c].begin() + src_start_sample,
                                         source->planes[c].begin() + src_end_sample);
                if (clip.volume != 1.0f) {
                    for (auto& s : segment.planes[c]) s *= clip.volume;
                }
            }

            if (segment.sample_rate != sample_rate) {
                audio::Resampler resampler(segment.sample_rate, sample_rate, segment.num_channels);
                segment = resampler.process(segment);
            }
            if (segment.num_channels != channels) {
                audio::ChannelRouterMatrix matrix = (segment.num_channels == 2 && channels == 1)
                    ? audio::ChannelRouterMatrix::stereo_to_mono()
                    : (segment.num_channels == 1 && channels == 2)
                        ? audio::ChannelRouterMatrix::mono_to_stereo()
                        : audio::ChannelRouterMatrix::identity(std::min(segment.num_channels, channels));
                segment = audio::route_channels(segment, matrix);
                if (segment.num_channels != channels) {
                    // Fall back: pad/truncate channels directly.
                    segment.planes.resize(channels, std::vector<float>(segment.num_samples(), 0.0f));
                    segment.num_channels = channels;
                }
            }

            // Place this segment into a full-length (silent elsewhere) layer
            // so trekker::audio::mix() can combine everything positionally.
            audio::AudioBuffer layer = audio::AudioBuffer::silent(sample_rate, channels, out_samples);
            const std::size_t dest_offset = static_cast<std::size_t>(
                std::llround(((overlap_start - start_us) / 1'000'000.0) * sample_rate));
            for (std::uint32_t c = 0; c < channels; ++c) {
                for (std::size_t i = 0; i < segment.num_samples() && dest_offset + i < out_samples; ++i) {
                    layer.planes[c][dest_offset + i] = segment.planes[c][i];
                }
            }
            layers.push_back(std::move(layer));
        }
    }

    if (layers.empty()) {
        return audio::AudioBuffer::silent(sample_rate, channels, out_samples);
    }
    return audio::mix(layers);
}

} // namespace movie_editor
} // namespace trekker
