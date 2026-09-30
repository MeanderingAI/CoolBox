#ifndef TREKKER_VIDEO_ASSETS_AUDIO_CONTAINER_H
#define TREKKER_VIDEO_ASSETS_AUDIO_CONTAINER_H

// audio_container — reads/writes standard RIFF/WAVE files for the planar
// trekker::audio::AudioBuffer model used throughout audio_processing.
//
// Two encodings are supported:
//   * PCM16    — plain uncompressed 16-bit signed PCM (the WAV lingua franca).
//   * ImaAdpcm — a genuine ~4:1 lossy compressor (4-bit IMA/DVI ADPCM,
//                WAVE_FORMAT_IMA_ADPCM / 0x0011), hand-rolled rather than
//                pulled from a third-party codec library. Mono/stereo block
//                layout matches the classic Microsoft IMA ADPCM WAV spec
//                (per-channel 4-byte block header + round-robin 8-sample
//                groups); channel counts above 2 use the same round-robin
//                scheme generalised, which is internally self-consistent but
//                not a standardised multi-channel layout.

#include "../../audio_processing/headers/audio_processing.h"

#include <cstdint>
#include <string>
#include <vector>

namespace trekker {
namespace audio {
namespace container {

enum class WavEncoding {
    PCM16,
    ImaAdpcm,
};

// Writes `buffer` to `path` as a RIFF/WAVE file. Samples are converted to
// 16-bit range regardless of the buffer's original SampleFormat.
// `adpcm_block_align` only applies to WavEncoding::ImaAdpcm (default 256
// bytes, ~1024 mono samples/block); it is rounded down internally so the
// per-channel block body divides evenly into 8-sample groups.
void write_wav(const std::string& path, const AudioBuffer& buffer,
              WavEncoding encoding = WavEncoding::PCM16,
              std::uint16_t adpcm_block_align = 256);

// Reads a RIFF/WAVE file (PCM 8/16/32-bit or IMA ADPCM) back into a planar
// F32 AudioBuffer (samples normalised to [-1,1]). Throws std::runtime_error
// for malformed input or unsupported encodings/bit depths.
AudioBuffer read_wav(const std::string& path);

// ── IMA ADPCM building blocks (exposed for testing/reuse) ────────────────────

struct ImaAdpcmState {
    std::int32_t predictor = 0;
    int step_index = 0;
};

// Encodes `samples` (raw S16-range values) into 4-bit IMA ADPCM nibbles
// packed two-per-byte (first sample in the low nibble), updating `state` in
// place. Does not emit the per-block header (initial predictor/step index);
// callers that need a complete WAV block must prepend that separately.
std::vector<std::uint8_t> ima_adpcm_encode_nibbles(const std::vector<std::int16_t>& samples,
                                                   ImaAdpcmState& state);

// Inverse of ima_adpcm_encode_nibbles: decodes `sample_count` samples from
// packed nibbles, updating `state` in place.
std::vector<std::int16_t> ima_adpcm_decode_nibbles(const std::vector<std::uint8_t>& nibble_bytes,
                                                   std::size_t sample_count,
                                                   ImaAdpcmState& state);

} // namespace container
} // namespace audio
} // namespace trekker

#endif // TREKKER_VIDEO_ASSETS_AUDIO_CONTAINER_H
