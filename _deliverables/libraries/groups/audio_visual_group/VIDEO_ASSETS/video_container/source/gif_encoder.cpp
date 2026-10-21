// GifEncoder — assembles a GIF89a animation file from a sequence of
// VideoFrames using median_cut_quantize() for palette reduction and
// gif_lzw_compress() for entropy coding of the indexed pixel stream.
#include "../headers/video_container.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>

namespace trekker {
namespace video {
namespace container {

namespace {

void write_u8(std::ofstream& out, std::uint8_t v) {
    out.put(static_cast<char>(v));
}

void write_u16le(std::ofstream& out, std::uint16_t v) {
    write_u8(out, static_cast<std::uint8_t>(v & 0xFF));
    write_u8(out, static_cast<std::uint8_t>((v >> 8) & 0xFF));
}

// Smallest power-of-two color-table size (2..256) that can hold `count`
// palette entries, expressed as the GIF "size of color table" field N where
// table size = 2^(N+1).
int color_table_size_field(std::size_t count) {
    int n = 1; // minimum table holds 2 entries (field value 0)
    while ((1 << (n + 1)) < static_cast<int>(count) && n < 7) ++n;
    return n;
}

int min_code_size_for(std::size_t palette_entries) {
    int bits = 2; // GIF requires at least 2-bit codes even for very small palettes
    while ((1u << bits) < palette_entries && bits < 8) ++bits;
    return bits;
}

void write_sub_blocks(std::ofstream& out, const std::vector<std::uint8_t>& data) {
    std::size_t offset = 0;
    while (offset < data.size()) {
        const std::size_t chunk = std::min<std::size_t>(255, data.size() - offset);
        write_u8(out, static_cast<std::uint8_t>(chunk));
        out.write(reinterpret_cast<const char*>(data.data() + offset), static_cast<std::streamsize>(chunk));
        offset += chunk;
    }
    write_u8(out, 0x00); // block terminator
}

} // namespace

struct GifEncoder::Impl {
    std::ofstream out;
    std::size_t width = 0;
    std::size_t height = 0;
    GifOptions options;
    std::size_t frames_written = 0;
    bool finished = false;
};

GifEncoder::GifEncoder(const std::string& path, std::size_t width, std::size_t height,
                      GifOptions options)
    : impl_(new Impl()) {
    if (width == 0 || height == 0 || width > 0xFFFF || height > 0xFFFF) {
        throw std::invalid_argument("GifEncoder: invalid dimensions");
    }
    impl_->width = width;
    impl_->height = height;
    impl_->options = options;
    impl_->options.max_colors = std::max(2, std::min(256, options.max_colors));

    impl_->out.open(path, std::ios::binary | std::ios::trunc);
    if (!impl_->out) {
        throw std::runtime_error("GifEncoder: failed to open output path: " + path);
    }

    // Header
    impl_->out.write("GIF89a", 6);

    // Logical Screen Descriptor. We defer to a *global* color table sized for
    // the requested max_colors so every frame can reuse the same header;
    // per-frame palettes are instead stored as local color tables, which
    // keeps the encoder simple while still giving each frame full-fidelity
    // quantization independent of earlier frames.
    write_u16le(impl_->out, static_cast<std::uint16_t>(width));
    write_u16le(impl_->out, static_cast<std::uint16_t>(height));
    const std::uint8_t packed = 0x00; // no global color table (bit 7 clear)
    write_u8(impl_->out, packed);
    write_u8(impl_->out, 0x00); // background color index
    write_u8(impl_->out, 0x00); // pixel aspect ratio (unused)

    // NETSCAPE2.0 application extension so the animation loops.
    write_u8(impl_->out, 0x21); // extension introducer
    write_u8(impl_->out, 0xFF); // application extension label
    write_u8(impl_->out, 0x0B); // block size (11)
    impl_->out.write("NETSCAPE2.0", 11);
    write_u8(impl_->out, 0x03); // sub-block size
    write_u8(impl_->out, 0x01); // sub-block id
    write_u16le(impl_->out, static_cast<std::uint16_t>(std::max(0, impl_->options.loop_count)));
    write_u8(impl_->out, 0x00); // block terminator
}

GifEncoder::~GifEncoder() {
    finish();
    delete impl_;
}

void GifEncoder::write_frame(const VideoFrame& frame, std::int64_t frame_delay_us) {
    if (impl_->finished) throw std::logic_error("GifEncoder: write_frame() after finish()");
    if (frame.width != impl_->width || frame.height != impl_->height) {
        throw std::invalid_argument("GifEncoder: frame dimensions do not match the encoder");
    }

    const QuantizedImage quant = median_cut_quantize(frame, impl_->options.max_colors);
    const std::size_t palette_entries = quant.palette.size() / 3;
    const int table_size_field = color_table_size_field(palette_entries);
    const std::size_t table_entries = static_cast<std::size_t>(1) << (table_size_field + 1);

    // Graphic Control Extension (per-frame delay, in centiseconds).
    std::int64_t delay_cs = frame_delay_us / 10000; // us -> 1/100s
    delay_cs = std::max<std::int64_t>(1, delay_cs);
    delay_cs = std::min<std::int64_t>(0xFFFF, delay_cs);

    write_u8(impl_->out, 0x21); // extension introducer
    write_u8(impl_->out, 0xF9); // graphic control label
    write_u8(impl_->out, 0x04); // block size
    write_u8(impl_->out, 0x04); // packed: disposal method = 2 (restore to background)
    write_u16le(impl_->out, static_cast<std::uint16_t>(delay_cs));
    write_u8(impl_->out, 0x00); // transparent color index (unused)
    write_u8(impl_->out, 0x00); // block terminator

    // Image Descriptor with a local color table.
    write_u8(impl_->out, 0x2C); // image separator
    write_u16le(impl_->out, 0); // left
    write_u16le(impl_->out, 0); // top
    write_u16le(impl_->out, static_cast<std::uint16_t>(impl_->width));
    write_u16le(impl_->out, static_cast<std::uint16_t>(impl_->height));
    const std::uint8_t image_packed = static_cast<std::uint8_t>(0x80 | table_size_field); // local color table present
    write_u8(impl_->out, image_packed);

    for (std::size_t i = 0; i < table_entries; ++i) {
        if (i < palette_entries) {
            write_u8(impl_->out, quant.palette[i * 3 + 0]);
            write_u8(impl_->out, quant.palette[i * 3 + 1]);
            write_u8(impl_->out, quant.palette[i * 3 + 2]);
        } else {
            write_u8(impl_->out, 0);
            write_u8(impl_->out, 0);
            write_u8(impl_->out, 0);
        }
    }

    const int min_code_size = min_code_size_for(std::max<std::size_t>(2, palette_entries));
    write_u8(impl_->out, static_cast<std::uint8_t>(min_code_size));
    const std::vector<std::uint8_t> compressed = gif_lzw_compress(quant.indices, min_code_size);
    write_sub_blocks(impl_->out, compressed);

    ++impl_->frames_written;
}

void GifEncoder::finish() {
    if (impl_->finished) return;
    impl_->finished = true;
    if (impl_->out.is_open()) {
        write_u8(impl_->out, 0x3B); // GIF trailer
        impl_->out.close();
    }
}

std::size_t GifEncoder::frame_count() const { return impl_->frames_written; }

} // namespace container
} // namespace video
} // namespace trekker
