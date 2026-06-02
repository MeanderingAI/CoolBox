#include "video_filters.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace trekker {
namespace video {
namespace filters {

namespace {

constexpr float kPi = 3.14159265358979323846f;

// Build a 1-D Gaussian kernel (length = kernel_size, must be odd).
std::vector<float> gaussian_kernel_1d(float sigma, int ksz) {
    if (ksz % 2 == 0) throw std::invalid_argument("kernel_size must be odd");
    std::vector<float> k(ksz);
    const float s2 = 2.f * sigma * sigma;
    float sum = 0.f;
    const int half = ksz / 2;
    for (int i = -half; i <= half; ++i) {
        k[i + half] = std::exp(-(i * i) / s2);
        sum += k[i + half];
    }
    for (auto& v : k) v /= sum;
    return k;
}

// Separate horizontal + vertical pass on a single Plane.
Plane convolve_gaussian(const Plane& src, float sigma, int ksz) {
    const auto k = gaussian_kernel_1d(sigma, ksz);
    const int half = ksz / 2;
    const int W = static_cast<int>(src.width);
    const int H = static_cast<int>(src.height);
    // Horizontal pass
    Plane tmp(src.width, src.height);
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x) {
            float acc = 0.f;
            for (int d = -half; d <= half; ++d)
                acc += k[d + half] * src.at(static_cast<std::size_t>(std::max(0, std::min(W-1, x+d))),
                                            static_cast<std::size_t>(y));
            tmp.at(x, y) = static_cast<std::uint8_t>(std::max(0.f, std::min(255.f, acc)));
        }
    // Vertical pass
    Plane out(src.width, src.height);
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x) {
            float acc = 0.f;
            for (int d = -half; d <= half; ++d)
                acc += k[d + half] * tmp.at(static_cast<std::size_t>(x),
                                            static_cast<std::size_t>(std::max(0, std::min(H-1, y+d))));
            out.at(x, y) = static_cast<std::uint8_t>(std::max(0.f, std::min(255.f, acc)));
        }
    return out;
}

VideoFrame apply_per_plane(const VideoFrame& src,
                           std::function<Plane(const Plane&)> fn) {
    VideoFrame out;
    out.format      = src.format;
    out.width       = src.width;
    out.height      = src.height;
    out.pts         = src.pts;
    out.duration_us = src.duration_us;
    out.planes.reserve(src.planes.size());
    for (const auto& p : src.planes)
        out.planes.push_back(fn(p));
    return out;
}

std::uint8_t clamp_u8(float v) {
    return static_cast<std::uint8_t>(std::min(255.f, std::max(0.f, v)));
}

} // namespace

// ── Gaussian blur ─────────────────────────────────────────────────────────────

VideoFrame gaussian_blur(const VideoFrame& src, float sigma, int kernel_size) {
    // For YUV formats only blur the luma plane to avoid colour shifts.
    if (src.format == PixelFormat::YUV420P || src.format == PixelFormat::YUV422P ||
        src.format == PixelFormat::YUV444P) {
        VideoFrame out = src;
        out.planes[0] = convolve_gaussian(src.planes[0], sigma, kernel_size);
        return out;
    }
    return apply_per_plane(src, [&](const Plane& p) {
        return convolve_gaussian(p, sigma, kernel_size);
    });
}

// ── Unsharp mask ──────────────────────────────────────────────────────────────

VideoFrame unsharp_mask(const VideoFrame& src, float sigma, float amount, int kernel_size) {
    auto blurred = gaussian_blur(src, sigma, kernel_size);
    VideoFrame out = src;
    for (std::size_t pi = 0; pi < src.planes.size(); ++pi)
        for (std::size_t i = 0; i < src.planes[pi].data.size(); ++i) {
            const float orig = src.planes[pi].data[i];
            const float blur = blurred.planes[pi].data[i];
            out.planes[pi].data[i] = clamp_u8(orig + amount * (orig - blur));
        }
    return out;
}

// ── Box blur ──────────────────────────────────────────────────────────────────

