#ifndef TREKKER_VIDEO_ASSETS_VIDEO_CODEC_H
#define TREKKER_VIDEO_ASSETS_VIDEO_CODEC_H

#include <cstddef>
#include <cstdint>
#include <vector>
#include <stdexcept>

namespace trekker {
namespace video {

// ── Pixel formats ─────────────────────────────────────────────────────────────

enum class PixelFormat {
    RGB24,      // Packed: R G B, 8 bits each
    RGBA32,     // Packed: R G B A, 8 bits each
    BGR24,      // Packed: B G R, 8 bits each
    BGRA32,     // Packed: B G R A, 8 bits each
    YUV420P,    // Planar: Y full res, U/V half res (4:2:0)
    YUV422P,    // Planar: Y full res, U/V half horizontal res (4:2:2)
    YUV444P,    // Planar: Y U V all full res (4:4:4)
    NV12,       // Semi-planar: Y plane + interleaved UV (used by many HW codecs)
    GRAY8,      // Single luma plane
};

int bytes_per_pixel(PixelFormat fmt);
const char* pixel_format_name(PixelFormat fmt);

// ── Plane ─────────────────────────────────────────────────────────────────────
// A single channel plane (Y, U, V, or packed data).

struct Plane {
    std::vector<std::uint8_t> data;
    std::size_t width  = 0;
    std::size_t height = 0;
    std::size_t stride = 0; // bytes per row (may be > width for alignment)

    Plane() = default;
    Plane(std::size_t w, std::size_t h, std::size_t row_stride = 0);

    std::uint8_t  at(std::size_t x, std::size_t y) const;
    std::uint8_t& at(std::size_t x, std::size_t y);
};

// ── VideoFrame ────────────────────────────────────────────────────────────────
// Container for one decoded video frame.  Planes are populated depending on
// the pixel format:
//   RGB24/RGBA32/BGR24/BGRA32/GRAY8 → planes[0] only
//   YUV420P / YUV422P / YUV444P    → planes[0]=Y, planes[1]=U, planes[2]=V
//   NV12                            → planes[0]=Y, planes[1]=UV interleaved

struct VideoFrame {
    PixelFormat              format      = PixelFormat::RGB24;
    std::size_t              width       = 0;
    std::size_t              height      = 0;
    std::int64_t             pts         = 0; // presentation timestamp (μs)
    std::int64_t             duration_us = 0; // frame duration (μs)
    std::vector<Plane>       planes;

    // Construct a blank frame (zero-filled).
    static VideoFrame blank(PixelFormat fmt, std::size_t w, std::size_t h);

    // Number of planes for the given format.
    static int plane_count(PixelFormat fmt);
    int        plane_count() const { return plane_count(format); }
};

// ── YUV ↔ RGB conversion ──────────────────────────────────────────────────────

// BT.601 full-range coefficients (suitable for SDR content).
struct RgbPixel  { std::uint8_t r, g, b; };
struct YuvPixel  { std::uint8_t y, u, v; };

RgbPixel yuv_to_rgb(YuvPixel yuv);
YuvPixel rgb_to_yuv(RgbPixel rgb);

// Convert between packed/planar formats.
// Throws std::invalid_argument if dimensions are mismatched.
VideoFrame convert_format(const VideoFrame& src, PixelFormat dst_fmt);

// ── Chroma subsampling helpers ─────────────────────────────────────────────────

// Width/height of the U and V planes for the given format and luma dimensions.
std::size_t chroma_width (PixelFormat fmt, std::size_t luma_width);
std::size_t chroma_height(PixelFormat fmt, std::size_t luma_height);

} // namespace video
} // namespace trekker

#endif // TREKKER_VIDEO_ASSETS_VIDEO_CODEC_H
