#pragma once
#include <cstddef>
#include <cstdint>
#include <string>

namespace music_studio {

// ── VideoView ─────────────────────────────────────────────────────────────────
// Video-display panel backed by video_display_lib and video_codec (when linked).
//
// Features:
//   • Frame-buffer display surface (1280×720 default)
//   • Transport controls: play, pause, stop, seek slider
//   • Playback speed slider (0.25×, 0.5×, 1×, 2×, 4×)
//   • Volume slider (audio track hint)
//   • Loop toggle
//   • Timecode overlay toggle
//   • Custom text overlay editor (add / remove / reposition)
//   • Pixel-format / zoom info panel

class VideoView {
public:
    VideoView();
    ~VideoView();

    void init();
    void tick(std::int64_t delta_us);  // drives VideoPlayer::tick

    // ── Source ────────────────────────────────────────────────────────────
    void load(const std::string& path);

    // ── Transport controls ────────────────────────────────────────────────
    void play();
    void pause();
    void stop();
    void seek(std::int64_t pts_us);
    void set_playback_rate(double rate);
    void set_volume(float v);
    void set_loop(bool loop);

    // ── Overlays ──────────────────────────────────────────────────────────
    void enable_timecode_overlay(bool en);
    void add_text_overlay(const std::string& text, int x, int y);
    void clear_overlays();

    // ── Status ────────────────────────────────────────────────────────────
    bool        is_playing()  const;
    std::int64_t position_us() const;
    std::int64_t duration_us() const;
    std::size_t  fb_width()    const;
    std::size_t  fb_height()   const;

    static constexpr std::size_t kDefaultWidth  = 1280;
    static constexpr std::size_t kDefaultHeight = 720;

private:
    struct Impl;
    Impl* impl_;
};

} // namespace music_studio
