// AviWriter — hand-rolled RIFF/AVI 1.0 muxer producing uncompressed BGR24
// video (optionally with an interleaved 16-bit PCM audio stream). This is a
// long-established, widely-supported "movie" container (no video codec is
// implemented — frames are stored as raw BI_RGB DIBs — but the file is a
// genuine, standards-compliant AVI that any media player / ffprobe can open).
#include "../headers/video_container.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <vector>

namespace trekker {
namespace video {
namespace container {

namespace {

void w_u8(std::ofstream& out, std::uint8_t v) { out.put(static_cast<char>(v)); }

void w_u16le(std::ofstream& out, std::uint16_t v) {
    w_u8(out, static_cast<std::uint8_t>(v & 0xFF));
    w_u8(out, static_cast<std::uint8_t>((v >> 8) & 0xFF));
}

void w_u32le(std::ofstream& out, std::uint32_t v) {
    w_u8(out, static_cast<std::uint8_t>(v & 0xFF));
    w_u8(out, static_cast<std::uint8_t>((v >> 8) & 0xFF));
    w_u8(out, static_cast<std::uint8_t>((v >> 16) & 0xFF));
    w_u8(out, static_cast<std::uint8_t>((v >> 24) & 0xFF));
}

void w_i32le(std::ofstream& out, std::int32_t v) { w_u32le(out, static_cast<std::uint32_t>(v)); }

void w_fourcc(std::ofstream& out, const char cc[4]) { out.write(cc, 4); }

// Writes a placeholder 32-bit field and returns the file offset it occupies
// so the real value can be seeked back to and patched once known.
std::streampos w_placeholder_u32(std::ofstream& out) {
    const std::streampos pos = out.tellp();
    w_u32le(out, 0);
    return pos;
}

void patch_u32le(std::ofstream& out, std::streampos pos, std::uint32_t value) {
    const std::streampos saved = out.tellp();
    out.seekp(pos);
    w_u32le(out, value);
    out.seekp(saved);
}

std::size_t dib_stride(std::size_t width) { return ((width * 3 + 3) / 4) * 4; }

struct IndexEntry {
    char fourcc[4];
    std::uint32_t flags;
    std::uint32_t offset; // relative to the first byte after the 'movi' fourcc
    std::uint32_t size;
};

constexpr std::uint32_t kAviIf1Keyframe = 0x00000010u; // AVIIF_KEYFRAME

} // namespace

struct AviWriter::Impl {
    std::ofstream out;
    std::size_t width = 0;
    std::size_t height = 0;
    double fps = 0.0;
    AviAudioConfig audio;
    bool has_audio = false;
    std::uint16_t block_align = 0;
    std::uint32_t avg_bytes_per_sec = 0;

    std::streampos riff_size_pos;
    std::streampos avih_total_frames_pos;
    std::streampos avih_max_bytes_per_sec_pos;
    std::streampos avih_suggested_buffer_pos;
    std::streampos vids_strh_length_pos;
    std::streampos auds_strh_length_pos;
    std::streampos movi_size_pos;
    std::streampos movi_data_start; // position right after the 'movi' fourcc

    std::size_t frame_count = 0;
    std::size_t audio_sample_frames = 0;
    std::uint32_t max_chunk_bytes = 0;
    bool finished = false;

