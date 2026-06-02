#ifndef TREKKER_VIDEO_ASSETS_VIDEO_FILTERS_H
#define TREKKER_VIDEO_ASSETS_VIDEO_FILTERS_H

#include "../../video_codec/headers/video_codec.h"
#include <cstddef>
#include <cstdint>
#include <vector>
#include <functional>

namespace trekker {
namespace video {
namespace filters {

// ── Spatial filters ───────────────────────────────────────────────────────────
// All spatial filters operate in-place on a VideoFrame (RGB24 or GRAY8) or
// on the luma plane of a YUV frame.

// Gaussian blur — sigma controls spread, kernel_size must be odd.
VideoFrame gaussian_blur(const VideoFrame& src, float sigma, int kernel_size = 5);

// Sharpening unsharp mask: sharpened = src + amount*(src - blur(src)).
VideoFrame unsharp_mask(const VideoFrame& src, float sigma, float amount, int kernel_size = 5);

// Bilateral filter — edge-preserving smoothing.
// sigma_space: spatial Gaussian, sigma_color: intensity Gaussian.
VideoFrame bilateral_filter(const VideoFrame& src, float sigma_space, float sigma_color,
                            int kernel_size = 7);

// Box blur (fast, non-Gaussian).
VideoFrame box_blur(const VideoFrame& src, int radius);

// Median filter — good for impulse (salt-and-pepper) noise removal.
VideoFrame median_filter(const VideoFrame& src, int kernel_size = 3);

// Sobel edge detection — returns GRAY8 edge-magnitude frame.
VideoFrame sobel_edges(const VideoFrame& src);

// ── Color grading ─────────────────────────────────────────────────────────────

struct ColorGradeParams {
    // Brightness/contrast: out = clamp(contrast * in + brightness)
    float brightness = 0.0f;   // additive, range [-1, 1]
    float contrast   = 1.0f;   // multiplicative, range [0, 4]

    // Saturation: 0 = grayscale, 1 = no change, >1 more saturated.
    float saturation = 1.0f;

    // Gamma correction: out = pow(in, 1/gamma).
    float gamma = 1.0f;

    // Hue rotation in degrees [-180, 180].
    float hue_degrees = 0.0f;

    // Per-channel lift/gain (applied after all other ops).
    float r_gain = 1.0f, g_gain = 1.0f, b_gain = 1.0f;
};

VideoFrame color_grade(const VideoFrame& src, const ColorGradeParams& params);

// 1D LUT (look-up table) — maps each 0-255 input to a 0-255 output per channel.
struct Lut1D {
    std::vector<std::uint8_t> r_table; // 256 entries
    std::vector<std::uint8_t> g_table;
    std::vector<std::uint8_t> b_table;

    static Lut1D identity();
    static Lut1D from_gamma(float gamma);
    static Lut1D from_curves(const std::vector<std::pair<float,float>>& control_points);
};

VideoFrame apply_lut(const VideoFrame& src, const Lut1D& lut);

// ── Temporal filters ──────────────────────────────────────────────────────────
// Temporal filters need access to adjacent frames.

// Temporal noise reduction: blends current frame with the previous blended frame.
// alpha: weight of current frame [0,1]; higher = less smoothing.
class TemporalDenoise {
public:
    explicit TemporalDenoise(float alpha = 0.7f);

    // Feed frames in sequence; returns the denoised output.
    VideoFrame process(const VideoFrame& current);

    void reset();

private:
    float      alpha_;
    VideoFrame prev_;
    bool       has_prev_ = false;
};

// Motion-compensated temporal averaging — uses block-matching to align
// neighbouring frames before blending.  Higher quality than plain temporal blend.
class MotionCompensatedDenoise {
public:
    struct Config {
        int   block_size    = 16;   // macroblock side length (pixels)
        int   search_radius = 8;    // half-pel search range
        float blend_alpha   = 0.6f; // weight of current frame
    };
    MotionCompensatedDenoise();
    explicit MotionCompensatedDenoise(Config cfg);

    VideoFrame process(const VideoFrame& current);
    void reset();

private:
    Config     cfg_;
    VideoFrame prev_;
    bool       has_prev_ = false;

    // Returns motion-vector-shifted version of prev aligned to current.
    VideoFrame compensate(const VideoFrame& current, const VideoFrame& prev) const;
};

} // namespace filters
} // namespace video
} // namespace trekker

#endif // TREKKER_VIDEO_ASSETS_VIDEO_FILTERS_H
