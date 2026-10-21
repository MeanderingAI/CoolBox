// AVI reader — decodes an uncompressed BI_RGB AVI file (the same subset
// AviWriter produces: one 'vids' stream of raw BGR24 DIB frames, optionally
// interleaved with one 'auds' stream of 16-bit PCM) back into RGB24
// VideoFrames (+ raw interleaved PCM audio if present). Any other
// compression fourcc (MJPEG, DivX, H.264-in-AVI, etc.) is rejected with a
// clear error naming the unsupported codec rather than producing garbage.
#include "../headers/video_container.h"

#include <cctype>
#include <cstring>
#include <fstream>
#include <stdexcept>

namespace trekker {
namespace video {
namespace container {

namespace {

std::uint16_t r_u16le(const std::uint8_t* p) {
    return static_cast<std::uint16_t>(p[0]) | (static_cast<std::uint16_t>(p[1]) << 8);
}
std::uint32_t r_u32le(const std::uint8_t* p) {
    return static_cast<std::uint32_t>(p[0]) | (static_cast<std::uint32_t>(p[1]) << 8) |
           (static_cast<std::uint32_t>(p[2]) << 16) | (static_cast<std::uint32_t>(p[3]) << 24);
}
std::int32_t r_i32le(const std::uint8_t* p) { return static_cast<std::int32_t>(r_u32le(p)); }

bool fourcc_is(const std::uint8_t* p, const char cc[4]) { return std::memcmp(p, cc, 4) == 0; }

std::size_t dib_stride(std::size_t width) { return ((width * 3 + 3) / 4) * 4; }

} // namespace

DecodedAvi read_avi(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("read_avi: failed to open input path: " + path);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    if (bytes.size() < 12 || !fourcc_is(&bytes[0], "RIFF") || !fourcc_is(&bytes[8], "AVI ")) {
        throw std::runtime_error("read_avi: not a RIFF/AVI file: " + path);
    }

    DecodedAvi result;
    bool have_video_format = false;
    bool have_audio_format = false;
    bool current_stream_is_audio = false; // which strl we're currently inside
    std::uint32_t video_rate = 0, video_scale = 1;

    // Walk the top-level RIFF payload: a flat sequence of chunks/LISTs
    // between the 12-byte RIFF header and EOF.
    std::size_t pos = 12;
    while (pos + 8 <= bytes.size()) {
        char fourcc[5] = {0};
        std::memcpy(fourcc, &bytes[pos], 4);
        const std::uint32_t size = r_u32le(&bytes[pos + 4]);
        const std::size_t data_start = pos + 8;
        if (data_start + size > bytes.size()) break; // truncated file; stop gracefully

        if (std::strcmp(fourcc, "LIST") == 0) {
            if (size < 4) { pos = data_start + size + (size % 2); continue; }
            char list_type[5] = {0};
            std::memcpy(list_type, &bytes[data_start], 4);
            const std::size_t list_end = data_start + size;

            if (std::strcmp(list_type, "hdrl") == 0) {
                // Walk hdrl's children: avih, and one or more "strl" LISTs.
                std::size_t p = data_start + 4;
                while (p + 8 <= list_end) {
                    char cc[5] = {0};
                    std::memcpy(cc, &bytes[p], 4);
                    const std::uint32_t csize = r_u32le(&bytes[p + 4]);
                    const std::size_t cdata = p + 8;
                    if (cdata + csize > list_end) break;

                    if (std::strcmp(cc, "avih") == 0 && csize >= 56) {
                        result.width = r_u32le(&bytes[cdata + 32]);
                        result.height = r_u32le(&bytes[cdata + 36]);
                    } else if (std::strcmp(cc, "LIST") == 0 && csize >= 4 &&
                              fourcc_is(&bytes[cdata], "strl")) {
                        const std::size_t strl_end = cdata + csize;
                        std::size_t sp = cdata + 4;
                        bool this_is_audio = false;
                        while (sp + 8 <= strl_end) {
                            char scc[5] = {0};
                            std::memcpy(scc, &bytes[sp], 4);
                            const std::uint32_t scsize = r_u32le(&bytes[sp + 4]);
                            const std::size_t sdata = sp + 8;
                            if (sdata + scsize > strl_end) break;

                            if (std::strcmp(scc, "strh") == 0 && scsize >= 56) {
                                this_is_audio = fourcc_is(&bytes[sdata], "auds");
                                if (!this_is_audio && !fourcc_is(&bytes[sdata], "vids")) {
                                    throw std::runtime_error("read_avi: unsupported stream type (only vids/auds are)");
                                }
                                if (!this_is_audio) {
                                    video_scale = r_u32le(&bytes[sdata + 20]);
                                    video_rate = r_u32le(&bytes[sdata + 24]);
                                }
                            } else if (std::strcmp(scc, "strf") == 0) {
                                if (this_is_audio) {
                                    if (scsize < 16) throw std::runtime_error("read_avi: truncated WAVEFORMATEX");
                                    const std::uint16_t format_tag = r_u16le(&bytes[sdata]);
                                    if (format_tag != 1) {
                                        throw std::runtime_error("read_avi: unsupported audio format tag "
                                                                 + std::to_string(format_tag) + " (only PCM is supported)");
                                    }
                                    result.audio_config.num_channels = r_u16le(&bytes[sdata + 2]);
                                    result.audio_config.sample_rate = r_u32le(&bytes[sdata + 4]);
                                    result.audio_config.bits_per_sample = r_u16le(&bytes[sdata + 14]);
                                    if (result.audio_config.bits_per_sample != 16) {
                                        throw std::runtime_error("read_avi: only 16-bit PCM audio is supported");
                                    }
                                    have_audio_format = true;
                                } else {
                                    if (scsize < 40) throw std::runtime_error("read_avi: truncated BITMAPINFOHEADER");
                                    const std::uint32_t compression = r_u32le(&bytes[sdata + 16]);
                                    const std::uint16_t bit_count = r_u16le(&bytes[sdata + 14]);
                                    if (compression != 0 /* BI_RGB */ || bit_count != 24) {
                                        char comp_fourcc[5] = {0};
                                        std::memcpy(comp_fourcc, &bytes[sdata + 16], 4);
                                        const bool printable = compression != 0 && isprint(comp_fourcc[0]);
                                        throw std::runtime_error(
                                            std::string("read_avi: unsupported video compression (") +
                                            (printable ? comp_fourcc : std::to_string(compression)) +
                                            "); only uncompressed 24-bit BI_RGB is supported");
                                    }
                                    have_video_format = true;
                                }
                            }
                            sp = sdata + scsize + (scsize % 2);
                        }
                    }
                    p = cdata + csize + (csize % 2);
                }
            } else if (std::strcmp(list_type, "movi") == 0) {
                const std::size_t movi_data_start = data_start + 4;
                std::size_t p = movi_data_start;
                while (p + 8 <= list_end) {
                    char cc[5] = {0};
                    std::memcpy(cc, &bytes[p], 4);
                    const std::uint32_t csize = r_u32le(&bytes[p + 4]);
                    const std::size_t cdata = p + 8;
                    if (cdata + csize > list_end) break;

                    const bool is_video = (cc[2] == 'd' && (cc[3] == 'b' || cc[3] == 'c'));
                    const bool is_audio = (cc[2] == 'w' && cc[3] == 'b');
                    if (is_video) {
                        if (!have_video_format || result.width == 0 || result.height == 0) {
                            throw std::runtime_error("read_avi: video frame chunk before stream format was read");
                        }
                        const std::size_t stride = dib_stride(result.width);
                        const std::size_t expected = stride * result.height;
                        if (csize < expected) throw std::runtime_error("read_avi: truncated video frame chunk");

                        VideoFrame frame = VideoFrame::blank(PixelFormat::RGB24, result.width, result.height);
                        Plane& plane = frame.planes[0];
                        for (std::size_t y = 0; y < result.height; ++y) {
                            const std::size_t dib_row = result.height - 1 - y; // DIB rows are bottom-up
                            const std::uint8_t* row = &bytes[cdata + dib_row * stride];
                            for (std::size_t x = 0; x < result.width; ++x) {
                                plane.at(x * 3 + 0, y) = row[x * 3 + 2]; // R (DIB stores B,G,R)
                                plane.at(x * 3 + 1, y) = row[x * 3 + 1]; // G
                                plane.at(x * 3 + 2, y) = row[x * 3 + 0]; // B
                            }
                        }
                        result.frames.push_back(std::move(frame));
                    } else if (is_audio) {
                        if (!have_audio_format) throw std::runtime_error("read_avi: audio chunk before stream format was read");
                        const std::size_t sample_count = csize / sizeof(std::int16_t);
                        const std::size_t base = result.audio_pcm.size();
                        result.audio_pcm.resize(base + sample_count);
                        for (std::size_t i = 0; i < sample_count; ++i) {
                            result.audio_pcm[base + i] = static_cast<std::int16_t>(r_u16le(&bytes[cdata + i * 2]));
                        }
                    }
                    p = cdata + csize + (csize % 2);
                }
            }
            pos = data_start + size + (size % 2);
        } else {
            // idx1 or any other top-level chunk we don't need to interpret.
            pos = data_start + size + (size % 2);
        }
    }

    (void)current_stream_is_audio;
    if (!have_video_format) throw std::runtime_error("read_avi: no supported video stream found in " + path);
    if (result.frames.empty()) throw std::runtime_error("read_avi: no video frames found in " + path);
    if (video_rate == 0 || video_scale == 0) {
        result.fps = 24.0; // defensive fallback; well-formed files always set this
    } else {
        // AVIStreamHeader convention: fps = dwRate / dwScale (e.g. AviWriter
        // writes dwScale=1000, dwRate=round(fps*1000)).
        result.fps = static_cast<double>(video_rate) / static_cast<double>(video_scale);
    }
    return result;
}

} // namespace container
} // namespace video
} // namespace trekker