    std::vector<IndexEntry> index;
};

AviWriter::AviWriter(const std::string& path, std::size_t width, std::size_t height,
                    double fps, AviAudioConfig audio_config)
    : impl_(new Impl()) {
    if (width == 0 || height == 0) throw std::invalid_argument("AviWriter: invalid dimensions");
    if (fps <= 0.0) throw std::invalid_argument("AviWriter: fps must be positive");

    impl_->width = width;
    impl_->height = height;
    impl_->fps = fps;
    impl_->audio = audio_config;
    impl_->has_audio = audio_config.sample_rate > 0;
    if (impl_->has_audio && audio_config.bits_per_sample != 16) {
        throw std::invalid_argument("AviWriter: only 16-bit PCM audio is supported");
    }
    if (impl_->has_audio) {
        impl_->block_align = static_cast<std::uint16_t>(audio_config.num_channels * (audio_config.bits_per_sample / 8));
        impl_->avg_bytes_per_sec = audio_config.sample_rate * impl_->block_align;
    }

    auto& out = impl_->out;
    out.open(path, std::ios::binary | std::ios::trunc);
    if (!out) throw std::runtime_error("AviWriter: failed to open output path: " + path);

    const std::size_t frame_bytes = dib_stride(width) * height;
    const std::uint32_t micro_sec_per_frame = static_cast<std::uint32_t>(1'000'000.0 / fps + 0.5);
    const std::uint32_t num_streams = impl_->has_audio ? 2u : 1u;

    // ── RIFF/AVI  ─────────────────────────────────────────────────────────
    w_fourcc(out, "RIFF");
    impl_->riff_size_pos = w_placeholder_u32(out);
    w_fourcc(out, "AVI ");

    // ── hdrl ──────────────────────────────────────────────────────────────
    w_fourcc(out, "LIST");
    const std::size_t hdrl_size = 4 /*'hdrl'*/
        + (8 + 56)                     // avih
        + (8 + (4 + (8 + 56) + (8 + 40))) // vids strl LIST
        + (impl_->has_audio ? (8 + (4 + (8 + 56) + (8 + 18))) : 0); // auds strl LIST
    w_u32le(out, static_cast<std::uint32_t>(hdrl_size));
    w_fourcc(out, "hdrl");

    // avih — MainAVIHeader
    w_fourcc(out, "avih");
    w_u32le(out, 56);
    w_u32le(out, micro_sec_per_frame);
    impl_->avih_max_bytes_per_sec_pos = w_placeholder_u32(out); // dwMaxBytesPerSec (patched)
    w_u32le(out, 0);                                            // dwPaddingGranularity
    w_u32le(out, 0x10 | 0x100);                                 // AVIF_HASINDEX | AVIF_ISINTERLEAVED
    impl_->avih_total_frames_pos = w_placeholder_u32(out);       // dwTotalFrames (patched)
    w_u32le(out, 0);                                            // dwInitialFrames
    w_u32le(out, num_streams);                                  // dwStreams
    impl_->avih_suggested_buffer_pos = w_placeholder_u32(out);   // dwSuggestedBufferSize (patched)
    w_u32le(out, static_cast<std::uint32_t>(width));
    w_u32le(out, static_cast<std::uint32_t>(height));
    for (int i = 0; i < 4; ++i) w_u32le(out, 0); // dwReserved[4]

    // strl (video)
    w_fourcc(out, "LIST");
    w_u32le(out, static_cast<std::uint32_t>(4 + (8 + 56) + (8 + 40)));
    w_fourcc(out, "strl");

    w_fourcc(out, "strh");
    w_u32le(out, 56);
    w_fourcc(out, "vids");
    w_fourcc(out, "DIB ");
    w_u32le(out, 0);              // dwFlags
    w_u16le(out, 0);              // wPriority
    w_u16le(out, 0);              // wLanguage
    w_u32le(out, 0);              // dwInitialFrames
    w_u32le(out, 1000);           // dwScale
    w_u32le(out, static_cast<std::uint32_t>(fps * 1000.0 + 0.5)); // dwRate (dwRate/dwScale = fps)
    w_u32le(out, 0);              // dwStart
    impl_->vids_strh_length_pos = w_placeholder_u32(out); // dwLength (patched)
    w_u32le(out, static_cast<std::uint32_t>(frame_bytes)); // dwSuggestedBufferSize
    w_u32le(out, 0xFFFFFFFFu);    // dwQuality (unspecified)
    w_u32le(out, 0);              // dwSampleSize (video: variable/unspecified)
    w_u16le(out, 0); w_u16le(out, 0); w_u16le(out, 0); w_u16le(out, 0); // rcFrame

    w_fourcc(out, "strf");
    w_u32le(out, 40);
    w_u32le(out, 40);                          // biSize
    w_i32le(out, static_cast<std::int32_t>(width));
    w_i32le(out, static_cast<std::int32_t>(height)); // positive => bottom-up rows
    w_u16le(out, 1);                           // biPlanes
    w_u16le(out, 24);                          // biBitCount
    w_u32le(out, 0);                           // biCompression = BI_RGB
    w_u32le(out, static_cast<std::uint32_t>(frame_bytes)); // biSizeImage
    w_i32le(out, 0);                           // biXPelsPerMeter
    w_i32le(out, 0);                           // biYPelsPerMeter
    w_u32le(out, 0);                           // biClrUsed
    w_u32le(out, 0);                           // biClrImportant

    // strl (audio)
    if (impl_->has_audio) {
        w_fourcc(out, "LIST");
        w_u32le(out, static_cast<std::uint32_t>(4 + (8 + 56) + (8 + 18)));
        w_fourcc(out, "strl");

        w_fourcc(out, "strh");
        w_u32le(out, 56);
        w_fourcc(out, "auds");
        w_u32le(out, 0);          // fccHandler (unspecified for PCM)
        w_u32le(out, 0);          // dwFlags
        w_u16le(out, 0);          // wPriority
        w_u16le(out, 0);          // wLanguage
        w_u32le(out, 0);          // dwInitialFrames
        w_u32le(out, impl_->block_align);      // dwScale
        w_u32le(out, impl_->avg_bytes_per_sec); // dwRate (dwRate/dwScale = sample rate)
        w_u32le(out, 0);          // dwStart
        impl_->auds_strh_length_pos = w_placeholder_u32(out); // dwLength (patched, sample frames)
        w_u32le(out, impl_->avg_bytes_per_sec); // dwSuggestedBufferSize (~1 second)
        w_u32le(out, 0xFFFFFFFFu); // dwQuality
        w_u32le(out, impl_->block_align);       // dwSampleSize
        w_u16le(out, 0); w_u16le(out, 0); w_u16le(out, 0); w_u16le(out, 0); // rcFrame

        w_fourcc(out, "strf");
        w_u32le(out, 18);
        w_u16le(out, 1);                        // wFormatTag = PCM
        w_u16le(out, audio_config.num_channels);
        w_u32le(out, audio_config.sample_rate);
        w_u32le(out, impl_->avg_bytes_per_sec);
        w_u16le(out, impl_->block_align);
        w_u16le(out, audio_config.bits_per_sample);
        w_u16le(out, 0);                        // cbSize
    }

    // ── movi ──────────────────────────────────────────────────────────────
    w_fourcc(out, "LIST");
    impl_->movi_size_pos = w_placeholder_u32(out);
    w_fourcc(out, "movi");
    impl_->movi_data_start = out.tellp();
}

AviWriter::~AviWriter() {
    finish();
    delete impl_;
}

namespace {
void write_riff_chunk(std::ofstream& out, const char fourcc[4],
                      const std::uint8_t* data, std::size_t size,
                      std::vector<IndexEntry>& index, std::streampos movi_data_start) {
    const std::streampos chunk_start = out.tellp();
    w_fourcc(out, fourcc);
    w_u32le(out, static_cast<std::uint32_t>(size));
    out.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(size));
    if (size % 2 != 0) w_u8(out, 0x00); // word-align padding (not counted in chunk size)

