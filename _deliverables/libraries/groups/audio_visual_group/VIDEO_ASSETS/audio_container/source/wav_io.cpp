// RIFF/WAVE reader & writer supporting PCM (8/16/32-bit int, 32-bit float)
// and a hand-rolled IMA ADPCM (WAVE_FORMAT_IMA_ADPCM / 0x0011) codec.
#include "../headers/audio_container.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <stdexcept>

namespace trekker {
namespace audio {
namespace container {

namespace {

constexpr std::uint16_t kFormatPcm = 1;
constexpr std::uint16_t kFormatIeeeFloat = 3;
constexpr std::uint16_t kFormatImaAdpcm = 0x0011;

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
void w_fourcc(std::ofstream& out, const char cc[4]) { out.write(cc, 4); }

std::uint16_t r_u16le(const std::uint8_t* p) {
    return static_cast<std::uint16_t>(p[0]) | (static_cast<std::uint16_t>(p[1]) << 8);
}
std::uint32_t r_u32le(const std::uint8_t* p) {
    return static_cast<std::uint32_t>(p[0]) | (static_cast<std::uint32_t>(p[1]) << 8) |
           (static_cast<std::uint32_t>(p[2]) << 16) | (static_cast<std::uint32_t>(p[3]) << 24);
}

// Converts an AudioBuffer's planar samples (any SampleFormat) to raw S16
// per-channel arrays, ready for either PCM16 packing or ADPCM encoding.
std::vector<std::vector<std::int16_t>> to_s16_planes(const AudioBuffer& buffer) {
    std::vector<std::vector<std::int16_t>> planes(buffer.num_channels);
    const std::size_t n = buffer.num_samples();
    for (std::uint32_t c = 0; c < buffer.num_channels; ++c) {
        planes[c].resize(n);
        const auto& src = buffer.planes[c];
        for (std::size_t i = 0; i < n; ++i) {
            double v = src[i];
            switch (buffer.format) {
                case SampleFormat::F32:
                case SampleFormat::F64:
                    v = std::clamp(v, -1.0, 1.0) * 32767.0;
                    break;
                case SampleFormat::S16:
                    break; // already in range
                case SampleFormat::S32:
                    v /= 65536.0; // map 32-bit range down to 16-bit
                    break;
            }
            v = std::clamp(v, -32768.0, 32767.0);
            planes[c][i] = static_cast<std::int16_t>(std::lround(v));
        }
    }
    return planes;
}

std::size_t round_down_to_multiple(std::size_t value, std::size_t multiple) {
    if (multiple == 0) return value;
    return (value / multiple) * multiple;
}

} // namespace

// ── Writing ───────────────────────────────────────────────────────────────────

void write_wav(const std::string& path, const AudioBuffer& buffer,
              WavEncoding encoding, std::uint16_t adpcm_block_align) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) throw std::runtime_error("write_wav: failed to open output path: " + path);

    const std::uint16_t channels = static_cast<std::uint16_t>(buffer.num_channels);
    if (channels == 0) throw std::invalid_argument("write_wav: buffer has no channels");
    const std::vector<std::vector<std::int16_t>> planes = to_s16_planes(buffer);
    const std::size_t num_samples = buffer.num_samples();

    if (encoding == WavEncoding::PCM16) {
        const std::uint16_t block_align = static_cast<std::uint16_t>(channels * 2);
        const std::uint32_t avg_bytes_per_sec = buffer.sample_rate * block_align;
        const std::uint32_t data_bytes = static_cast<std::uint32_t>(num_samples * block_align);
        const std::uint32_t fmt_chunk_size = 16;
        const std::uint32_t riff_size = 4 /*WAVE*/ + (8 + fmt_chunk_size) + (8 + data_bytes) +
                                        (data_bytes % 2);

        w_fourcc(out, "RIFF");
        w_u32le(out, riff_size);
        w_fourcc(out, "WAVE");

        w_fourcc(out, "fmt ");
        w_u32le(out, fmt_chunk_size);
        w_u16le(out, kFormatPcm);
        w_u16le(out, channels);
        w_u32le(out, buffer.sample_rate);
        w_u32le(out, avg_bytes_per_sec);
        w_u16le(out, block_align);
        w_u16le(out, 16);

        w_fourcc(out, "data");
        w_u32le(out, data_bytes);
        for (std::size_t i = 0; i < num_samples; ++i) {
            for (std::uint16_t c = 0; c < channels; ++c) {
                w_u16le(out, static_cast<std::uint16_t>(planes[c][i]));
            }
        }
        if (data_bytes % 2 != 0) w_u8(out, 0);
        return;
    }

