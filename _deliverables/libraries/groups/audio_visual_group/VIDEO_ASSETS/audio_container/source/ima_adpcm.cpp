// IMA/DVI ADPCM 4-bit codec — the standard reference algorithm (fixed
// step-size and index-adjustment tables) used by countless implementations
// since the format's introduction in the early 1990s.
#include "../headers/audio_container.h"

#include <algorithm>
#include <cstdint>

namespace trekker {
namespace audio {
namespace container {

namespace {

constexpr int kIndexTable[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8,
};

constexpr int kStepTable[89] = {
    7,     8,     9,     10,    11,    12,    13,    14,    16,    17,
    19,    21,    23,    25,    28,    31,    34,    37,    41,    45,
    50,    55,    60,    66,    73,    80,    88,    97,    107,   118,
    130,   143,   157,   173,   190,   209,   230,   253,   279,   307,
    337,   371,   408,   449,   494,   544,   598,   658,   724,   796,
    876,   963,   1060,  1166,  1282,  1411,  1552,  1707,  1878,  2066,
    2272,  2499,  2749,  3024,  3327,  3660,  4026,  4428,  4871,  5358,
    5894,  6484,  7132,  7845,  8630,  9493,  10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767,
};

std::uint8_t encode_sample(std::int16_t sample, ImaAdpcmState& state) {
    const int step = kStepTable[state.step_index];
    int diff = static_cast<int>(sample) - state.predictor;
    int sign = 0;
    if (diff < 0) {
        sign = 8;
        diff = -diff;
    }

    int delta = 0;
    int vpdiff = step >> 3;
    int remaining = diff;
    int cur_step = step;

    if (remaining >= cur_step) {
        delta |= 4;
        remaining -= cur_step;
        vpdiff += cur_step;
    }
    cur_step >>= 1;
    if (remaining >= cur_step) {
        delta |= 2;
        remaining -= cur_step;
        vpdiff += cur_step;
    }
    cur_step >>= 1;
    if (remaining >= cur_step) {
        delta |= 1;
        vpdiff += cur_step;
    }

    int predictor = state.predictor + (sign ? -vpdiff : vpdiff);
    predictor = std::clamp(predictor, -32768, 32767);
    state.predictor = predictor;

    int step_index = state.step_index + kIndexTable[sign | delta];
    state.step_index = std::clamp(step_index, 0, 88);

    return static_cast<std::uint8_t>(sign | delta);
}

std::int16_t decode_nibble(std::uint8_t nibble, ImaAdpcmState& state) {
    const int step = kStepTable[state.step_index];
    int vpdiff = step >> 3;
    if (nibble & 4) vpdiff += step;
    if (nibble & 2) vpdiff += step >> 1;
    if (nibble & 1) vpdiff += step >> 2;

    int predictor = state.predictor + ((nibble & 8) ? -vpdiff : vpdiff);
    predictor = std::clamp(predictor, -32768, 32767);
    state.predictor = predictor;

    int step_index = state.step_index + kIndexTable[nibble];
    state.step_index = std::clamp(step_index, 0, 88);

    return static_cast<std::int16_t>(predictor);
}

} // namespace

std::vector<std::uint8_t> ima_adpcm_encode_nibbles(const std::vector<std::int16_t>& samples,
                                                   ImaAdpcmState& state) {
    std::vector<std::uint8_t> out;
    out.reserve((samples.size() + 1) / 2);
    std::uint8_t pending_low_nibble = 0;
    bool have_pending = false;
    for (const std::int16_t sample : samples) {
        const std::uint8_t nibble = encode_sample(sample, state);
        if (!have_pending) {
            pending_low_nibble = nibble;
            have_pending = true;
        } else {
            out.push_back(static_cast<std::uint8_t>(pending_low_nibble | (nibble << 4)));
            have_pending = false;
        }
    }
    if (have_pending) {
        out.push_back(pending_low_nibble); // odd sample count: high nibble left as 0
    }
    return out;
}

std::vector<std::int16_t> ima_adpcm_decode_nibbles(const std::vector<std::uint8_t>& nibble_bytes,
                                                   std::size_t sample_count,
                                                   ImaAdpcmState& state) {
    std::vector<std::int16_t> out;
    out.reserve(sample_count);
    for (std::size_t i = 0; i < nibble_bytes.size() && out.size() < sample_count; ++i) {
        const std::uint8_t byte = nibble_bytes[i];
        out.push_back(decode_nibble(static_cast<std::uint8_t>(byte & 0x0F), state));
        if (out.size() >= sample_count) break;
        out.push_back(decode_nibble(static_cast<std::uint8_t>((byte >> 4) & 0x0F), state));
    }
    return out;
}

} // namespace container
} // namespace audio
} // namespace trekker