    IndexEntry entry{};
    std::memcpy(entry.fourcc, fourcc, 4);
    entry.flags = kAviIf1Keyframe; // every chunk here is independently decodable
    entry.offset = static_cast<std::uint32_t>(chunk_start - movi_data_start);
    entry.size = static_cast<std::uint32_t>(size);
    index.push_back(entry);
}
} // namespace

void AviWriter::write_frame(const VideoFrame& frame) {
    if (impl_->finished) throw std::logic_error("AviWriter: write_frame() after finish()");
    if (frame.width != impl_->width || frame.height != impl_->height) {
        throw std::invalid_argument("AviWriter: frame dimensions do not match the writer");
    }

    VideoFrame rgb = frame.format == PixelFormat::RGB24 ? frame : convert_format(frame, PixelFormat::RGB24);
    const Plane& src_plane = rgb.planes[0];
    const std::size_t stride = dib_stride(impl_->width);
    std::vector<std::uint8_t> dib(stride * impl_->height, 0);

    // DIB rows are stored bottom-up, and BGR (not RGB) byte order.
    for (std::size_t y = 0; y < impl_->height; ++y) {
        const std::size_t dst_row = impl_->height - 1 - y;
        std::uint8_t* dst_row_ptr = dib.data() + dst_row * stride;
        for (std::size_t x = 0; x < impl_->width; ++x) {
            const std::uint8_t r = src_plane.at(x * 3 + 0, y);
            const std::uint8_t g = src_plane.at(x * 3 + 1, y);
            const std::uint8_t b = src_plane.at(x * 3 + 2, y);
            dst_row_ptr[x * 3 + 0] = b;
            dst_row_ptr[x * 3 + 1] = g;
            dst_row_ptr[x * 3 + 2] = r;
        }
    }

    write_riff_chunk(impl_->out, "00db", dib.data(), dib.size(), impl_->index, impl_->movi_data_start);
    impl_->max_chunk_bytes = std::max<std::uint32_t>(impl_->max_chunk_bytes, static_cast<std::uint32_t>(dib.size()));
    ++impl_->frame_count;
}

void AviWriter::write_audio(const std::vector<std::int16_t>& interleaved_pcm) {
    if (impl_->finished) throw std::logic_error("AviWriter: write_audio() after finish()");
    if (!impl_->has_audio) throw std::logic_error("AviWriter: writer was constructed without an audio stream");
    if (interleaved_pcm.empty()) return;
    if (interleaved_pcm.size() % impl_->audio.num_channels != 0) {
        throw std::invalid_argument("AviWriter: sample count is not a multiple of the channel count");
    }

    const auto* bytes = reinterpret_cast<const std::uint8_t*>(interleaved_pcm.data());
    const std::size_t byte_count = interleaved_pcm.size() * sizeof(std::int16_t);
    write_riff_chunk(impl_->out, "01wb", bytes, byte_count, impl_->index, impl_->movi_data_start);
    impl_->max_chunk_bytes = std::max<std::uint32_t>(impl_->max_chunk_bytes, static_cast<std::uint32_t>(byte_count));
    impl_->audio_sample_frames += interleaved_pcm.size() / impl_->audio.num_channels;
}

void AviWriter::finish() {
    if (impl_->finished) return;
    impl_->finished = true;
    auto& out = impl_->out;
    if (!out.is_open()) return;

    // movi LIST size covers everything from the 'movi' fourcc to the last
    // chunk (inclusive of any padding byte already written).
    const std::streampos movi_end = out.tellp();
    const std::uint32_t movi_size = static_cast<std::uint32_t>(movi_end - impl_->movi_size_pos - 4);
    patch_u32le(out, impl_->movi_size_pos, movi_size);

    // idx1 — flat index of every chunk in movi, offsets relative to the
    // first byte after the 'movi' fourcc (the classic AVI 1.0 convention).
    const std::streampos idx1_start = out.tellp();
    w_fourcc(out, "idx1");
    const std::uint32_t idx1_size = static_cast<std::uint32_t>(impl_->index.size() * 16);
    w_u32le(out, idx1_size);
    for (const auto& entry : impl_->index) {
        w_fourcc(out, entry.fourcc);
        w_u32le(out, entry.flags);
        w_u32le(out, entry.offset);
        w_u32le(out, entry.size);
    }
    (void)idx1_start;

    // Patch header fields that depended on the total frame/sample counts.
    patch_u32le(out, impl_->avih_total_frames_pos, static_cast<std::uint32_t>(impl_->frame_count));
    patch_u32le(out, impl_->vids_strh_length_pos, static_cast<std::uint32_t>(impl_->frame_count));
    if (impl_->has_audio) {
        patch_u32le(out, impl_->auds_strh_length_pos, static_cast<std::uint32_t>(impl_->audio_sample_frames));
    }
    patch_u32le(out, impl_->avih_suggested_buffer_pos, impl_->max_chunk_bytes);
    const std::uint32_t max_bytes_per_sec = static_cast<std::uint32_t>(impl_->width * impl_->height * 3 * impl_->fps)
                                            + impl_->avg_bytes_per_sec;
    patch_u32le(out, impl_->avih_max_bytes_per_sec_pos, max_bytes_per_sec);

    const std::streampos file_end = out.tellp();
    const std::uint32_t riff_size = static_cast<std::uint32_t>(file_end - impl_->riff_size_pos - 4);
    patch_u32le(out, impl_->riff_size_pos, riff_size);

    out.close();
}

std::size_t AviWriter::frame_count() const { return impl_->frame_count; }
std::size_t AviWriter::audio_sample_frames() const { return impl_->audio_sample_frames; }

} // namespace container
} // namespace video
} // namespace trekker