    // ── IMA ADPCM ──────────────────────────────────────────────────────────
    const std::size_t header_bytes = 4u * channels;
    if (adpcm_block_align <= header_bytes) {
        throw std::invalid_argument("write_wav: adpcm_block_align too small for the channel count");
    }
    const std::size_t body_bytes = round_down_to_multiple(
        static_cast<std::size_t>(adpcm_block_align) - header_bytes, 4u * channels);
    if (body_bytes == 0) {
        throw std::invalid_argument("write_wav: adpcm_block_align leaves no room for a full sample group");
    }
    const std::size_t effective_block_align = header_bytes + body_bytes;
    const std::size_t rounds_per_block = body_bytes / (4u * channels);
    const std::size_t samples_per_block = 1u + rounds_per_block * 8u;

    const std::size_t num_blocks = (num_samples == 0) ? 0
        : (num_samples + samples_per_block - 1) / samples_per_block;

    const std::uint32_t avg_bytes_per_sec = static_cast<std::uint32_t>(
        (static_cast<double>(effective_block_align) * buffer.sample_rate) / samples_per_block);
    const std::uint32_t fmt_chunk_size = 20; // 16 base + cbSize(2) + wSamplesPerBlock(2)
    const std::uint32_t data_bytes = static_cast<std::uint32_t>(num_blocks * effective_block_align);
    const std::uint32_t riff_size = 4 + (8 + fmt_chunk_size) + (8 + 4 /*fact*/) + (8 + data_bytes) +
                                    (data_bytes % 2);

    w_fourcc(out, "RIFF");
    w_u32le(out, riff_size);
    w_fourcc(out, "WAVE");

    w_fourcc(out, "fmt ");
    w_u32le(out, fmt_chunk_size);
    w_u16le(out, kFormatImaAdpcm);
    w_u16le(out, channels);
    w_u32le(out, buffer.sample_rate);
    w_u32le(out, avg_bytes_per_sec);
    w_u16le(out, static_cast<std::uint16_t>(effective_block_align));
    w_u16le(out, 4); // wBitsPerSample
    w_u16le(out, 2); // cbSize (extra format bytes that follow)
    w_u16le(out, static_cast<std::uint16_t>(samples_per_block));

    w_fourcc(out, "fact");
    w_u32le(out, 4);
    w_u32le(out, static_cast<std::uint32_t>(num_samples));

    w_fourcc(out, "data");
    w_u32le(out, data_bytes);

