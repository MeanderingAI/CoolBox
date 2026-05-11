#include "video_display.hpp"
#include <algorithm>
#include <cstring>
#include <sstream>
#include <stdexcept>

namespace app_assets {
namespace video_display {

// ── FrameBuffer ───────────────────────────────────────────────────────────────

FrameBuffer::FrameBuffer(std::size_t w, std::size_t h)
    : width_(w), height_(h), data_(w * h * 4, 0) {}

void FrameBuffer::resize(std::size_t w, std::size_t h) {
    width_ = w; height_ = h;
    data_.assign(w * h * 4, 0);
}

Rgba FrameBuffer::pixel(std::size_t x, std::size_t y) const {
    const std::size_t off = (y * width_ + x) * 4;
    return {data_[off], data_[off+1], data_[off+2], data_[off+3]};
}

void FrameBuffer::set_pixel(std::size_t x, std::size_t y, Rgba c) {
    const std::size_t off = (y * width_ + x) * 4;
    data_[off]   = c.r; data_[off+1] = c.g;
    data_[off+2] = c.b; data_[off+3] = c.a;
}

void FrameBuffer::clear(Rgba col) {
    for (std::size_t i = 0; i < width_ * height_; ++i) {
        data_[i*4  ] = col.r; data_[i*4+1] = col.g;
        data_[i*4+2] = col.b; data_[i*4+3] = col.a;
    }
}

void FrameBuffer::blit_rgb24(const std::uint8_t* src,
                             std::size_t sw, std::size_t sh) {
    for (std::size_t dy = 0; dy < height_; ++dy) {
        const std::size_t sy = dy * sh / height_;
        for (std::size_t dx = 0; dx < width_; ++dx) {
            const std::size_t sx  = dx * sw / width_;
            const std::size_t si  = (sy * sw + sx) * 3;
            const std::size_t di  = (dy * width_ + dx) * 4;
            data_[di  ] = src[si  ];
            data_[di+1] = src[si+1];
            data_[di+2] = src[si+2];
            data_[di+3] = 255;
        }
    }
}

void FrameBuffer::blit_rgba32(const std::uint8_t* src,
                              std::size_t sw, std::size_t sh) {
    for (std::size_t dy = 0; dy < height_; ++dy) {
        const std::size_t sy = dy * sh / height_;
        for (std::size_t dx = 0; dx < width_; ++dx) {
            const std::size_t sx = dx * sw / width_;
            const std::size_t si = (sy * sw + sx) * 4;
            const std::size_t di = (dy * width_ + dx) * 4;
            std::memcpy(&data_[di], &src[si], 4);
        }
    }
}

// ── Overlay layout ────────────────────────────────────────────────────────────

std::vector<OverlayDrawCmd> layout_overlays(const std::vector<Overlay>& overlays,
                                            std::size_t fw, std::size_t fh,
                                            int cw, int ch) {
    std::vector<OverlayDrawCmd> cmds;
    for (const auto& ov : overlays) {
        if (!ov.visible || ov.text.empty()) continue;
        const int text_w = static_cast<int>(ov.text.size()) * cw;
        const int text_h = ch;
        int x = ov.offset_x, y = ov.offset_y;
        switch (ov.anchor) {
            case OverlayAnchor::TopRight:
                x = static_cast<int>(fw) - text_w - ov.offset_x; break;
            case OverlayAnchor::BottomLeft:
                y = static_cast<int>(fh) - text_h - ov.offset_y; break;
            case OverlayAnchor::BottomRight:
                x = static_cast<int>(fw) - text_w - ov.offset_x;
                y = static_cast<int>(fh) - text_h - ov.offset_y; break;
            case OverlayAnchor::Center:
                x = (static_cast<int>(fw) - text_w) / 2;
                y = (static_cast<int>(fh) - text_h) / 2; break;
            default: break;
        }
        cmds.push_back({x, y, text_w, text_h, ov.text, ov.style});
    }
    return cmds;
}

// ── VideoPlayer ───────────────────────────────────────────────────────────────

static std::int64_t fps_to_period_us(double fps) {
    return (fps > 0.0) ? static_cast<std::int64_t>(1'000'000.0 / fps) : 33'333;
}

VideoPlayer::VideoPlayer(PlayerConfig cfg)
    : cfg_(cfg)
    , frame_period_us_(fps_to_period_us(cfg.fps))
{}

void VideoPlayer::play()  { state_.store(PlaybackState::Playing); }
void VideoPlayer::pause() { state_.store(PlaybackState::Paused);  }
void VideoPlayer::stop()  {
    state_.store(PlaybackState::Stopped);
    position_us_.store(0);
    last_frame_pts_ = -1;
    accumulated_us_ = 0;
}
void VideoPlayer::seek(std::int64_t pts_us) {
    position_us_.store(std::max(std::int64_t(0), pts_us));
    last_frame_pts_ = -1;
}
void VideoPlayer::set_playback_rate(double rate) {
    rate_.store(std::max(0.0, std::min(8.0, rate)));
}
void VideoPlayer::set_volume(float v) {
    volume_.store(std::max(0.f, std::min(1.f, v)));
}

bool VideoPlayer::tick(std::int64_t delta_us, FrameBuffer& fb) {
    if (state_.load() != PlaybackState::Playing) return false;

    accumulated_us_ += static_cast<std::int64_t>(delta_us * rate_.load());
    if (accumulated_us_ < frame_period_us_) return false;
    accumulated_us_ -= frame_period_us_;

    std::int64_t pos = position_us_.load() + frame_period_us_;

    // Loop / end handling.
    if (cfg_.duration_us > 0 && pos >= cfg_.duration_us) {
        if (cfg_.loop) pos = 0;
        else { stop(); return false; }
    }
    position_us_.store(pos);

    // Update timecode overlay.
    if (timecode_overlay_ && !overlays_.empty()) {
        const std::int64_t s = pos / 1'000'000;
        const std::int64_t ms = (pos % 1'000'000) / 1000;
        std::ostringstream oss;
        oss << s / 3600 << ":"
            << (s % 3600) / 60 << ":"
            << s % 60 << "."
            << ms / 10;
        overlays_.back().text = oss.str();
    }

    if (frame_provider_) frame_provider_(pos, fb);
    last_frame_pts_ = pos;
    return true;
}

void VideoPlayer::enable_timecode_overlay(bool enable) {
    timecode_overlay_ = enable;
    if (enable) {
        Overlay ov;
        ov.text   = "00:00:00.00";
        ov.anchor = OverlayAnchor::BottomRight;
        ov.style.foreground = {255, 255, 255, 200};
        overlays_.push_back(ov);
    } else {
        overlays_.clear();
    }
}

} // namespace video_display
} // namespace app_assets
