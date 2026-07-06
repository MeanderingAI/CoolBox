#ifndef TREKKER_VIDEO_ASSETS_AUDIO_SYNC_H
#define TREKKER_VIDEO_ASSETS_AUDIO_SYNC_H

#include <cstdint>
#include <deque>
#include <optional>

namespace trekker {
namespace av_sync {

// ── AV timestamp ──────────────────────────────────────────────────────────────

struct AvTimestamp {
    std::int64_t pts = 0;  // presentation timestamp (μs)
    std::int64_t dts = 0;  // decode timestamp (μs)

    static AvTimestamp from_pts(std::int64_t pts_us) { return {pts_us, pts_us}; }

    std::int64_t pts_ms() const { return pts / 1000; }
    std::int64_t dts_ms() const { return dts / 1000; }
};

// ── Clock ─────────────────────────────────────────────────────────────────────
// A monotonic clock that drives A/V synchronisation.
// In a real player this wraps a high-resolution system clock; here it is
// simulated with an explicit tick() call so the logic is unit-testable.

class MasterClock {
public:
    void set_time_us(std::int64_t us) { time_us_ = us; }
    std::int64_t time_us() const      { return time_us_; }
    void tick(std::int64_t delta_us)  { time_us_ += delta_us; }

private:
    std::int64_t time_us_ = 0;
};

// ── SyncController ────────────────────────────────────────────────────────────
// Measures the drift between an audio stream's PTS and the master clock and
// derives a correction factor for the audio resampler.
//
// Positive drift  → audio is ahead of video  → slow down audio (factor < 1).
// Negative drift  → audio is behind video     → speed up audio (factor > 1).

class SyncController {
public:
    struct Config {
        // How many drift measurements to average over.
        std::size_t history_size    = 32;
        // Drift magnitude (μs) below which no correction is applied.
        std::int64_t dead_zone_us   = 5'000;
        // Maximum correction factor deviation from 1.0.
        float max_correction_delta  = 0.05f;
    };

    SyncController();
    explicit SyncController(Config cfg);

    // Call when a new audio frame with the given PTS has been decoded.
    void report_audio_pts(std::int64_t pts_us, const MasterClock& clock);

    // Instantaneous drift: audio_pts − master_clock (μs).
    // Positive ⇒ audio ahead.
    std::int64_t drift_us() const;

    // Smoothed correction factor to apply to the resampler's output rate.
    // 1.0 = no correction.  < 1.0 = slow audio.  > 1.0 = speed up audio.
    float correction_factor() const;

    void reset();

private:
    Config cfg_;
    std::deque<std::int64_t> drift_history_;
    std::int64_t last_drift_us_ = 0;
};

// ── LipSyncAdjuster ───────────────────────────────────────────────────────────
// Applies a static user-configurable audio/video offset so that audio and
// video rendering are in subjective lip-sync.

class LipSyncAdjuster {
public:
    LipSyncAdjuster()  = default;

    // Positive offset_ms: delay audio (audio plays later than video).
    // Negative offset_ms: advance audio.
    void set_offset_ms(std::int64_t ms) { offset_us_ = ms * 1000; }
    std::int64_t offset_ms() const      { return offset_us_ / 1000; }
    std::int64_t offset_us() const      { return offset_us_; }

    // Returns the adjusted PTS that should be used for audio presentation.
    std::int64_t adjust_audio_pts(std::int64_t raw_pts_us) const {
        return raw_pts_us - offset_us_;
    }

    // Returns true when the audio frame with this (adjusted) PTS should be
    // presented, given the current master clock position.
    bool should_present(std::int64_t adjusted_pts_us, const MasterClock& clk,
                        std::int64_t tolerance_us = 20'000) const;

private:
    std::int64_t offset_us_ = 0;
};

// ── DriftDetector ─────────────────────────────────────────────────────────────
// Tracks long-term drift between audio and video streams expressed as a linear
// regression over a window of (video_pts, audio_pts) sample pairs.

class DriftDetector {
public:
    explicit DriftDetector(std::size_t window_size = 64);

    void push(std::int64_t video_pts_us, std::int64_t audio_pts_us);

    // Estimated drift rate in μs-per-second.  Positive ⇒ audio running fast.
    double drift_rate_us_per_s() const;

    // Estimated current offset (bias) in μs.
    std::int64_t current_offset_us() const;

    void reset();

private:
    std::size_t window_size_;
    std::deque<std::pair<std::int64_t, std::int64_t>> samples_;
};

} // namespace av_sync
} // namespace trekker

#endif // TREKKER_VIDEO_ASSETS_AUDIO_SYNC_H
