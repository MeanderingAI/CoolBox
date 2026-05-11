#include "motion_analysis.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>

namespace trekker {
namespace motion {

// ── FlowField helpers ─────────────────────────────────────────────────────────

float FlowField::magnitude_at(std::size_t x, std::size_t y) const {
    const auto& v = at(x, y);
    return std::sqrt(v.dx * v.dx + v.dy * v.dy);
}

float FlowField::mean_magnitude() const {
    if (flow.empty()) return 0.f;
    float sum = 0.f;
    for (const auto& v : flow) sum += std::sqrt(v.dx * v.dx + v.dy * v.dy);
    return sum / static_cast<float>(flow.size());
}

// ── Optical flow (gradient-based, simplified Farneback approximation) ──────────

static const VideoFrame& to_gray(const VideoFrame& f, VideoFrame& tmp) {
    if (f.format == PixelFormat::GRAY8) return f;
    tmp = convert_format(f, PixelFormat::GRAY8);
    return tmp;
}

FlowField optical_flow(const VideoFrame& prev_in, const VideoFrame& curr_in,
                       int window_size, int iterations) {
    VideoFrame prev_tmp, curr_tmp;
    const auto& prev = to_gray(prev_in, prev_tmp);
    const auto& curr = to_gray(curr_in, curr_tmp);

    if (prev.width != curr.width || prev.height != curr.height)
        throw std::invalid_argument("optical_flow: frames must be the same size");

    const int W = static_cast<int>(prev.width);
    const int H = static_cast<int>(prev.height);
    const int hw = window_size / 2;

    FlowField field;
    field.width  = prev.width;
    field.height = prev.height;
    field.flow.resize(W * H, {0.f, 0.f});

    // Iterative Lucas-Kanade per pixel.
    for (int iter = 0; iter < iterations; ++iter) {
        for (int y = hw; y < H - hw; ++y)
            for (int x = hw; x < W - hw; ++x) {
                float Ixx = 0, Ixy = 0, Iyy = 0, Ixt = 0, Iyt = 0;
                for (int dy = -hw; dy <= hw; ++dy)
                    for (int dx = -hw; dx <= hw; ++dx) {
                        // Spatial gradients via central differences
                        const int nx = std::max(1, std::min(W-2, x + dx));
                        const int ny = std::max(1, std::min(H-2, y + dy));
                        const float Ix = (curr.planes[0].at(nx+1, ny) -
                                          curr.planes[0].at(nx-1, ny)) * 0.5f;
                        const float Iy = (curr.planes[0].at(nx, ny+1) -
                                          curr.planes[0].at(nx, ny-1)) * 0.5f;
                        const float It = static_cast<float>(curr.planes[0].at(nx, ny)) -
                                         static_cast<float>(prev.planes[0].at(nx, ny));
                        Ixx += Ix * Ix;
                        Ixy += Ix * Iy;
                        Iyy += Iy * Iy;
                        Ixt += Ix * It;
                        Iyt += Iy * It;
                    }
                const float det = Ixx * Iyy - Ixy * Ixy;
                if (std::abs(det) > 1e-3f) {
                    field.flow[y * W + x].dx = -(Iyy * Ixt - Ixy * Iyt) / det;
                    field.flow[y * W + x].dy = -(Ixx * Iyt - Ixy * Ixt) / det;
                }
            }
    }
    return field;
}

// ── Motion vectors ────────────────────────────────────────────────────────────

std::vector<MotionVector> estimate_motion_vectors(const VideoFrame& prev_in,
                                                  const VideoFrame& curr_in,
                                                  int block_size,
                                                  int search_radius) {
    VideoFrame pt, ct;
    const auto& prev = to_gray(prev_in, pt);
    const auto& curr = to_gray(curr_in, ct);
    const int W = static_cast<int>(curr.width);
    const int H = static_cast<int>(curr.height);
    std::vector<MotionVector> mvs;
    for (int by = 0; by < H; by += block_size)
        for (int bx = 0; bx < W; bx += block_size) {
            MotionVector best; best.block_x = bx; best.block_y = by;
            best.sad = std::numeric_limits<long long>::max();
            for (int dy = -search_radius; dy <= search_radius; ++dy)
                for (int dx = -search_radius; dx <= search_radius; ++dx) {
                    long long sad = 0;
                    for (int y = 0; y < block_size && by+y < H; ++y)
                        for (int x = 0; x < block_size && bx+x < W; ++x) {
                            const int nx = std::max(0, std::min(W-1, bx+x+dx));
                            const int ny = std::max(0, std::min(H-1, by+y+dy));
                            sad += std::abs(curr.planes[0].at(bx+x, by+y) -
                                            prev.planes[0].at(nx, ny));
                        }
                    if (sad < best.sad) { best.sad = sad; best.dx = dx; best.dy = dy; }
                }
            mvs.push_back(best);
        }
    return mvs;
}

// ── Scene cut detection ───────────────────────────────────────────────────────

bool detect_scene_cut(const VideoFrame& prev_in, const VideoFrame& curr_in,
                      float threshold) {
    VideoFrame pt, ct;
    const auto& prev = to_gray(prev_in, pt);
    const auto& curr = to_gray(curr_in, ct);
    const std::size_t n = prev.planes[0].data.size();
    if (n == 0) return false;
    long long diff = 0;
    for (std::size_t i = 0; i < n; ++i)
        diff += std::abs(static_cast<int>(curr.planes[0].data[i]) -
                         static_cast<int>(prev.planes[0].data[i]));
    return (diff / static_cast<float>(n)) / 255.f >= threshold;
}

SceneDetector::SceneDetector(Config cfg) : cfg_(cfg) {}

bool SceneDetector::process(const VideoFrame& frame) {
    if (!has_prev_) {
        prev_ = frame; has_prev_ = true; return false;
    }
    VideoFrame pt, ct;
    const auto& prev = to_gray(prev_, pt);
    const auto& curr = to_gray(frame, ct);
    const std::size_t n = prev.planes[0].data.size();
    float diff_frac = 0.f;
    if (n > 0) {
        long long diff = 0;
        for (std::size_t i = 0; i < n; ++i)
            diff += std::abs(static_cast<int>(curr.planes[0].data[i]) -
                             static_cast<int>(prev.planes[0].data[i]));
        diff_frac = (diff / static_cast<float>(n)) / 255.f;
    }
    diff_history_.push_back(diff_frac);
    if (diff_history_.size() > cfg_.history_frames) diff_history_.erase(diff_history_.begin());

    const float mean_diff = diff_history_.empty() ? 0.f :
        std::accumulate(diff_history_.begin(), diff_history_.end(), 0.f) / diff_history_.size();
    const float adaptive_threshold = std::max(cfg_.base_threshold, mean_diff * cfg_.sensitivity);
    prev_ = frame;
    return diff_frac >= adaptive_threshold;
}

void SceneDetector::reset() { has_prev_ = false; diff_history_.clear(); }

// ── Block activity ────────────────────────────────────────────────────────────

std::vector<BlockActivity> compute_block_activity(const VideoFrame& frame,
                                                  const FlowField& flow,
                                                  int block_size) {
    VideoFrame tmp;
    const auto& gray = to_gray(frame, tmp);
    const int W = static_cast<int>(gray.width);
    const int H = static_cast<int>(gray.height);
    std::vector<BlockActivity> result;
    for (int by = 0; by < H; by += block_size)
        for (int bx = 0; bx < W; bx += block_size) {
            float sum = 0.f, sum_sq = 0.f, mag = 0.f; int cnt = 0;
            for (int y = 0; y < block_size && by+y < H; ++y)
                for (int x = 0; x < block_size && bx+x < W; ++x) {
                    const float v = gray.planes[0].at(bx+x, by+y);
                    sum += v; sum_sq += v * v;
                    if (bx+x < static_cast<int>(flow.width) &&
                        by+y < static_cast<int>(flow.height))
                        mag += flow.magnitude_at(bx+x, by+y);
                    ++cnt;
                }
            const float mean = sum / cnt;
            result.push_back({static_cast<std::size_t>(bx), static_cast<std::size_t>(by),
                              sum_sq / cnt - mean * mean,
                              mag / cnt});
        }
    return result;
}

} // namespace motion
} // namespace trekker