VideoFrame box_blur(const VideoFrame& src, int radius) {
    return apply_per_plane(src, [&](const Plane& src_p) {
        const int W = static_cast<int>(src_p.width);
        const int H = static_cast<int>(src_p.height);
        Plane out(src_p.width, src_p.height);
        for (int y = 0; y < H; ++y)
            for (int x = 0; x < W; ++x) {
                float acc = 0.f; int cnt = 0;
                for (int dy = -radius; dy <= radius; ++dy)
                    for (int dx = -radius; dx <= radius; ++dx) {
                        const int nx = std::max(0, std::min(W-1, x+dx));
                        const int ny = std::max(0, std::min(H-1, y+dy));
                        acc += src_p.at(nx, ny); ++cnt;
                    }
                out.at(x, y) = static_cast<std::uint8_t>(acc / cnt);
            }
        return out;
    });
}

// ── Median filter ─────────────────────────────────────────────────────────────

VideoFrame median_filter(const VideoFrame& src, int kernel_size) {
    return apply_per_plane(src, [&](const Plane& src_p) {
        const int W = static_cast<int>(src_p.width);
        const int H = static_cast<int>(src_p.height);
        const int half = kernel_size / 2;
        Plane out(src_p.width, src_p.height);
        std::vector<std::uint8_t> window;
        window.reserve(kernel_size * kernel_size);
        for (int y = 0; y < H; ++y)
            for (int x = 0; x < W; ++x) {
                window.clear();
                for (int dy = -half; dy <= half; ++dy)
                    for (int dx = -half; dx <= half; ++dx)
                        window.push_back(src_p.at(
                            std::max(0, std::min(W-1, x+dx)),
                            std::max(0, std::min(H-1, y+dy))));
                std::nth_element(window.begin(), window.begin() + window.size()/2, window.end());
                out.at(x, y) = window[window.size()/2];
            }
        return out;
    });
}

// ── Sobel edges ───────────────────────────────────────────────────────────────

VideoFrame sobel_edges(const VideoFrame& src) {
    // Convert to GRAY8 first if needed
    const VideoFrame& gray_src = (src.format == PixelFormat::GRAY8)
        ? src : convert_format(src, PixelFormat::GRAY8);
    const auto& p = gray_src.planes[0];
    const int W = static_cast<int>(p.width);
    const int H = static_cast<int>(p.height);

    VideoFrame out = VideoFrame::blank(PixelFormat::GRAY8, p.width, p.height);
    out.pts         = src.pts;
    out.duration_us = src.duration_us;
    auto& op = out.planes[0];

    for (int y = 1; y < H-1; ++y)
        for (int x = 1; x < W-1; ++x) {
            const float gx =
                -p.at(x-1,y-1) - 2*p.at(x-1,y) - p.at(x-1,y+1)
                +p.at(x+1,y-1) + 2*p.at(x+1,y) + p.at(x+1,y+1);
            const float gy =
                -p.at(x-1,y-1) - 2*p.at(x,y-1) - p.at(x+1,y-1)
                +p.at(x-1,y+1) + 2*p.at(x,y+1) + p.at(x+1,y+1);
            op.at(x, y) = clamp_u8(std::sqrt(gx*gx + gy*gy));
        }
    return out;
}

// ── Bilateral filter ──────────────────────────────────────────────────────────

VideoFrame bilateral_filter(const VideoFrame& src, float sigma_space, float sigma_color,
                            int kernel_size) {
    return apply_per_plane(src, [&](const Plane& sp) {
        const int W = static_cast<int>(sp.width);
        const int H = static_cast<int>(sp.height);
        const int half = kernel_size / 2;
        const float s2  = 2.f * sigma_space * sigma_space;
        const float c2  = 2.f * sigma_color * sigma_color;
        Plane out(sp.width, sp.height);
        for (int y = 0; y < H; ++y)
            for (int x = 0; x < W; ++x) {
                float acc = 0.f, wsum = 0.f;
                const float center = sp.at(x, y);
                for (int dy = -half; dy <= half; ++dy)
                    for (int dx = -half; dx <= half; ++dx) {
                        const int nx = std::max(0, std::min(W-1, x+dx));
                        const int ny = std::max(0, std::min(H-1, y+dy));
                        const float nb = sp.at(nx, ny);
                        const float diff = nb - center;
                        const float w = std::exp(-(dx*dx+dy*dy)/s2 - (diff*diff)/c2);
                        acc  += w * nb;
                        wsum += w;
                    }
                out.at(x, y) = clamp_u8(acc / wsum);
            }
        return out;
    });
}

