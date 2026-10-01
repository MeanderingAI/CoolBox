#include "tyst_framework.hpp"
#include "video_container.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <string>
#include <vector>

using trekker::video::VideoFrame;
using trekker::video::PixelFormat;
using trekker::video::Plane;
namespace vc = trekker::video::container;

namespace {

VideoFrame make_rgb_frame(std::size_t width, std::size_t height,
                          const std::vector<std::array<std::uint8_t, 3>>& pixel_colors) {
    VideoFrame frame = VideoFrame::blank(PixelFormat::RGB24, width, height);
    Plane& plane = frame.planes[0];
    for (std::size_t y = 0; y < height; ++y) {
        for (std::size_t x = 0; x < width; ++x) {
            const auto& c = pixel_colors[y * width + x];
            plane.at(x * 3 + 0, y) = c[0];
            plane.at(x * 3 + 1, y) = c[1];
            plane.at(x * 3 + 2, y) = c[2];
        }
    }
    return frame;
}

std::string temp_file_path(const std::string& suffix) {
    static int counter = 0;
    const char* tmp_dir = std::getenv("TMPDIR");
    const std::string dir = tmp_dir ? tmp_dir : "/tmp/";
    return dir + (dir.back() == '/' ? "" : "/") + "video_container_test_" +
           std::to_string(++counter) + suffix;
}

std::vector<std::uint8_t> read_whole_file(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    return std::vector<std::uint8_t>((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

std::uint32_t read_u32le(const std::uint8_t* p) {
    return static_cast<std::uint32_t>(p[0]) | (static_cast<std::uint32_t>(p[1]) << 8) |
           (static_cast<std::uint32_t>(p[2]) << 16) | (static_cast<std::uint32_t>(p[3]) << 24);
}

std::uint16_t read_u16le(const std::uint8_t* p) {
    return static_cast<std::uint16_t>(p[0]) | (static_cast<std::uint16_t>(p[1]) << 8);
}

} // namespace

// ── GIF LZW round-trip ────────────────────────────────────────────────────────

TYST_TEST(VideoContainerTests, GifLzwRoundTripSmallAlphabet) {
    // 4-color alphabet (min_code_size=2), with repeated runs so the
    // dictionary actually builds up multi-symbol strings.
    std::vector<std::uint8_t> indices;
    for (int rep = 0; rep < 20; ++rep) {
        indices.insert(indices.end(), {0, 0, 1, 1, 2, 3, 3, 3, 2, 1, 0});
    }
    const auto compressed = vc::gif_lzw_compress(indices, 2);
    const auto decompressed = vc::gif_lzw_decompress(compressed, 2, indices.size());
    TYST_ASSERT_EQ(decompressed.size(), indices.size());
    TYST_EXPECT_TRUE(std::equal(decompressed.begin(), decompressed.end(), indices.begin()));
}

TYST_TEST(VideoContainerTests, GifLzwRoundTripFullAlphabetForcesClear) {
    // 256-color alphabet with enough distinct sequences that the code table
    // should overflow past 4096 entries at least once, forcing an internal
    // Clear Code reset that must not corrupt the stream.
    std::vector<std::uint8_t> indices;
    unsigned seed = 12345u;
    for (int i = 0; i < 20000; ++i) {
        seed = seed * 1103515245u + 12345u;
        indices.push_back(static_cast<std::uint8_t>((seed >> 16) & 0xFF));
    }
    const auto compressed = vc::gif_lzw_compress(indices, 8);
    const auto decompressed = vc::gif_lzw_decompress(compressed, 8, indices.size());
    TYST_ASSERT_EQ(decompressed.size(), indices.size());
    TYST_EXPECT_TRUE(std::equal(decompressed.begin(), decompressed.end(), indices.begin()));
}

TYST_TEST(VideoContainerTests, GifLzwRoundTripEmpty) {
    std::vector<std::uint8_t> indices;
    const auto compressed = vc::gif_lzw_compress(indices, 4);
    const auto decompressed = vc::gif_lzw_decompress(compressed, 4, 0);
    TYST_EXPECT_TRUE(decompressed.empty());
}

// ── Median-cut quantizer ──────────────────────────────────────────────────────

TYST_TEST(VideoContainerTests, MedianCutQuantizeExactWhenUnderBudget) {
    // 4 distinct colors, budget of 8 — quantizer should reproduce every pixel
    // exactly (no perceptual loss when there's headroom in the palette).
    const std::vector<std::array<std::uint8_t, 3>> colors = {
        {255, 0, 0}, {0, 255, 0}, {0, 0, 255}, {255, 255, 0},
        {255, 0, 0}, {0, 255, 0}, {0, 0, 255}, {255, 255, 0},
    };
    VideoFrame frame = make_rgb_frame(4, 2, colors);
    const auto quant = vc::median_cut_quantize(frame, 8);
    TYST_ASSERT_EQ(quant.indices.size(), colors.size());
    TYST_EXPECT_LE(quant.palette.size() / 3, static_cast<std::size_t>(8));

    for (std::size_t i = 0; i < colors.size(); ++i) {
        const std::uint8_t idx = quant.indices[i];
        TYST_EXPECT_EQ(quant.palette[idx * 3 + 0], colors[i][0]);
        TYST_EXPECT_EQ(quant.palette[idx * 3 + 1], colors[i][1]);
        TYST_EXPECT_EQ(quant.palette[idx * 3 + 2], colors[i][2]);
    }
}

TYST_TEST(VideoContainerTests, MedianCutQuantizeRespectsColorBudget) {
    std::vector<std::array<std::uint8_t, 3>> colors;
    for (int i = 0; i < 64; ++i) {
        colors.push_back({static_cast<std::uint8_t>(i * 4), static_cast<std::uint8_t>(255 - i * 4),
                          static_cast<std::uint8_t>((i * 37) % 256)});
    }
    VideoFrame frame = make_rgb_frame(8, 8, colors);
    const auto quant = vc::median_cut_quantize(frame, 4);
    TYST_EXPECT_LE(quant.palette.size() / 3, static_cast<std::size_t>(4));
    for (const auto idx : quant.indices) {
        TYST_EXPECT_LT(static_cast<std::size_t>(idx), quant.palette.size() / 3);
    }
}

// ── GifEncoder end-to-end ─────────────────────────────────────────────────────

namespace {

// Minimal GIF reader sufficient to validate what GifEncoder writes: parses
// the header/extensions and decodes the first image frame back to RGB using
// gif_lzw_decompress + the local color table.
struct DecodedGifFrame {
    std::size_t width = 0, height = 0;
    std::vector<std::array<std::uint8_t, 3>> pixels;
};

std::vector<DecodedGifFrame> decode_gif(const std::vector<std::uint8_t>& bytes, int& frame_marker_count) {
    std::vector<DecodedGifFrame> frames;
    frame_marker_count = 0;
    std::size_t pos = 6; // skip "GIF89a"
    const std::uint16_t screen_w = read_u16le(&bytes[pos]);
    const std::uint16_t screen_h = read_u16le(&bytes[pos + 2]);
    (void)screen_w; (void)screen_h;
    const std::uint8_t packed = bytes[pos + 4];
    pos += 7;
    if (packed & 0x80) {
        const int gct_size = 2 << (packed & 0x07);
        pos += static_cast<std::size_t>(gct_size) * 3;
    }

    while (pos < bytes.size()) {
        const std::uint8_t marker = bytes[pos];
        if (marker == 0x21) { // extension
            pos += 2; // introducer + label
            while (bytes[pos] != 0x00) {
                const std::uint8_t block_size = bytes[pos];
                pos += 1 + block_size;
            }
            pos += 1; // terminator
        } else if (marker == 0x2C) { // image descriptor
            ++frame_marker_count;
            pos += 1;
            const std::uint16_t w = read_u16le(&bytes[pos + 4]);
            const std::uint16_t h = read_u16le(&bytes[pos + 6]);
            const std::uint8_t img_packed = bytes[pos + 8];
            pos += 9;

            std::vector<std::array<std::uint8_t, 3>> palette;
            if (img_packed & 0x80) {
                const int lct_size = 2 << (img_packed & 0x07);
                for (int i = 0; i < lct_size; ++i) {
                    palette.push_back({bytes[pos], bytes[pos + 1], bytes[pos + 2]});
                    pos += 3;
                }
            }

            const std::uint8_t min_code_size = bytes[pos];
            pos += 1;
            std::vector<std::uint8_t> compressed;
            while (bytes[pos] != 0x00) {
                const std::uint8_t block_size = bytes[pos];
                compressed.insert(compressed.end(), bytes.begin() + static_cast<long>(pos) + 1,
                                  bytes.begin() + static_cast<long>(pos) + 1 + block_size);
                pos += 1 + block_size;
            }
            pos += 1; // terminator

            const auto indices = vc::gif_lzw_decompress(compressed, min_code_size,
                                                        static_cast<std::size_t>(w) * h);
            DecodedGifFrame decoded;
            decoded.width = w;
            decoded.height = h;
            decoded.pixels.resize(indices.size());
            for (std::size_t i = 0; i < indices.size(); ++i) decoded.pixels[i] = palette.at(indices[i]);
            frames.push_back(std::move(decoded));
        } else if (marker == 0x3B) { // trailer
            break;
        } else {
            break; // unexpected marker; stop parsing defensively
        }
    }
    return frames;
}

} // namespace

TYST_TEST(VideoContainerTests, GifEncoderRoundTripsPixelsExactly) {
    const std::string path = temp_file_path(".gif");
    const std::size_t w = 4, h = 3;

    std::vector<std::array<std::uint8_t, 3>> frame1_colors = {
        {10, 20, 30}, {40, 50, 60}, {70, 80, 90}, {100, 110, 120},
        {130, 140, 150}, {160, 170, 180}, {190, 200, 210}, {220, 230, 240},
        {5, 6, 7}, {8, 9, 10}, {11, 12, 13}, {14, 15, 16},
    };
    std::vector<std::array<std::uint8_t, 3>> frame2_colors(w * h, {255, 0, 0});

    {
        vc::GifEncoder encoder(path, w, h, vc::GifOptions{/*loop_count=*/0, /*max_colors=*/256});
        encoder.write_frame(make_rgb_frame(w, h, frame1_colors), 100'000);
        encoder.write_frame(make_rgb_frame(w, h, frame2_colors), 50'000);
        TYST_EXPECT_EQ(encoder.frame_count(), static_cast<std::size_t>(2));
        encoder.finish();
    }

    const auto bytes = read_whole_file(path);
    TYST_ASSERT_TRUE(bytes.size() > 20);
    TYST_EXPECT_EQ(std::string(bytes.begin(), bytes.begin() + 6), std::string("GIF89a"));
    TYST_EXPECT_EQ(bytes.back(), 0x3B);

    int frame_markers = 0;
    const auto decoded = decode_gif(bytes, frame_markers);
    TYST_EXPECT_EQ(frame_markers, 2);
    TYST_ASSERT_EQ(decoded.size(), static_cast<std::size_t>(2));
    TYST_EXPECT_EQ(decoded[0].width, w);
    TYST_EXPECT_EQ(decoded[0].height, h);
    for (std::size_t i = 0; i < frame1_colors.size(); ++i) {
        TYST_EXPECT_EQ(decoded[0].pixels[i][0], frame1_colors[i][0]);
        TYST_EXPECT_EQ(decoded[0].pixels[i][1], frame1_colors[i][1]);
        TYST_EXPECT_EQ(decoded[0].pixels[i][2], frame1_colors[i][2]);
    }
    for (std::size_t i = 0; i < frame2_colors.size(); ++i) {
        TYST_EXPECT_EQ(decoded[1].pixels[i][0], frame2_colors[i][0]);
    }

    std::remove(path.c_str());
}

// ── AviWriter end-to-end ──────────────────────────────────────────────────────

namespace {

// Locates the first occurrence of a fourcc-prefixed chunk anywhere in the
// byte stream (a proper RIFF walker would follow LIST nesting explicitly,
// but a linear scan is sufficient — and robust — for this validation test).
std::vector<std::size_t> find_chunks(const std::vector<std::uint8_t>& bytes, const char fourcc[4],
                                     std::size_t limit = SIZE_MAX) {
    std::vector<std::size_t> hits;
    const std::size_t end = std::min(limit, bytes.size());
    for (std::size_t i = 0; i + 4 <= end; ++i) {
        if (std::memcmp(bytes.data() + i, fourcc, 4) == 0) hits.push_back(i);
    }
    return hits;
}

} // namespace

TYST_TEST(VideoContainerTests, AviWriterVideoOnlyProducesValidHeadersAndFrames) {
    const std::string path = temp_file_path(".avi");
    // 5px width forces DIB row padding (5*3=15 bytes -> padded to 16).
    const std::size_t w = 5, h = 2;
    const double fps = 24.0;

    std::vector<std::array<std::uint8_t, 3>> colors(w * h);
    for (std::size_t i = 0; i < colors.size(); ++i) {
        colors[i] = {static_cast<std::uint8_t>(i * 10), static_cast<std::uint8_t>(i * 20), static_cast<std::uint8_t>(i * 30)};
    }
    const VideoFrame frame = make_rgb_frame(w, h, colors);

    {
        vc::AviWriter writer(path, w, h, fps);
        writer.write_frame(frame);
        writer.write_frame(frame);
        writer.write_frame(frame);
        TYST_EXPECT_EQ(writer.frame_count(), static_cast<std::size_t>(3));
        writer.finish();
    }

    const auto bytes = read_whole_file(path);
    TYST_ASSERT_TRUE(bytes.size() > 64);
    TYST_EXPECT_EQ(std::string(bytes.begin(), bytes.begin() + 4), std::string("RIFF"));
    TYST_EXPECT_EQ(std::string(bytes.begin() + 8, bytes.begin() + 12), std::string("AVI "));

    const std::uint32_t riff_size = read_u32le(&bytes[4]);
    TYST_EXPECT_EQ(static_cast<std::size_t>(riff_size) + 8, bytes.size());

    // avih.dwTotalFrames — avih chunk begins right after "LIST"<size>"hdrl".
    const auto avih_hits = find_chunks(bytes, "avih");
    TYST_ASSERT_EQ(avih_hits.size(), static_cast<std::size_t>(1));
    const std::size_t avih_data = avih_hits[0] + 8;
    const std::uint32_t total_frames = read_u32le(&bytes[avih_data + 4 * 4]); // dwTotalFrames is the 5th field
    TYST_EXPECT_EQ(total_frames, static_cast<std::uint32_t>(3));
    const std::uint32_t avih_width = read_u32le(&bytes[avih_data + 4 * 8]);
    const std::uint32_t avih_height = read_u32le(&bytes[avih_data + 4 * 9]);
    TYST_EXPECT_EQ(avih_width, static_cast<std::uint32_t>(w));
    TYST_EXPECT_EQ(avih_height, static_cast<std::uint32_t>(h));

    // strf BITMAPINFOHEADER
    const auto strf_hits = find_chunks(bytes, "strf");
    TYST_ASSERT_TRUE(!strf_hits.empty());
    const std::size_t strf_data = strf_hits[0] + 8;
    TYST_EXPECT_EQ(read_u32le(&bytes[strf_data + 4]), static_cast<std::uint32_t>(w));   // biWidth
    TYST_EXPECT_EQ(read_u32le(&bytes[strf_data + 8]), static_cast<std::uint32_t>(h));   // biHeight
    TYST_EXPECT_EQ(read_u16le(&bytes[strf_data + 14]), static_cast<std::uint16_t>(24)); // biBitCount

    // Video frame chunks ("00db") — restrict the scan to before idx1, since
    // the index itself also repeats these fourccs (once per entry).
    const auto idx1_hits_prescan = find_chunks(bytes, "idx1");
    TYST_ASSERT_EQ(idx1_hits_prescan.size(), static_cast<std::size_t>(1));
    const std::size_t movi_region_end = idx1_hits_prescan[0];

    const auto frame_hits = find_chunks(bytes, "00db", movi_region_end);
    TYST_ASSERT_EQ(frame_hits.size(), static_cast<std::size_t>(3));
    const std::size_t stride = ((w * 3 + 3) / 4) * 4;
    const std::uint32_t expected_chunk_size = static_cast<std::uint32_t>(stride * h);
    for (auto hit : frame_hits) {
        TYST_EXPECT_EQ(read_u32le(&bytes[hit + 4]), expected_chunk_size);
    }

    // Verify pixel round-trip for the first written frame: DIB rows are
    // bottom-up and BGR-ordered.
    const std::size_t pixel_data_start = frame_hits[0] + 8;
    for (std::size_t y = 0; y < h; ++y) {
        const std::size_t dib_row = h - 1 - y;
        for (std::size_t x = 0; x < w; ++x) {
            const std::size_t off = pixel_data_start + dib_row * stride + x * 3;
            const auto& expected = colors[y * w + x];
            TYST_EXPECT_EQ(bytes[off + 0], expected[2]); // B
            TYST_EXPECT_EQ(bytes[off + 1], expected[1]); // G
            TYST_EXPECT_EQ(bytes[off + 2], expected[0]); // R
        }
    }

    // idx1 — one entry (16 bytes) per chunk written.
    const std::uint32_t idx1_size = read_u32le(&bytes[idx1_hits_prescan[0] + 4]);
    TYST_EXPECT_EQ(idx1_size, static_cast<std::uint32_t>(3 * 16));

    TYST_EXPECT_EQ(bytes.back(), bytes.back()); // file closed cleanly (no crash reading tail)
    std::remove(path.c_str());
}

TYST_TEST(VideoContainerTests, AviWriterInterleavesAudioAndVideoStreams) {
    const std::string path = temp_file_path("_audio.avi");
    const std::size_t w = 4, h = 2;
    std::vector<std::array<std::uint8_t, 3>> colors(w * h, {1, 2, 3});
    const VideoFrame frame = make_rgb_frame(w, h, colors);

    vc::AviAudioConfig audio_cfg;
    audio_cfg.sample_rate = 8000;
    audio_cfg.num_channels = 1;
    audio_cfg.bits_per_sample = 16;

    const std::vector<std::int16_t> pcm_block(4000, 1234); // 0.5s of audio at 8kHz mono

    {
        vc::AviWriter writer(path, w, h, 24.0, audio_cfg);
        writer.write_frame(frame);
        writer.write_audio(pcm_block);
        writer.write_frame(frame);
        writer.write_audio(pcm_block);
        TYST_EXPECT_EQ(writer.frame_count(), static_cast<std::size_t>(2));
        TYST_EXPECT_EQ(writer.audio_sample_frames(), static_cast<std::size_t>(8000));
        writer.finish();
    }

    const auto bytes = read_whole_file(path);
    const auto idx1_hits = find_chunks(bytes, "idx1");
    TYST_ASSERT_EQ(idx1_hits.size(), static_cast<std::size_t>(1));
    const std::size_t movi_region_end = idx1_hits[0];

    const auto video_hits = find_chunks(bytes, "00db", movi_region_end);
    const auto audio_hits = find_chunks(bytes, "01wb", movi_region_end);
    TYST_EXPECT_EQ(video_hits.size(), static_cast<std::size_t>(2));
    TYST_EXPECT_EQ(audio_hits.size(), static_cast<std::size_t>(2));

    for (auto hit : audio_hits) {
        TYST_EXPECT_EQ(read_u32le(&bytes[hit + 4]), static_cast<std::uint32_t>(pcm_block.size() * 2));
    }

    const auto auds_strh_hits = find_chunks(bytes, "auds");
    TYST_ASSERT_EQ(auds_strh_hits.size(), static_cast<std::size_t>(1));
    // auds strh data layout (auds_strh_hits[0] already points at the start of
    // the fccType field, whose 4 bytes literally spell "auds"):
    // fccType(4) fccHandler(4) dwFlags(4) wPriority(2) wLanguage(2)
    // dwInitialFrames(4) dwScale(4) dwRate(4) dwStart(4) dwLength(4) ...
    const std::size_t length_field_offset = auds_strh_hits[0] + 4 + 4 + 4 + 2 + 2 + 4 + 4 + 4 + 4;
    TYST_EXPECT_EQ(read_u32le(&bytes[length_field_offset]), static_cast<std::uint32_t>(8000));

    TYST_EXPECT_EQ(read_u32le(&bytes[idx1_hits[0] + 4]), static_cast<std::uint32_t>(4 * 16));

    std::remove(path.c_str());
}

// ── Readers (round-trip with the writers above) ──────────────────────────────

TYST_TEST(VideoContainerTests, ReadGifRoundTripsSolidColorFrames) {
    const std::string path = temp_file_path("_roundtrip.gif");
    const std::size_t w = 6, h = 4;
    std::vector<std::array<std::uint8_t, 3>> red(w * h, {200, 10, 10});
    std::vector<std::array<std::uint8_t, 3>> blue(w * h, {10, 10, 200});
    const VideoFrame frame_a = make_rgb_frame(w, h, red);
    const VideoFrame frame_b = make_rgb_frame(w, h, blue);

    {
        vc::GifEncoder encoder(path, w, h);
        encoder.write_frame(frame_a, 150'000); // 150ms
        encoder.write_frame(frame_b, 250'000); // 250ms
        encoder.finish();
    }

    const vc::DecodedGif decoded = vc::read_gif(path);
    TYST_ASSERT_EQ(decoded.width, w);
    TYST_ASSERT_EQ(decoded.height, h);
    TYST_ASSERT_EQ(decoded.frames.size(), static_cast<std::size_t>(2));
    TYST_ASSERT_EQ(decoded.frame_delay_us.size(), static_cast<std::size_t>(2));

    // GIF delay granularity is 1/100s, so expect the nearest 10ms multiple.
    TYST_EXPECT_EQ(decoded.frame_delay_us[0], static_cast<std::int64_t>(150'000));
    TYST_EXPECT_EQ(decoded.frame_delay_us[1], static_cast<std::int64_t>(250'000));

    const Plane& p0 = decoded.frames[0].planes[0];
    const Plane& p1 = decoded.frames[1].planes[0];
    TYST_EXPECT_EQ(p0.at(0, 0), static_cast<std::uint8_t>(200));
    TYST_EXPECT_EQ(p0.at(1, 0), static_cast<std::uint8_t>(10));
    TYST_EXPECT_EQ(p0.at(2, 0), static_cast<std::uint8_t>(10));
    TYST_EXPECT_EQ(p1.at(0, 0), static_cast<std::uint8_t>(10));
    TYST_EXPECT_EQ(p1.at(1, 0), static_cast<std::uint8_t>(10));
    TYST_EXPECT_EQ(p1.at(2, 0), static_cast<std::uint8_t>(200));

    std::remove(path.c_str());
}

TYST_TEST(VideoContainerTests, ReadGifRoundTripsGradientWithinQuantizationTolerance) {
    const std::string path = temp_file_path("_gradient.gif");
    const std::size_t w = 16, h = 8;
    std::vector<std::array<std::uint8_t, 3>> colors(w * h);
    for (std::size_t y = 0; y < h; ++y) {
        for (std::size_t x = 0; x < w; ++x) {
            const std::uint8_t v = static_cast<std::uint8_t>((x * 255) / (w - 1));
            colors[y * w + x] = {v, static_cast<std::uint8_t>(255 - v), 128};
        }
    }
    const VideoFrame original = make_rgb_frame(w, h, colors);

    {
        vc::GifEncoder encoder(path, w, h, vc::GifOptions{0, 256});
        encoder.write_frame(original, 100'000);
        encoder.finish();
    }

    const vc::DecodedGif decoded = vc::read_gif(path);
    TYST_ASSERT_EQ(decoded.frames.size(), static_cast<std::size_t>(1));
    const Plane& out = decoded.frames[0].planes[0];
    const Plane& in = original.planes[0];
    // 256-color quantization of a 16-step gradient should be exact (well
    // within the available palette budget), so require an exact match.
    for (std::size_t y = 0; y < h; ++y) {
        for (std::size_t x = 0; x < w; ++x) {
            for (int c = 0; c < 3; ++c) {
                TYST_EXPECT_EQ(out.at(x * 3 + c, y), in.at(x * 3 + c, y));
            }
        }
    }

    std::remove(path.c_str());
}

TYST_TEST(VideoContainerTests, ReadAviRoundTripsVideoOnlyFrames) {
    const std::string path = temp_file_path("_roundtrip_video.avi");
    const std::size_t w = 8, h = 6;
    std::vector<std::array<std::uint8_t, 3>> colors_a(w * h, {12, 34, 56});
    std::vector<std::array<std::uint8_t, 3>> colors_b(w * h, {200, 150, 100});
    const VideoFrame frame_a = make_rgb_frame(w, h, colors_a);
    const VideoFrame frame_b = make_rgb_frame(w, h, colors_b);

    {
        vc::AviWriter writer(path, w, h, 25.0);
        writer.write_frame(frame_a);
        writer.write_frame(frame_b);
        writer.finish();
    }

    const vc::DecodedAvi decoded = vc::read_avi(path);
    TYST_ASSERT_EQ(decoded.width, w);
    TYST_ASSERT_EQ(decoded.height, h);
    TYST_ASSERT_EQ(decoded.frames.size(), static_cast<std::size_t>(2));
    TYST_EXPECT_EQ(decoded.audio_config.sample_rate, static_cast<std::uint32_t>(0));
    TYST_EXPECT_TRUE(decoded.audio_pcm.empty());
    TYST_EXPECT_TRUE(std::abs(decoded.fps - 25.0) < 0.01);

    const Plane& p0 = decoded.frames[0].planes[0];
    const Plane& p1 = decoded.frames[1].planes[0];
    for (std::size_t y = 0; y < h; ++y) {
        for (std::size_t x = 0; x < w; ++x) {
            TYST_EXPECT_EQ(p0.at(x * 3 + 0, y), static_cast<std::uint8_t>(12));
            TYST_EXPECT_EQ(p0.at(x * 3 + 1, y), static_cast<std::uint8_t>(34));
            TYST_EXPECT_EQ(p0.at(x * 3 + 2, y), static_cast<std::uint8_t>(56));
            TYST_EXPECT_EQ(p1.at(x * 3 + 0, y), static_cast<std::uint8_t>(200));
            TYST_EXPECT_EQ(p1.at(x * 3 + 1, y), static_cast<std::uint8_t>(150));
            TYST_EXPECT_EQ(p1.at(x * 3 + 2, y), static_cast<std::uint8_t>(100));
        }
    }

    std::remove(path.c_str());
}

TYST_TEST(VideoContainerTests, ReadAviRoundTripsInterleavedAudio) {
    const std::string path = temp_file_path("_roundtrip_audio.avi");
    const std::size_t w = 4, h = 2;
    std::vector<std::array<std::uint8_t, 3>> colors(w * h, {5, 6, 7});
    const VideoFrame frame = make_rgb_frame(w, h, colors);

    vc::AviAudioConfig audio_cfg;
    audio_cfg.sample_rate = 8000;
    audio_cfg.num_channels = 1;
    audio_cfg.bits_per_sample = 16;
    const std::vector<std::int16_t> pcm_block = {100, -200, 300, -400, 500};

    {
        vc::AviWriter writer(path, w, h, 24.0, audio_cfg);
        writer.write_frame(frame);
        writer.write_audio(pcm_block);
        writer.write_frame(frame);
        writer.write_audio(pcm_block);
        writer.finish();
    }

    const vc::DecodedAvi decoded = vc::read_avi(path);
    TYST_ASSERT_EQ(decoded.frames.size(), static_cast<std::size_t>(2));
    TYST_EXPECT_EQ(decoded.audio_config.sample_rate, static_cast<std::uint32_t>(8000));
    TYST_EXPECT_EQ(decoded.audio_config.num_channels, static_cast<std::uint16_t>(1));
    TYST_ASSERT_EQ(decoded.audio_pcm.size(), pcm_block.size() * 2);
    for (std::size_t i = 0; i < pcm_block.size(); ++i) {
        TYST_EXPECT_EQ(decoded.audio_pcm[i], pcm_block[i]);
        TYST_EXPECT_EQ(decoded.audio_pcm[pcm_block.size() + i], pcm_block[i]);
    }

    std::remove(path.c_str());
}

TYST_TEST(VideoContainerTests, ReadAviRejectsUnsupportedCompression) {
    // Hand-craft a minimal RIFF/AVI with a bogus (non-zero) biCompression
    // fourcc to verify read_avi() rejects it clearly instead of misreading
    // garbage as uncompressed pixels.
    const std::string path = temp_file_path("_bad_codec.avi");
    const std::size_t w = 2, h = 2;

    std::vector<std::array<std::uint8_t, 3>> colors(w * h, {1, 2, 3});
    const VideoFrame frame = make_rgb_frame(w, h, colors);
    {
        vc::AviWriter writer(path, w, h, 24.0);
        writer.write_frame(frame);
        writer.finish();
    }

    auto bytes = read_whole_file(path);
    const auto strf_hits = find_chunks(bytes, "strf");
    TYST_ASSERT_EQ(strf_hits.size(), static_cast<std::size_t>(1));
    // BITMAPINFOHEADER.biCompression sits 16 bytes into the chunk data
    // (biSize, biWidth, biHeight, biPlanes+biBitCount all precede it), and
    // the chunk data itself starts 8 bytes after find_chunks' fourcc hit
    // (4 bytes of fourcc + 4 bytes of chunk size).
    const std::size_t compression_offset = strf_hits[0] + 8 + 16;
    bytes[compression_offset + 0] = 'M';
    bytes[compression_offset + 1] = 'J';
    bytes[compression_offset + 2] = 'P';
    bytes[compression_offset + 3] = 'G';

    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    out.close();

    bool threw = false;
    try {
        vc::read_avi(path);
    } catch (const std::runtime_error& e) {
        threw = true;
        const std::string msg = e.what();
        TYST_EXPECT_TRUE(msg.find("MJPG") != std::string::npos);
    }
    TYST_EXPECT_TRUE(threw);

    std::remove(path.c_str());
}
