#ifndef TREKKER_VIDEO_ASSETS_VIDEO_CONTAINER_H
#define TREKKER_VIDEO_ASSETS_VIDEO_CONTAINER_H

// video_container — muxes raw VideoFrame/PCM data produced by movie_editor (or
// any other consumer) into standard, widely-playable "movie" file formats.
//
// This library intentionally avoids third-party video codecs (H.264, VP9,
// etc.) which are out of scope for a from-scratch implementation. Instead it
// hand-rolls two well-documented, self-contained formats:
//
//   * AVI  — RIFF container carrying uncompressed BGR24 video (one stream)
//            plus optional interleaved 16-bit PCM audio (a second stream).
//            Any standard media player / ffprobe can open the result.
//   * GIF  — GIF89a animated image, including a hand-rolled median-cut color
//            quantizer and the GIF-flavoured variable-width LZW compressor.
//
// Both are genuine encoders (not just raw byte dumps): AVI applies DIB
// row-padding/bottom-up conventions and RIFF index chunks, GIF performs
// real palette reduction + LZW entropy coding.

#include "../../video_codec/headers/video_codec.h"

#include <cstdint>
#include <string>
#include <vector>

namespace trekker {
namespace video {
namespace container {

// ── AVI writer ────────────────────────────────────────────────────────────────

struct AviAudioConfig {
    std::uint32_t sample_rate     = 0;   // 0 disables the audio stream entirely
    std::uint16_t num_channels    = 2;
    std::uint16_t bits_per_sample = 16;  // only 16-bit PCM is supported
};

class AviWriter {
public:
    // width/height/fps describe the (constant) video stream; every frame
    // passed to write_frame() must share these dimensions.
    AviWriter(const std::string& path, std::size_t width, std::size_t height,
              double fps, AviAudioConfig audio_config = {});
    ~AviWriter();

    AviWriter(const AviWriter&) = delete;
    AviWriter& operator=(const AviWriter&) = delete;

    // Converts `frame` to BGR24 (if needed) and appends it as the next video
    // frame. Throws std::invalid_argument on a dimension mismatch.
    void write_frame(const VideoFrame& frame);

    // Appends interleaved 16-bit PCM samples to the audio stream. Throws
    // std::logic_error if the writer was constructed without an audio stream.
    void write_audio(const std::vector<std::int16_t>& interleaved_pcm);

    // Backpatches RIFF/AVI header sizes, writes the idx1 index, and closes
    // the file. Safe to call multiple times; also invoked by the destructor.
    void finish();

    std::size_t frame_count() const;
    std::size_t audio_sample_frames() const; // per-channel sample count written

private:
    struct Impl;
    Impl* impl_;
};

// ── GIF encoder ───────────────────────────────────────────────────────────────

struct GifOptions {
    int loop_count = 0;    // 0 = loop forever (NETSCAPE2.0 convention)
    int max_colors = 256;  // palette size per frame, clamped to [2, 256]
};

class GifEncoder {
public:
    GifEncoder(const std::string& path, std::size_t width, std::size_t height,
              GifOptions options = {});
    ~GifEncoder();

    GifEncoder(const GifEncoder&) = delete;
    GifEncoder& operator=(const GifEncoder&) = delete;

    // Quantizes `frame` (converted to RGB24 if needed) and appends it as the
    // next animation frame. frame_delay_us is stored in GIF's native 1/100s
    // granularity (rounded).
    void write_frame(const VideoFrame& frame, std::int64_t frame_delay_us);

    void finish();
    std::size_t frame_count() const;

private:
    struct Impl;
    Impl* impl_;
};

// ── Building blocks (exposed for reuse/testing) ───────────────────────────────

struct QuantizedImage {
    std::vector<std::uint8_t> palette; // RGB triplets; palette.size()/3 entries
    std::vector<std::uint8_t> indices; // one entry per pixel, row-major
};

// Median-cut color quantizer: reduces an RGB24 frame to at most max_colors
// palette entries (clamped to [2,256]) and returns the per-pixel indices.
QuantizedImage median_cut_quantize(const VideoFrame& rgb_frame, int max_colors);

// GIF-flavoured LZW: variable code width (min_code_size+1 .. 12 bits),
// LSB-first bit packing, Clear/End control codes. Exposed standalone so
// tests can validate round-tripping without parsing a whole GIF file.
std::vector<std::uint8_t> gif_lzw_compress(const std::vector<std::uint8_t>& indices,
                                           int min_code_size);
std::vector<std::uint8_t> gif_lzw_decompress(const std::vector<std::uint8_t>& compressed,
                                             int min_code_size,
                                             std::size_t expected_count);

} // namespace container
} // namespace video
} // namespace trekker

#endif // TREKKER_VIDEO_ASSETS_VIDEO_CONTAINER_H