// ── Color grading ─────────────────────────────────────────────────────────────

VideoFrame color_grade(const VideoFrame& src, const ColorGradeParams& p) {
    // Work in RGB24
    const bool was_yuv = (src.format != PixelFormat::RGB24);
    VideoFrame rgb = was_yuv ? convert_format(src, PixelFormat::RGB24) : src;
    auto& plane = rgb.planes[0];

    for (std::size_t i = 0; i < plane.data.size(); i += 3) {
        float r = plane.data[i    ] / 255.f;
        float g = plane.data[i + 1] / 255.f;
        float b = plane.data[i + 2] / 255.f;

        // Brightness + contrast
        r = p.contrast * r + p.brightness;
        g = p.contrast * g + p.brightness;
        b = p.contrast * b + p.brightness;

        // Saturation via luma-preserving desaturation blend
        const float luma = 0.299f*r + 0.587f*g + 0.114f*b;
        r = luma + p.saturation * (r - luma);
        g = luma + p.saturation * (g - luma);
        b = luma + p.saturation * (b - luma);

        // Gamma
        if (p.gamma > 0.f && p.gamma != 1.f) {
            r = std::pow(std::max(0.f, r), 1.f / p.gamma);
            g = std::pow(std::max(0.f, g), 1.f / p.gamma);
            b = std::pow(std::max(0.f, b), 1.f / p.gamma);
        }

        // Hue rotation (in HSV space)
        if (p.hue_degrees != 0.f) {
            const float deg = p.hue_degrees * (kPi / 180.f);
            const float cosH = std::cos(deg), sinH = std::sin(deg);
            const float nr = r*( cosH + (1.f-cosH)/3.f + sinH*std::sqrt(1.f/3.f))
                           + g*((1.f-cosH)/3.f - sinH*std::sqrt(1.f/3.f))
                           + b*((1.f-cosH)/3.f + sinH*std::sqrt(1.f/3.f));
            const float ng = r*((1.f-cosH)/3.f + sinH*std::sqrt(1.f/3.f))
                           + g*( cosH + (1.f-cosH)/3.f)
                           + b*((1.f-cosH)/3.f - sinH*std::sqrt(1.f/3.f));
            const float nb = r*((1.f-cosH)/3.f - sinH*std::sqrt(1.f/3.f))
                           + g*((1.f-cosH)/3.f + sinH*std::sqrt(1.f/3.f))
                           + b*( cosH + (1.f-cosH)/3.f);
            r = nr; g = ng; b = nb;
        }

        // Per-channel gain
        r *= p.r_gain; g *= p.g_gain; b *= p.b_gain;

        plane.data[i    ] = clamp_u8(r * 255.f);
        plane.data[i + 1] = clamp_u8(g * 255.f);
        plane.data[i + 2] = clamp_u8(b * 255.f);
    }

    return was_yuv ? convert_format(rgb, src.format) : rgb;
}

// ── LUT ───────────────────────────────────────────────────────────────────────

Lut1D Lut1D::identity() {
    Lut1D lut;
    lut.r_table.resize(256); lut.g_table.resize(256); lut.b_table.resize(256);
    for (int i = 0; i < 256; ++i)
        lut.r_table[i] = lut.g_table[i] = lut.b_table[i] = static_cast<std::uint8_t>(i);
    return lut;
}

Lut1D Lut1D::from_gamma(float gamma) {
    Lut1D lut;
    lut.r_table.resize(256); lut.g_table.resize(256); lut.b_table.resize(256);
    for (int i = 0; i < 256; ++i) {
        const std::uint8_t v = clamp_u8(std::pow(i / 255.f, 1.f/gamma) * 255.f);
        lut.r_table[i] = lut.g_table[i] = lut.b_table[i] = v;
    }
    return lut;
}

VideoFrame apply_lut(const VideoFrame& src, const Lut1D& lut) {
    if (src.format != PixelFormat::RGB24)
        return apply_lut(convert_format(src, PixelFormat::RGB24), lut);
    VideoFrame out = src;
    auto& p = out.planes[0];
    for (std::size_t i = 0; i < p.data.size(); i += 3) {
        p.data[i    ] = lut.r_table[p.data[i    ]];
        p.data[i + 1] = lut.g_table[p.data[i + 1]];
        p.data[i + 2] = lut.b_table[p.data[i + 2]];
    }
    return out;
}

