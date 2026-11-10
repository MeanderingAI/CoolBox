#include "../headers/movie_editor_core.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include "fonts.hpp"

namespace trekker {
namespace movie_editor {

namespace {

const graphics::fonts::FontFace& title_font() {
    thread_local graphics::fonts::FontFace font;
    if (!font.is_loaded()) {
        std::vector<std::string> paths;
#ifdef _WIN32
        const char* windows = std::getenv("WINDIR");
        const std::string root = windows ? windows : "C:\\Windows";
        paths = {root + "\\Fonts\\segoeui.ttf", root + "\\Fonts\\arial.ttf"};
#elif defined(__APPLE__)
        paths = {"/System/Library/Fonts/Supplemental/Arial.ttf",
                 "/Library/Fonts/Arial.ttf", "/System/Library/Fonts/Supplemental/Verdana.ttf"};
#else
        paths = {"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
                 "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf",
                 "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf"};
#endif
        for (const auto& path : paths) if (font.load_from_file(path)) return font;
        throw std::runtime_error("Text rendering requires an installed TrueType font (Segoe UI, Arial, DejaVu Sans, or Liberation Sans).");
    }
    return font;
}

graphics::Color text_color(std::uint32_t rgb) {
    return {static_cast<std::uint8_t>(rgb >> 16), static_cast<std::uint8_t>(rgb >> 8),
            static_cast<std::uint8_t>(rgb), 255};
}

void compose_text(video::VideoFrame& frame, const timeline::Clip& clip, std::int64_t pts_us) {
    const auto& style = *clip.text;
    validate_text_style(style);
    const auto& font = title_font();
    const double elapsed = static_cast<double>(pts_us - clip.position_us);
    const double remaining = static_cast<double>(clip.timeline_end_us() - pts_us);
    const double alpha = std::clamp(std::min(
        style.fade_in_us ? elapsed / style.fade_in_us : 1.0,
        style.fade_out_us ? remaining / style.fade_out_us : 1.0), 0.0, 1.0);
    if (alpha == 0) return;
    graphics::Canvas canvas(static_cast<int>(frame.width), static_cast<int>(frame.height));
    auto& plane = frame.planes[0];
    const auto background = text_color(style.background_color);
    const double opacity = style.background ? style.background_opacity : 0;
    for (std::size_t y = 0; y < frame.height; ++y)
        for (std::size_t x = 0; x < frame.width; ++x)
            canvas.set_pixel(static_cast<int>(x), static_cast<int>(y),
                {static_cast<std::uint8_t>(std::lround(plane.at(x * 3, y) * (1 - opacity) + background.r * opacity)),
                 static_cast<std::uint8_t>(std::lround(plane.at(x * 3 + 1, y) * (1 - opacity) + background.g * opacity)),
                 static_cast<std::uint8_t>(std::lround(plane.at(x * 3 + 2, y) * (1 - opacity) + background.b * opacity)), 255});
    const float pixels = static_cast<float>(style.font_size * frame.height / 720.0);
    const auto bounds = text_layout(style, frame.width, frame.height);
    auto foreground = text_color(style.color);
    foreground.a = static_cast<std::uint8_t>(std::lround(style.opacity * 255));
    if (!graphics::fonts::TextRenderer::draw_text(canvas, font, bounds.x, bounds.y, style.text, foreground, pixels))
        throw std::runtime_error("TrueType title rendering failed.");
    // Fade the complete layer once, including the title background.
    for (std::size_t row = 0; row < frame.height; ++row)
        for (std::size_t col = 0; col < frame.width; ++col) {
            const auto color = canvas.get_pixel(static_cast<int>(col), static_cast<int>(row));
            const std::uint8_t channels[] = {color.r, color.g, color.b};
            for (std::size_t c = 0; c < 3; ++c) {
                auto& dst = plane.at(col * 3 + c, row);
                dst = static_cast<std::uint8_t>(std::lround(dst * (1 - alpha) + channels[c] * alpha));
            }
        }
}

struct FitBounds {
    std::size_t width, height, x, y;
};

FitBounds fit_bounds(std::size_t width, std::size_t height, std::size_t out_w, std::size_t out_h) {
    const double scale = std::min(static_cast<double>(out_w) / width, static_cast<double>(out_h) / height);
    const auto fitted_w = std::min(out_w, std::max<std::size_t>(1, static_cast<std::size_t>(width * scale)));
    const auto fitted_h = std::min(out_h, std::max<std::size_t>(1, static_cast<std::size_t>(height * scale)));
    return {fitted_w, fitted_h, (out_w - fitted_w) / 2, (out_h - fitted_h) / 2};
}

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
    const auto fit = fit_bounds(tex.width, tex.height, out_w, out_h);
    for (std::size_t y = 0; y < fit.height; ++y) {
        const std::size_t sy = std::min<std::size_t>(tex.height - 1, (y * static_cast<std::size_t>(tex.height)) / fit.height);
        for (std::size_t x = 0; x < fit.width; ++x) {
            const std::size_t sx = std::min<std::size_t>(tex.width - 1, (x * static_cast<std::size_t>(tex.width)) / fit.width);
            const std::size_t src_idx = (sy * static_cast<std::size_t>(tex.width) + sx) * static_cast<std::size_t>(tex.channels);
            std::uint8_t r, g, b;
            if (tex.channels >= 3) {
                r = tex.pixels[src_idx + 0];
                g = tex.pixels[src_idx + 1];
                b = tex.pixels[src_idx + 2];
            } else {
                r = g = b = tex.pixels[src_idx];
            }
            plane.at((x + fit.x) * 3 + 0, y + fit.y) = r;
            plane.at((x + fit.x) * 3 + 1, y + fit.y) = g;
            plane.at((x + fit.x) * 3 + 2, y + fit.y) = b;
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

// Same idea as resolve_frame_file() but for ImportedVideo assets, whose
// frames live in memory (decoded_frames) rather than on disk, and whose
// per-frame timing may be variable (GIF delay times) rather than constant
// fps. frame_start_us is sorted ascending, so a binary search finds the
// last frame whose start time has already passed.
const video::VideoFrame* resolve_decoded_frame(const MediaAsset& asset, std::int64_t source_us) {
    if (asset.decoded_frames.empty()) return nullptr;
    const auto& starts = asset.frame_start_us;
    auto it = std::upper_bound(starts.begin(), starts.end(), source_us);
    std::size_t index = it == starts.begin() ? 0 : static_cast<std::size_t>((it - starts.begin()) - 1);
    index = std::min(index, asset.decoded_frames.size() - 1);
    return &asset.decoded_frames[index];
}

// Nearest-neighbour resize of an already-decoded RGB24 VideoFrame (as
// opposed to blit_to_frame() above, which converts from a graphics::Texture
// — ImportedVideo assets skip the texture-loading path entirely since
// their frames are already in-memory VideoFrames).
video::VideoFrame resize_video_frame(const video::VideoFrame& src, std::size_t out_w, std::size_t out_h) {
    if (src.width == out_w && src.height == out_h && src.format == video::PixelFormat::RGB24) return src;
    const video::VideoFrame rgb = src.format == video::PixelFormat::RGB24 ? src : video::convert_format(src, video::PixelFormat::RGB24);
    video::VideoFrame out = video::VideoFrame::blank(video::PixelFormat::RGB24, out_w, out_h);
    const video::Plane& in_plane = rgb.planes[0];
    video::Plane& out_plane = out.planes[0];
    const auto fit = fit_bounds(rgb.width, rgb.height, out_w, out_h);
    for (std::size_t y = 0; y < fit.height; ++y) {
        const std::size_t sy = std::min<std::size_t>(rgb.height - 1, (y * rgb.height) / fit.height);
        for (std::size_t x = 0; x < fit.width; ++x) {
            const std::size_t sx = std::min<std::size_t>(rgb.width - 1, (x * rgb.width) / fit.width);
            out_plane.at((x + fit.x) * 3 + 0, y + fit.y) = in_plane.at(sx * 3 + 0, sy);
            out_plane.at((x + fit.x) * 3 + 1, y + fit.y) = in_plane.at(sx * 3 + 1, sy);
            out_plane.at((x + fit.x) * 3 + 2, y + fit.y) = in_plane.at(sx * 3 + 2, sy);
        }
    }
    return out;
}

} // namespace

TextLayout text_layout(const TextStyle& style, std::size_t width, std::size_t height) {
    validate_text_style(style);
    if (!width || !height || width > 16384 || height > 16384)
        throw std::invalid_argument("Text layout dimensions must be between 1 and 16384.");
    const auto bounds = graphics::fonts::TextRenderer::measure_text(title_font(), style.text,
        static_cast<float>(style.font_size * height / 720.0));
    return {static_cast<int>(std::lround(style.x * std::max(0, static_cast<int>(width) - bounds.width))),
            static_cast<int>(std::lround(style.y * std::max(0, static_cast<int>(height) - bounds.height))),
            bounds.width, bounds.height};
}

void ensure_text_font_available() {
    (void)title_font();
}

video::VideoFrame EditorProject::render_frame_at(std::int64_t pts_us, std::size_t out_width, std::size_t out_height) const {
    return render_frame_excluding(pts_us, out_width, out_height, 0, 0);
}

video::VideoFrame EditorProject::render_text_preview(const TextStyle& style, std::int64_t pts_us,
    std::size_t width, std::size_t height, std::uint64_t exclude_track, std::uint64_t exclude_clip) const {
    validate_text_style(style);
    if (exclude_track && exclude_clip)
        return render_frame_excluding(pts_us, width, height, exclude_track, exclude_clip, &style);
    auto frame = render_frame_excluding(pts_us, width, height, exclude_track, exclude_clip);
    timeline::Clip clip;
    clip.text = style;
    clip.text->fade_in_us = 0;
    clip.text->fade_out_us = 0;
    clip.out_point_us = 1;
    compose_text(frame, clip, 0);
    return frame;
}

video::VideoFrame EditorProject::render_frame_excluding(std::int64_t pts_us, std::size_t out_width,
    std::size_t out_height, std::uint64_t exclude_track, std::uint64_t exclude_clip, const TextStyle* replacement) const {
    if (!out_width || !out_height || out_width > 16384 || out_height > 16384)
        throw std::invalid_argument("Frame dimensions must be between 1 and 16384.");
    auto output = black_frame(out_width, out_height);
    const auto& tracks = timeline_.tracks();
    for (const auto& track : tracks) {
        if (track.type() != timeline::TrackType::Video || track.muted()) continue;
        const timeline::Clip* clip = track.clip_at(pts_us);
        if (!clip || clip->muted) continue;
        if (track.id() == exclude_track && clip->id == exclude_clip) {
            if (replacement) {
                auto draft = *clip;
                draft.text = *replacement;
                draft.text->fade_in_us = draft.text->fade_out_us = 0;
                compose_text(output, draft, pts_us);
            }
            continue;
        }
        if (clip->text) {
            compose_text(output, *clip, pts_us);
            continue;
        }

        const MediaAsset* asset = bin_.find(clip->source_path);
        if (!asset || (asset->type != MediaType::ImageSequence && asset->type != MediaType::StillImage &&
                      asset->type != MediaType::ImportedVideo)) continue;

        const std::int64_t elapsed_timeline_us = pts_us - clip->position_us;
        const std::int64_t source_us = clip->in_point_us +
            static_cast<std::int64_t>(elapsed_timeline_us * clip->speed);

        if (asset->type == MediaType::ImportedVideo && !asset->decoded_frames.empty()) {
            const video::VideoFrame* frame = resolve_decoded_frame(*asset, source_us);
            if (!frame) continue;
            output = resize_video_frame(*frame, out_width, out_height);
            continue;
        }

        const std::string* frame_file = resolve_frame_file(*asset, source_us);
        if (!frame_file) continue;

        // Disk-backed FFmpeg frames must not accumulate an entire movie in RAM.
        if (frame_cache_.size() >= 8 && frame_cache_.find(*frame_file) == frame_cache_.end()) frame_cache_.clear();
        const graphics::Texture* tex = load_cached_texture(*frame_file, frame_cache_);
        if (!tex) continue;

        output = blit_to_frame(*tex, out_width, out_height);
    }
    return output;
}

audio::AudioBuffer EditorProject::render_audio_mix(std::int64_t start_us, std::int64_t end_us,
                                                   std::uint32_t sample_rate, std::uint32_t channels) const {
    if (sample_rate == 0 || channels == 0) throw std::invalid_argument("Audio sample rate and channel count must be positive.");
    const std::int64_t window_us = std::max<std::int64_t>(0, end_us - start_us);
    const std::size_t out_samples = static_cast<std::size_t>(
        std::llround((window_us / 1'000'000.0) * sample_rate));

    std::vector<audio::AudioBuffer> layers;

    for (const auto& track : timeline_.tracks()) {
        if (track.muted()) continue;

        for (const auto& clip : track.clips()) {
            if (clip.muted) continue;
            if (!std::isfinite(clip.speed) || clip.speed <= 0)
                throw std::invalid_argument("Audio clip speed must be finite and positive.");
            const std::int64_t clip_end = clip.timeline_end_us();
            if (clip_end <= start_us || clip.position_us >= end_us) continue;

            const MediaAsset* asset = bin_.find(clip.source_path);
            if (!asset) continue;
            const bool embedded_audio = track.type() == timeline::TrackType::Video &&
                asset->type == MediaType::ImportedVideo && !asset->audio_path.empty();
            if (!embedded_audio && (track.type() != timeline::TrackType::Audio || asset->type != MediaType::Audio)) continue;

            const auto cache_it = audio_cache_.find(clip.source_path);
            const audio::AudioBuffer* source = nullptr;
            if (cache_it != audio_cache_.end()) {
                source = &cache_it->second;
            } else {
                auto [inserted, ok] = audio_cache_.emplace(clip.source_path,
                                                           audio::container::read_wav(embedded_audio ? asset->audio_path : clip.source_path));
                (void)ok;
                source = &inserted->second;
            }

            // Overlap of this clip with the render window, in timeline time.
            const std::int64_t overlap_start = std::max(start_us, clip.position_us);
            const std::int64_t overlap_end = std::min(end_us, clip_end);
            if (overlap_end <= overlap_start) continue;

            const auto count = static_cast<std::size_t>(std::llround(
                (overlap_end - overlap_start) * sample_rate / 1'000'000.0));
            auto segment = audio::AudioBuffer::silent(sample_rate, source->num_channels, count);
            const double source_start = (clip.in_point_us +
                (overlap_start - clip.position_us) * static_cast<double>(clip.speed)) *
                source->sample_rate / 1'000'000.0;
            const double step = source->sample_rate * static_cast<double>(clip.speed) / sample_rate;
            // Absolute source coordinates keep separate playback/export chunks continuous.
            for (std::size_t i = 0; i < count; ++i) {
                const double position = source_start + i * step;
                if (position < 0 || position >= source->num_samples()) continue;
                const auto first = static_cast<std::size_t>(position);
                const auto second = std::min(first + 1, source->num_samples() - 1);
                const float fraction = static_cast<float>(position - first);
                for (std::uint32_t c = 0; c < source->num_channels; ++c)
                    segment.planes[c][i] = (source->planes[c][first] * (1 - fraction) +
                                            source->planes[c][second] * fraction) * clip.volume;
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
