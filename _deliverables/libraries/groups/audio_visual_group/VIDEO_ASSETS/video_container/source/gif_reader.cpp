// GIF reader — decodes a GIF89a/GIF87a file (global/local color tables,
// LZW-compressed image data, Graphic Control Extension delays) back into a
// sequence of RGB24 VideoFrames. Interlaced images are not supported (rare
// in practice and not produced by GifEncoder).
#include "../headers/video_container.h"

#include <cstring>
#include <fstream>
#include <stdexcept>

namespace trekker {
namespace video {
namespace container {

namespace {

std::uint16_t read_u16le(const std::uint8_t* p) {
    return static_cast<std::uint16_t>(p[0]) | (static_cast<std::uint16_t>(p[1]) << 8);
}

struct Palette {
    std::vector<std::uint8_t> rgb; // triplets
    std::size_t size() const { return rgb.size() / 3; }
};

Palette read_color_table(const std::vector<std::uint8_t>& bytes, std::size_t& pos, int size_field) {
    Palette table;
    const std::size_t entries = static_cast<std::size_t>(1) << (size_field + 1);
    table.rgb.resize(entries * 3);
    if (pos + entries * 3 > bytes.size()) throw std::runtime_error("read_gif: truncated color table");
    std::memcpy(table.rgb.data(), &bytes[pos], entries * 3);
    pos += entries * 3;
    return table;
}

} // namespace

DecodedGif read_gif(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("read_gif: failed to open input path: " + path);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    if (bytes.size() < 13 || std::memcmp(bytes.data(), "GIF8", 4) != 0) {
        throw std::runtime_error("read_gif: not a GIF file: " + path);
    }

    DecodedGif result;
    std::size_t pos = 6; // skip "GIF87a"/"GIF89a"
    result.width = read_u16le(&bytes[pos]);
    result.height = read_u16le(&bytes[pos + 2]);
    const std::uint8_t screen_packed = bytes[pos + 4];
    pos += 7; // width(2) height(2) packed(1) bg_color(1) aspect(1)

    Palette global_table;
    if (screen_packed & 0x80) {
        global_table = read_color_table(bytes, pos, screen_packed & 0x07);
    }

    std::int64_t pending_delay_us = 100'000; // GIF default: 10 cs = 100ms if unspecified
    bool have_pending_gce = false;

    while (pos < bytes.size()) {
        const std::uint8_t marker = bytes[pos];
        if (marker == 0x3B) { // trailer
            break;
        }
        if (marker == 0x21) { // extension
            if (pos + 1 >= bytes.size()) throw std::runtime_error("read_gif: truncated extension");
            const std::uint8_t label = bytes[pos + 1];
            pos += 2;
            if (label == 0xF9) { // Graphic Control Extension
                if (pos + 4 >= bytes.size()) throw std::runtime_error("read_gif: truncated graphic control extension");
                const std::uint8_t block_size = bytes[pos];
                const std::uint16_t delay_cs = read_u16le(&bytes[pos + 2]);
                pending_delay_us = static_cast<std::int64_t>(delay_cs) * 10'000;
                if (pending_delay_us <= 0) pending_delay_us = 100'000;
                have_pending_gce = true;
                pos += 1 + block_size; // skip the size byte itself plus its payload
            }
            // Skip any remaining sub-blocks (covers GCE's own terminator and
            // any other extension type we don't otherwise interpret).
            while (pos < bytes.size() && bytes[pos] != 0x00) {
                const std::uint8_t block_size = bytes[pos];
                pos += 1 + block_size;
            }
            if (pos >= bytes.size()) throw std::runtime_error("read_gif: truncated extension block");
            pos += 1; // block terminator
            continue;
        }
        if (marker == 0x2C) { // image descriptor
            pos += 1;
            if (pos + 9 > bytes.size()) throw std::runtime_error("read_gif: truncated image descriptor");
            const std::uint16_t w = read_u16le(&bytes[pos + 4]);
            const std::uint16_t h = read_u16le(&bytes[pos + 6]);
            const std::uint8_t img_packed = bytes[pos + 8];
            pos += 9;

            if (img_packed & 0x40) throw std::runtime_error("read_gif: interlaced GIFs are not supported");

            Palette local_table;
            const Palette* active_table = &global_table;
            if (img_packed & 0x80) {
                local_table = read_color_table(bytes, pos, img_packed & 0x07);
                active_table = &local_table;
            }
            if (active_table->size() == 0) throw std::runtime_error("read_gif: no color table available for frame");

            if (pos >= bytes.size()) throw std::runtime_error("read_gif: truncated image data");
            const std::uint8_t min_code_size = bytes[pos];
            pos += 1;

            std::vector<std::uint8_t> compressed;
            while (pos < bytes.size() && bytes[pos] != 0x00) {
                const std::uint8_t block_size = bytes[pos];
                if (pos + 1 + block_size > bytes.size()) throw std::runtime_error("read_gif: truncated image sub-block");
                compressed.insert(compressed.end(), bytes.begin() + static_cast<long>(pos) + 1,
                                  bytes.begin() + static_cast<long>(pos) + 1 + block_size);
                pos += 1 + block_size;
            }
            if (pos >= bytes.size()) throw std::runtime_error("read_gif: missing image block terminator");
            pos += 1; // block terminator

            const auto indices = gif_lzw_decompress(compressed, min_code_size,
                                                    static_cast<std::size_t>(w) * h);
            if (indices.size() != static_cast<std::size_t>(w) * h) {
                throw std::runtime_error("read_gif: decoded fewer pixels than expected");
            }

            VideoFrame frame = VideoFrame::blank(PixelFormat::RGB24, w, h);
            Plane& plane = frame.planes[0];
            for (std::size_t y = 0; y < h; ++y) {
                for (std::size_t x = 0; x < w; ++x) {
                    const std::uint8_t idx = indices[y * w + x];
                    if (idx >= active_table->size()) throw std::runtime_error("read_gif: palette index out of range");
                    plane.at(x * 3 + 0, y) = active_table->rgb[idx * 3 + 0];
                    plane.at(x * 3 + 1, y) = active_table->rgb[idx * 3 + 1];
                    plane.at(x * 3 + 2, y) = active_table->rgb[idx * 3 + 2];
                }
            }
            result.frames.push_back(std::move(frame));
            result.frame_delay_us.push_back(pending_delay_us);

            pending_delay_us = 100'000;
            have_pending_gce = false;
            continue;
        }
        throw std::runtime_error("read_gif: unexpected block marker 0x" + std::to_string(marker));
    }
    (void)have_pending_gce;

    if (result.frames.empty()) throw std::runtime_error("read_gif: no image frames found in " + path);
    return result;
}

} // namespace container
} // namespace video
} // namespace trekker