// ── Temporal denoise ──────────────────────────────────────────────────────────

TemporalDenoise::TemporalDenoise(float alpha) : alpha_(alpha) {}

VideoFrame TemporalDenoise::process(const VideoFrame& current) {
    if (!has_prev_) {
        prev_     = current;
        has_prev_ = true;
        return current;
    }
    VideoFrame out = current;
    for (std::size_t pi = 0; pi < current.planes.size(); ++pi)
        for (std::size_t i = 0; i < current.planes[pi].data.size(); ++i) {
            const float c = current.planes[pi].data[i];
            const float p = prev_.planes[pi].data[i];
            out.planes[pi].data[i] = clamp_u8(alpha_ * c + (1.f - alpha_) * p);
        }
    prev_ = out;
    return out;
}

void TemporalDenoise::reset() { has_prev_ = false; }

// ── Motion-compensated denoise ────────────────────────────────────────────────

MotionCompensatedDenoise::MotionCompensatedDenoise() : MotionCompensatedDenoise(Config{}) {}

MotionCompensatedDenoise::MotionCompensatedDenoise(Config cfg) : cfg_(cfg) {}

VideoFrame MotionCompensatedDenoise::compensate(const VideoFrame& cur,
                                                const VideoFrame& prev) const {
    // Full-frame block-matching motion estimation.
    // Each block in `cur` finds its best match in `prev` within search_radius.
    const int W = static_cast<int>(cur.width);
    const int H = static_cast<int>(cur.height);
    const int bs = cfg_.block_size;
    const int sr = cfg_.search_radius;

    // Only operate on luma plane (or packed plane 0).
    const auto& cp = cur.planes[0];
    const auto& pp = prev.planes[0];

    VideoFrame shifted = prev;
    auto& sp = shifted.planes[0];

    for (int by = 0; by < H; by += bs)
        for (int bx = 0; bx < W; bx += bs) {
            // Block match: exhaustive SAD search
            int best_dx = 0, best_dy = 0;
            long long best_sad = std::numeric_limits<long long>::max();
            for (int dy = -sr; dy <= sr; ++dy)
                for (int dx = -sr; dx <= sr; ++dx) {
                    long long sad = 0;
                    for (int y = 0; y < bs && by+y < H; ++y)
                        for (int x = 0; x < bs && bx+x < W; ++x) {
                            const int nx = std::max(0, std::min(W-1, bx+x+dx));
                            const int ny = std::max(0, std::min(H-1, by+y+dy));
                            sad += std::abs(cp.at(bx+x, by+y) - pp.at(nx, ny));
                        }
                    if (sad < best_sad) { best_sad = sad; best_dx = dx; best_dy = dy; }
                }
            // Copy compensated block
            for (int y = 0; y < bs && by+y < H; ++y)
                for (int x = 0; x < bs && bx+x < W; ++x) {
                    const int nx = std::max(0, std::min(W-1, bx+x+best_dx));
                    const int ny = std::max(0, std::min(H-1, by+y+best_dy));
                    sp.at(bx+x, by+y) = pp.at(nx, ny);
                }
        }
    return shifted;
}

VideoFrame MotionCompensatedDenoise::process(const VideoFrame& current) {
    if (!has_prev_) {
        prev_ = current; has_prev_ = true; return current;
    }
    const auto aligned = compensate(current, prev_);
    VideoFrame out = current;
    for (std::size_t pi = 0; pi < current.planes.size(); ++pi)
        for (std::size_t i = 0; i < current.planes[pi].data.size(); ++i) {
            const float c = current.planes[pi].data[i];
            const float p = aligned.planes[pi].data[i];
            out.planes[pi].data[i] = clamp_u8(cfg_.blend_alpha * c + (1.f - cfg_.blend_alpha) * p);
        }
    prev_ = out;
    return out;
}

void MotionCompensatedDenoise::reset() { has_prev_ = false; }

} // namespace filters
} // namespace video
} // namespace trekker