    std::vector<ImaAdpcmState> state(channels);
    for (std::size_t block = 0; block < num_blocks; ++block) {
        const std::size_t block_start = block * samples_per_block;

        std::vector<std::int16_t> first_sample(channels);
        for (std::uint16_t c = 0; c < channels; ++c) {
            first_sample[c] = block_start < num_samples ? planes[c][block_start]
                                                        : (block_start == 0 ? 0 : planes[c][num_samples - 1]);
            state[c].predictor = first_sample[c];
        }
        for (std::uint16_t c = 0; c < channels; ++c) {
            w_u16le(out, static_cast<std::uint16_t>(first_sample[c]));
            w_u8(out, static_cast<std::uint8_t>(state[c].step_index));
            w_u8(out, 0); // reserved
        }

        // Gather (samples_per_block - 1) samples per channel for the body,
        // padding the tail of the final block by repeating the last real
        // sample (the "fact" chunk's true count lets read_wav() truncate).
        std::vector<std::vector<std::int16_t>> body(channels);
        for (std::uint16_t c = 0; c < channels; ++c) {
            body[c].resize(rounds_per_block * 8);
            for (std::size_t k = 0; k < body[c].size(); ++k) {
                const std::size_t src_idx = block_start + 1 + k;
                body[c][k] = src_idx < num_samples ? planes[c][src_idx]
                                                   : (num_samples == 0 ? 0 : planes[c][num_samples - 1]);
            }
        }

        for (std::size_t round = 0; round < rounds_per_block; ++round) {
            for (std::uint16_t c = 0; c < channels; ++c) {
                std::vector<std::int16_t> group(body[c].begin() + static_cast<long>(round * 8),
                                                body[c].begin() + static_cast<long>(round * 8 + 8));
                const auto packed = ima_adpcm_encode_nibbles(group, state[c]);
                out.write(reinterpret_cast<const char*>(packed.data()), static_cast<std::streamsize>(packed.size()));
            }
        }
    }
    if (data_bytes % 2 != 0) w_u8(out, 0);
}

// ── Reading ───────────────────────────────────────────────────────────────────

namespace {

struct RiffChunk {
    std::string fourcc;
    std::size_t data_offset;
    std::uint32_t size;
};

std::vector<RiffChunk> parse_chunks(const std::vector<std::uint8_t>& bytes, std::size_t start, std::size_t end) {
    std::vector<RiffChunk> chunks;
    std::size_t pos = start;
    while (pos + 8 <= end) {
        std::string fourcc(reinterpret_cast<const char*>(&bytes[pos]), 4);
        const std::uint32_t size = r_u32le(&bytes[pos + 4]);
        chunks.push_back({fourcc, pos + 8, size});
        pos += 8 + size + (size % 2);
    }
    return chunks;
}

const RiffChunk* find_chunk(const std::vector<RiffChunk>& chunks, const std::string& fourcc) {
    for (const auto& c : chunks) if (c.fourcc == fourcc) return &c;
    return nullptr;
}

} // namespace

AudioBuffer read_wav(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("read_wav: failed to open input path: " + path);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    if (bytes.size() < 12 || std::memcmp(bytes.data(), "RIFF", 4) != 0 ||
        std::memcmp(bytes.data() + 8, "WAVE", 4) != 0) {
        throw std::runtime_error("read_wav: not a RIFF/WAVE file: " + path);
    }

    const auto chunks = parse_chunks(bytes, 12, bytes.size());
    const RiffChunk* fmt = find_chunk(chunks, "fmt ");
    const RiffChunk* data = find_chunk(chunks, "data");
    if (!fmt || !data) throw std::runtime_error("read_wav: missing fmt/data chunk: " + path);
    if (fmt->size < 16) throw std::runtime_error("read_wav: fmt chunk too small: " + path);

    const std::uint16_t format_tag = r_u16le(&bytes[fmt->data_offset]);
    const std::uint16_t channels = r_u16le(&bytes[fmt->data_offset + 2]);
    const std::uint32_t sample_rate = r_u32le(&bytes[fmt->data_offset + 4]);
    const std::uint16_t block_align = r_u16le(&bytes[fmt->data_offset + 12]);
    const std::uint16_t bits_per_sample = r_u16le(&bytes[fmt->data_offset + 14]);
    if (channels == 0) throw std::runtime_error("read_wav: zero channels: " + path);

    AudioBuffer result;
    result.format = SampleFormat::F32;
    result.sample_rate = sample_rate;
    result.num_channels = channels;
    result.planes.assign(channels, {});

    if (format_tag == kFormatPcm || format_tag == kFormatIeeeFloat) {
        const std::size_t bytes_per_sample = bits_per_sample / 8;
        const std::size_t frame_bytes = bytes_per_sample * channels;
        if (frame_bytes == 0) throw std::runtime_error("read_wav: invalid bit depth: " + path);
        const std::size_t num_frames = data->size / frame_bytes;
        for (auto& p : result.planes) p.resize(num_frames);

        for (std::size_t i = 0; i < num_frames; ++i) {
            for (std::uint16_t c = 0; c < channels; ++c) {
                const std::uint8_t* p = &bytes[data->data_offset + i * frame_bytes + c * bytes_per_sample];
                float sample = 0.0f;
                if (format_tag == kFormatIeeeFloat && bits_per_sample == 32) {
                    std::uint32_t bits = r_u32le(p);
                    float f;
                    std::memcpy(&f, &bits, sizeof(f));
                    sample = f;
                } else if (bits_per_sample == 8) {
                    sample = (static_cast<int>(p[0]) - 128) / 128.0f; // WAV 8-bit PCM is unsigned
                } else if (bits_per_sample == 16) {
                    const std::int16_t v = static_cast<std::int16_t>(r_u16le(p));
                    sample = v / 32768.0f;
                } else if (bits_per_sample == 32) {
                    const std::int32_t v = static_cast<std::int32_t>(r_u32le(p));
                    sample = static_cast<float>(v / 2147483648.0);
                } else {
                    throw std::runtime_error("read_wav: unsupported PCM bit depth: " + path);
                }
                result.planes[c][i] = sample;
            }
        }
        return result;
    }

    if (format_tag == kFormatImaAdpcm) {
        if (fmt->size < 20) throw std::runtime_error("read_wav: IMA ADPCM fmt chunk missing extension: " + path);
        const std::uint16_t samples_per_block = r_u16le(&bytes[fmt->data_offset + 18]);
        if (block_align == 0 || samples_per_block == 0) {
            throw std::runtime_error("read_wav: invalid IMA ADPCM block parameters: " + path);
        }

        const RiffChunk* fact = find_chunk(chunks, "fact");
        const std::size_t num_blocks = data->size / block_align;
        const std::size_t decoded_capacity = num_blocks * samples_per_block;
        std::size_t true_sample_count = decoded_capacity;
        if (fact && fact->size >= 4) {
            true_sample_count = std::min<std::size_t>(r_u32le(&bytes[fact->data_offset]), decoded_capacity);
        }

        const std::size_t header_bytes = 4u * channels;
        if (block_align <= header_bytes) throw std::runtime_error("read_wav: block_align too small: " + path);
        const std::size_t body_bytes = block_align - header_bytes;
        const std::size_t rounds_per_block = body_bytes / (4u * channels);

        for (auto& p : result.planes) p.reserve(decoded_capacity);

        for (std::size_t block = 0; block < num_blocks; ++block) {
            const std::size_t block_offset = data->data_offset + block * block_align;
            std::vector<ImaAdpcmState> state(channels);
            std::vector<std::int16_t> first_sample(channels);
            for (std::uint16_t c = 0; c < channels; ++c) {
                const std::uint8_t* h = &bytes[block_offset + c * 4];
                first_sample[c] = static_cast<std::int16_t>(r_u16le(h));
                state[c].predictor = first_sample[c];
                state[c].step_index = h[2];
                if (state[c].step_index < 0 || state[c].step_index > 88) {
                    throw std::runtime_error("read_wav: corrupt IMA ADPCM step index: " + path);
                }
                result.planes[c].push_back(first_sample[c] / 32768.0f);
            }

            std::size_t body_offset = block_offset + header_bytes;
            for (std::size_t round = 0; round < rounds_per_block; ++round) {
                for (std::uint16_t c = 0; c < channels; ++c) {
                    std::vector<std::uint8_t> nibble_bytes(bytes.begin() + static_cast<long>(body_offset),
                                                           bytes.begin() + static_cast<long>(body_offset + 4));
                    const auto decoded = ima_adpcm_decode_nibbles(nibble_bytes, 8, state[c]);
                    for (auto s : decoded) result.planes[c].push_back(s / 32768.0f);
                    body_offset += 4;
                }
            }
        }

        for (auto& p : result.planes) {
            if (p.size() > true_sample_count) p.resize(true_sample_count);
        }
        return result;
    }

    throw std::runtime_error("read_wav: unsupported WAV format tag: " + path);
}

} // namespace container
} // namespace audio
} // namespace trekker
