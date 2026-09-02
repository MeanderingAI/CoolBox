#ifndef COOLBOX_APP_ASSETS_VIDEO_DISPLAY_HPP
#define COOLBOX_APP_ASSETS_VIDEO_DISPLAY_HPP

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace app_assets {
namespace video_display {

// ── Pixel / colour helpers ────────────────────────────────────────────────────

struct Rgba {
    std::uint8_t r = 0, g = 0, b = 0, a = 255;
};

// ── FrameBuffer ───────────────────────────────────────────────────────────────
// Software RGBA32 back-buffer that the display surface renders into.
// The GUI toolkit (SDL, GTK, Qt, etc.) consumes raw_data().

class FrameBuffer {
public:
    FrameBuffer() = default;
    FrameBuffer(std::size_t width, std::size_t height);

    std::size_t width()  const { return width_;  }
    std::size_t height() const { return height_; }
    std::size_t stride() const { return width_ * 4; } // RGBA bytes per row

    // Raw RGBA pixel data — one byte per channel, row-major.
    const std::vector<std::uint8_t>& raw_data() const { return data_; }
    std::vector<std::uint8_t>&       raw_data()       { return data_; }

    Rgba  pixel(std::size_t x, std::size_t y) const;
    void  set_pixel(std::size_t x, std::size_t y, Rgba c);

    // Fill entire buffer with a colour.
    void clear(Rgba colour = {0, 0, 0, 255});

    // Resize (discards content).
    void resize(std::size_t w, std::size_t h);

    // Blit a decoded RGB24 video frame into this buffer (with optional scaling).
    // Converts from RGB24 row-by-row, no gamma correction.
    // If the frame is a different size it is stretched to fill the buffer.
    void blit_rgb24(const std::uint8_t* rgb_data,
                    std::size_t src_width, std::size_t src_height);

    // Blit an RGBA32 source buffer.
    void blit_rgba32(const std::uint8_t* rgba_data,
                     std::size_t src_width, std::size_t src_height);

private:
    std::size_t              width_  = 0;
    std::size_t              height_ = 0;
    std::vector<std::uint8_t> data_; // RGBA32, width*height*4 bytes
};

// ── Overlay ───────────────────────────────────────────────────────────────────
// A text/image overlay rendered on top of the frame buffer.

enum class OverlayAnchor { TopLeft, TopRight, BottomLeft, BottomRight, Center };

struct OverlayStyle {
    Rgba foreground = {255, 255, 255, 255};
    Rgba background = {0,   0,   0,   128};
    int  font_size  = 14;   // pt — hint for the GUI toolkit
};

struct Overlay {
    std::string   text;
    OverlayAnchor anchor   = OverlayAnchor::BottomLeft;
    int           offset_x = 8;
    int           offset_y = 8;
    OverlayStyle  style;
    bool          visible  = true;
};

// Composite all visible overlays onto a FrameBuffer by drawing a solid
// coloured rectangle for each line of text.  The actual font rendering
// is left to the GUI toolkit; this provides layout metadata.
struct OverlayDrawCmd {
    int           x, y, w, h;
    std::string   text;
    OverlayStyle  style;
};

std::vector<OverlayDrawCmd> layout_overlays(
    const std::vector<Overlay>& overlays,
    std::size_t fb_width, std::size_t fb_height,
    int char_width = 8, int char_height = 16);

// ── Playback state ────────────────────────────────────────────────────────────

enum class PlaybackState { Stopped, Playing, Paused };

// ── VideoPlayer ───────────────────────────────────────────────────────────────
// Manages playback position and drives frame requests via a callback.
// The actual decode is done by the caller (e.g. using video_codec).

struct PlayerConfig {
    std::size_t    display_width   = 1280;
    std::size_t    display_height  = 720;
    double         fps             = 30.0;
    bool           loop            = false;
    std::int64_t   duration_us     = 0;   // 0 = unknown / streaming
};

class VideoPlayer {
public:
    // Callback invoked when the player needs the frame at pts_us.
    // The callee should fill the FrameBuffer.
    using FrameProvider = std::function<void(std::int64_t pts_us, FrameBuffer&)>;

    explicit VideoPlayer(PlayerConfig cfg = {});

    // ── Slider / transport controls ───────────────────────────────────────
    void play();
    void pause();
    void stop();
    void seek(std::int64_t pts_us);           // jump to position
    void set_playback_rate(double rate);       // 0.5 = half speed, 2.0 = double
    void set_volume(float v);                  // display volume hint [0,1]
    void set_loop(bool loop) { cfg_.loop = loop; }
    void set_frame_provider(FrameProvider fp) { frame_provider_ = std::move(fp); }

    // ── Status ────────────────────────────────────────────────────────────
    PlaybackState   state()         const { return state_.load(); }
    std::int64_t    position_us()   const { return position_us_.load(); }
    double          playback_rate() const { return rate_.load(); }
    float           volume()        const { return volume_.load(); }
    std::int64_t    duration_us()   const { return cfg_.duration_us; }

    // ── Render tick ───────────────────────────────────────────────────────
    // Call once per display frame (e.g. every 16 ms for 60 Hz).
    // delta_us: wall-clock elapsed since last call.
    // Advances playback position and calls frame_provider if a new frame is due.
    // Returns true if the frame buffer was updated.
    bool tick(std::int64_t delta_us, FrameBuffer& fb);

    // Overlays managed by the player (subtitles, timecode, debug info).
    std::vector<Overlay>& overlays() { return overlays_; }

    // Convenience: add a timecode overlay (auto-updates on tick).
    void enable_timecode_overlay(bool enable);

private:
    PlayerConfig                cfg_;
    std::atomic<PlaybackState>  state_{ PlaybackState::Stopped };
    std::atomic<std::int64_t>   position_us_{ 0 };
    std::atomic<double>         rate_{ 1.0 };
    std::atomic<float>          volume_{ 1.f };
    std::int64_t                frame_period_us_;
    std::int64_t                accumulated_us_ = 0;
    std::int64_t                last_frame_pts_ = -1;
    FrameProvider               frame_provider_;
    std::vector<Overlay>        overlays_;
    bool                        timecode_overlay_ = false;
    mutable std::mutex          mtx_;
};

} // namespace video_display
} // namespace app_assets

#endif // COOLBOX_APP_ASSETS_VIDEO_DISPLAY_HPP
