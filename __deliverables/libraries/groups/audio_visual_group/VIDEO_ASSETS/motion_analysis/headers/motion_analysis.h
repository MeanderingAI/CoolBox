#ifndef TREKKER_VIDEO_ASSETS_MOTION_ANALYSIS_H
#define TREKKER_VIDEO_ASSETS_MOTION_ANALYSIS_H

#include "video_codec.h"
#include <cstdint>
#include <vector>

namespace trekker {
namespace motion {

using namespace trekker::video;

// ── Optical flow (Lucas-Kanade / Farneback-style) ─────────────────────────────

struct FlowVector {
    float dx = 0.f;   // horizontal displacement in pixels
    float dy = 0.f;   // vertical displacement in pixels
};

// Dense optical flow field — one FlowVector per pixel.
struct FlowField {
    std::size_t               width  = 0;
    std::size_t               height = 0;
    std::vector<FlowVector>   flow;  // row-major [y * width + x]

    FlowVector& at(std::size_t x, std::size_t y) { return flow[y * width + x]; }
    const FlowVector& at(std::size_t x, std::size_t y) const { return flow[y * width + x]; }

    float magnitude_at(std::size_t x, std::size_t y) const;
    float mean_magnitude() const;
};

// Compute dense optical flow between two GRAY8 (or luma plane) frames.
// Uses a gradient-based iterative approximation (simplified Farneback).
FlowField optical_flow(const VideoFrame& prev, const VideoFrame& curr,
                       int window_size = 5, int iterations = 3);

// ── Motion vectors ────────────────────────────────────────────────────────────

struct MotionVector {
    int block_x = 0;   // top-left x of the block in the current frame
    int block_y = 0;   // top-left y of the block in the current frame
    int dx      = 0;   // displacement to matching block in previous frame
    int dy      = 0;
    long long sad = 0; // sum of absolute differences at the matched position
};

// Block-matching motion estimation.
std::vector<MotionVector> estimate_motion_vectors(const VideoFrame& prev,
                                                  const VideoFrame& curr,
                                                  int block_size = 16,
                                                  int search_radius = 8);

// ── Scene cut detection ───────────────────────────────────────────────────────

// Returns true when the transition from prev→curr looks like a hard cut.
// threshold: fraction of changed pixels (0.0–1.0); typical value 0.3.
bool detect_scene_cut(const VideoFrame& prev, const VideoFrame& curr,
                      float threshold = 0.30f);

// Stateful scene detector — maintains a histogram of recent inter-frame
// differences for adaptive thresholding.
class SceneDetector {
public:
    struct Config {
        float   base_threshold     = 0.30f;
        // Number of frames in the rolling history window.
        std::size_t history_frames = 30;
        // Sensitivity multiplier applied to the rolling mean difference.
        float   sensitivity        = 1.5f;
    };

    SceneDetector();
    explicit SceneDetector(Config cfg);

    // Returns true if this frame starts a new scene.
    bool process(const VideoFrame& frame);

    void reset();

private:
    Config cfg_;
    VideoFrame prev_;
    bool has_prev_ = false;
    std::vector<float> diff_history_;
};

// ── Macro-block activity ──────────────────────────────────────────────────────

struct BlockActivity {
    std::size_t x, y;       // top-left of the block
    float       variance;   // luma variance within the block
    float       motion_mag; // motion magnitude from the FlowField
};

// Combines optical flow and per-block variance into an activity map.
std::vector<BlockActivity> compute_block_activity(const VideoFrame& frame,
                                                  const FlowField& flow,
                                                  int block_size = 16);

} // namespace motion
} // namespace trekker

#endif // TREKKER_VIDEO_ASSETS_MOTION_ANALYSIS_H
