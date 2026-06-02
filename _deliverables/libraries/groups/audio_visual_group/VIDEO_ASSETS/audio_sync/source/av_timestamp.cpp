#include "audio_sync.h"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace trekker {
namespace av_sync {

// ── SyncController ────────────────────────────────────────────────────────────

SyncController::SyncController() : SyncController(Config{}) {}

SyncController::SyncController(Config cfg) : cfg_(cfg) {}

void SyncController::report_audio_pts(std::int64_t pts_us, const MasterClock& clock) {
    last_drift_us_ = pts_us - clock.time_us();
    drift_history_.push_back(last_drift_us_);
    if (drift_history_.size() > cfg_.history_size)
        drift_history_.pop_front();
}

std::int64_t SyncController::drift_us() const { return last_drift_us_; }

float SyncController::correction_factor() const {
    if (drift_history_.empty()) return 1.f;
    const double avg = std::accumulate(drift_history_.begin(), drift_history_.end(), 0LL)
                       / static_cast<double>(drift_history_.size());
    if (std::abs(avg) < static_cast<double>(cfg_.dead_zone_us)) return 1.f;
    // Proportional correction: pull the rate toward eliminating avg drift over 1 second.
    // A sample_rate of 48000 means 48000 samples/s. avg μs of drift per second.
    const float delta = std::max<float>(-cfg_.max_correction_delta,
                        std::min<float>( cfg_.max_correction_delta,
                        static_cast<float>(-avg / 1'000'000.0)));
    return 1.f + delta;
}

void SyncController::reset() {
    drift_history_.clear();
    last_drift_us_ = 0;
}

// ── LipSyncAdjuster ───────────────────────────────────────────────────────────

bool LipSyncAdjuster::should_present(std::int64_t adjusted_pts_us,
                                     const MasterClock& clk,
                                     std::int64_t tolerance_us) const {
    const std::int64_t diff = adjusted_pts_us - clk.time_us();
    return std::abs(diff) <= tolerance_us;
}

// ── DriftDetector ─────────────────────────────────────────────────────────────

DriftDetector::DriftDetector(std::size_t ws) : window_size_(ws) {}

void DriftDetector::push(std::int64_t v, std::int64_t a) {
    samples_.emplace_back(v, a);
    if (samples_.size() > window_size_) samples_.pop_front();
}

double DriftDetector::drift_rate_us_per_s() const {
    if (samples_.size() < 2) return 0.0;
    // Simple linear regression of (video_pts → audio_pts - video_pts)
    const std::size_t n = samples_.size();
    double sx = 0, sy = 0, sxx = 0, sxy = 0;
    for (auto& [v, a] : samples_) {
        const double x = static_cast<double>(v);
        const double y = static_cast<double>(a - v);
        sx += x; sy += y; sxx += x*x; sxy += x*y;
    }
    const double dn = static_cast<double>(n);
    const double denom = dn * sxx - sx * sx;
    if (std::abs(denom) < 1e-9) return 0.0;
    // slope is Δ(audio-video drift) / Δ(video_pts_us); multiply by 1e6 to get per-second.
    return (dn * sxy - sx * sy) / denom * 1'000'000.0;
}

std::int64_t DriftDetector::current_offset_us() const {
    if (samples_.empty()) return 0;
    const auto& [v, a] = samples_.back();
    return a - v;
}

void DriftDetector::reset() { samples_.clear(); }

} // namespace av_sync
} // namespace trekker
