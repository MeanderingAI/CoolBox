#include "video_codec.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace trekker {
namespace video {

// ── Pixel format helpers ──────────────────────────────────────────────────────

int bytes_per_pixel(PixelFormat fmt) {
    switch (fmt) {
        case PixelFormat::RGB24:   return 3;
        case PixelFormat::BGR24:   return 3;
        case PixelFormat::RGBA32:  return 4;
        case PixelFormat::BGRA32:  return 4;
        case PixelFormat::GRAY8:   return 1;
        case PixelFormat::NV12:    return 0; // semi-planar — no single bpp
        case PixelFormat::YUV420P: return 0;
        case PixelFormat::YUV422P: return 0;
        case PixelFormat::YUV444P: return 0;
    }
    return 0;
}

const char* pixel_format_name(PixelFormat fmt) {
    switch (fmt) {
        case PixelFormat::RGB24:   return "RGB24";
        case PixelFormat::BGR24:   return "BGR24";
        case PixelFormat::RGBA32:  return "RGBA32";
        case PixelFormat::BGRA32:  return "BGRA32";
        case PixelFormat::GRAY8:   return "GRAY8";
        case PixelFormat::NV12:    return "NV12";
        case PixelFormat::YUV420P: return "YUV420P";
        case PixelFormat::YUV422P: return "YUV422P";
        case PixelFormat::YUV444P: return "YUV444P";
    }
    return "UNKNOWN";
}

// ── Chroma subsampling ────────────────────────────────────────────────────────

std::size_t chroma_width(PixelFormat fmt, std::size_t luma_width) {
    switch (fmt) {
        case PixelFormat::YUV420P:
        case PixelFormat::YUV422P:
        case PixelFormat::NV12:
            return (luma_width + 1) / 2;
        default:
            return luma_width;
    }
}

std::size_t chroma_height(PixelFormat fmt, std::size_t luma_height) {
    switch (fmt) {
        case PixelFormat::YUV420P:
        case PixelFormat::NV12:
            return (luma_height + 1) / 2;
        default:
            return luma_height;
    }
}

// ── Plane ─────────────────────────────────────────────────────────────────────

Plane::Plane(std::size_t w, std::size_t h, std::size_t row_stride)
    : width(w), height(h), stride(row_stride ? row_stride : w)
{
    data.assign(stride * height, 0);
}

std::uint8_t Plane::at(std::size_t x, std::size_t y) const {
    return data[y * stride + x];
}

std::uint8_t& Plane::at(std::size_t x, std::size_t y) {
    return data[y * stride + x];
}

// ── VideoFrame ────────────────────────────────────────────────────────────────

int VideoFrame::plane_count(PixelFormat fmt) {
    switch (fmt) {
        case PixelFormat::YUV420P:
        case PixelFormat::YUV422P:
        case PixelFormat::YUV444P:
            return 3;
        case PixelFormat::NV12:
            return 2;
        default:
            return 1;
    }
}

VideoFrame VideoFrame::blank(PixelFormat fmt, std::size_t w, std::size_t h) {
    VideoFrame f;
    f.format = fmt;
    f.width  = w;
    f.height = h;
    const int n = plane_count(fmt);
    f.planes.reserve(n);
    if (n == 1) {
        const std::size_t bpp = static_cast<std::size_t>(bytes_per_pixel(fmt));
        f.planes.emplace_back(w * bpp, h);
    } else {
        // Luma
        f.planes.emplace_back(w, h);
        // Chroma plane(s)
        const std::size_t cw = chroma_width(fmt, w);
        const std::size_t ch = chroma_height(fmt, h);
        if (fmt == PixelFormat::NV12) {
            f.planes.emplace_back(cw * 2, ch); // interleaved UV
        } else {
            f.planes.emplace_back(cw, ch);
            f.planes.emplace_back(cw, ch);
        }
    }
    return f;
}

// ── YUV ↔ RGB (BT.601 full-range) ────────────────────────────────────────────

static std::uint8_t clamp_u8(int v) {
    return static_cast<std::uint8_t>(std::min(255, std::max(0, v)));
}

RgbPixel yuv_to_rgb(YuvPixel yuv) {
    const int y = yuv.y;
    const int u = yuv.u - 128;
    const int v = yuv.v - 128;
    return {
        clamp_u8(y + static_cast<int>(1.402   * v)),
        clamp_u8(y - static_cast<int>(0.344136 * u) - static_cast<int>(0.714136 * v)),
        clamp_u8(y + static_cast<int>(1.772   * u)),
    };
}

YuvPixel rgb_to_yuv(RgbPixel rgb) {
    const int r = rgb.r, g = rgb.g, b = rgb.b;
    return {
        clamp_u8( static_cast<int>( 0.299   * r + 0.587   * g + 0.114   * b)),
        clamp_u8( static_cast<int>(-0.168736* r - 0.331264* g + 0.5     * b) + 128),
        clamp_u8( static_cast<int>( 0.5     * r - 0.418688* g - 0.081312* b) + 128),
    };
}

// ── Format conversion ─────────────────────────────────────────────────────────

VideoFrame convert_format(const VideoFrame& src, PixelFormat dst_fmt) {
    if (src.format == dst_fmt) return src;

    // Strategy: convert src → RGB24 as intermediate, then → dst_fmt
    // For a production library this would use dedicated fast paths per pair.
    VideoFrame rgb = VideoFrame::blank(PixelFormat::RGB24, src.width, src.height);
    rgb.pts         = src.pts;
    rgb.duration_us = src.duration_us;

    auto& rp = rgb.planes[0];

    if (src.format == PixelFormat::YUV420P || src.format == PixelFormat::YUV444P ||
        src.format == PixelFormat::YUV422P) {
        for (std::size_t y = 0; y < src.height; ++y) {
            for (std::size_t x = 0; x < src.width; ++x) {
                const std::size_t cx = chroma_width(src.format, x + 1) - 1;
                const std::size_t cy = chroma_height(src.format, y + 1) - 1;
                YuvPixel yuv{
                    src.planes[0].at(x, y),
                    src.planes[1].at(cx, cy),
                    src.planes[2].at(cx, cy),
                };
                auto px = yuv_to_rgb(yuv);
                rp.data[(y * rp.stride) + x * 3 + 0] = px.r;
                rp.data[(y * rp.stride) + x * 3 + 1] = px.g;
                rp.data[(y * rp.stride) + x * 3 + 2] = px.b;
            }
        }
    } else if (src.format == PixelFormat::GRAY8) {
        for (std::size_t y = 0; y < src.height; ++y)
            for (std::size_t x = 0; x < src.width; ++x) {
                const std::uint8_t v = src.planes[0].at(x, y);
                rp.data[(y * rp.stride) + x * 3 + 0] = v;
                rp.data[(y * rp.stride) + x * 3 + 1] = v;
                rp.data[(y * rp.stride) + x * 3 + 2] = v;
            }
    } else if (src.format == PixelFormat::RGB24) {
        rgb = src;
    }
    // Other source formats left as future extension points.

    if (dst_fmt == PixelFormat::RGB24) return rgb;

    VideoFrame dst = VideoFrame::blank(dst_fmt, src.width, src.height);
    dst.pts         = src.pts;
    dst.duration_us = src.duration_us;

    if (dst_fmt == PixelFormat::GRAY8) {
        for (std::size_t y = 0; y < dst.height; ++y)
            for (std::size_t x = 0; x < dst.width; ++x) {
                const std::size_t i = y * rp.stride + x * 3;
                dst.planes[0].at(x, y) = clamp_u8(
                    static_cast<int>(0.299 * rp.data[i] + 0.587 * rp.data[i+1] + 0.114 * rp.data[i+2]));
            }
    } else if (dst_fmt == PixelFormat::YUV420P || dst_fmt == PixelFormat::YUV422P ||
               dst_fmt == PixelFormat::YUV444P) {
        for (std::size_t y = 0; y < dst.height; ++y)
            for (std::size_t x = 0; x < dst.width; ++x) {
                const std::size_t i = y * rp.stride + x * 3;
                auto yuv = rgb_to_yuv({rp.data[i], rp.data[i+1], rp.data[i+2]});
                const std::size_t cx = chroma_width(dst_fmt, x + 1) - 1;
                const std::size_t cy = chroma_height(dst_fmt, y + 1) - 1;
                dst.planes[0].at(x, y)   = yuv.y;
                dst.planes[1].at(cx, cy) = yuv.u;
                dst.planes[2].at(cx, cy) = yuv.v;
            }
    }

    return dst;
}

} // namespace video
} // namespace trekker
