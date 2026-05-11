#include "audio_processing.h"
#include <cmath>
#include <algorithm>

namespace trekker {
namespace audio {

// ── IIR filters ───────────────────────────────────────────────────────────────

static AudioBuffer iir_filter(const AudioBuffer& src, float cutoff_hz, bool high_pass) {
    const float wc   = 2.f * 3.14159265f * cutoff_hz / static_cast<float>(src.sample_rate);
    const float alpha = std::tan(wc / 2.f);
    const float a0    = 1.f + alpha;
    const float b0    = high_pass ? 1.f / a0 : alpha / a0;
    const float b1    = high_pass ? -1.f / a0 : alpha / a0;
    const float a1    = (alpha - 1.f) / a0;

    AudioBuffer out = src.clone();
    for (std::size_t c = 0; c < src.num_channels; ++c) {
        float x_prev = 0.f, y_prev = 0.f;
        for (std::size_t i = 0; i < src.num_samples(); ++i) {
            const float x = src.planes[c][i];
            const float y = b0 * x + b1 * x_prev - a1 * y_prev;
            out.planes[c][i] = y;
            x_prev = x; y_prev = y;
        }
    }
    return out;
}

AudioBuffer low_pass_filter(const AudioBuffer& src, float cutoff_hz) {
    return iir_filter(src, cutoff_hz, false);
}

AudioBuffer high_pass_filter(const AudioBuffer& src, float cutoff_hz) {
    return iir_filter(src, cutoff_hz, true);
}

// ── Delay / echo ──────────────────────────────────────────────────────────────

AudioBuffer delay_effect(const AudioBuffer& src, std::size_t delay_samples, float feedback) {
    feedback = std::max(0.f, std::min(0.99f, feedback));
    const std::size_t n = src.num_samples();
    AudioBuffer out = src.clone();

    for (std::size_t c = 0; c < src.num_channels; ++c) {
        std::vector<float> buf(delay_samples, 0.f);
        std::size_t head = 0;
        for (std::size_t i = 0; i < n; ++i) {
            const float delayed = buf[head];
            const float combined = src.planes[c][i] + feedback * delayed;
            out.planes[c][i] = std::max(-1.f, std::min(1.f, combined));
            buf[head] = combined;
            head = (head + 1) % delay_samples;
        }
    }
    return out;
}

// ── Compressor ────────────────────────────────────────────────────────────────

AudioBuffer compress(const AudioBuffer& src, float threshold, float ratio,
                     float attack_ms, float release_ms, float makeup_gain) {
    const float sr  = static_cast<float>(src.sample_rate);
    const float atk = std::exp(-1.f / (sr * attack_ms  / 1000.f));
    const float rel = std::exp(-1.f / (sr * release_ms / 1000.f));
    AudioBuffer out = src.clone();

    for (std::size_t c = 0; c < src.num_channels; ++c) {
        float env = 0.f;
        for (std::size_t i = 0; i < src.num_samples(); ++i) {
            const float samp = std::abs(src.planes[c][i]);
            env = samp > env ? atk * env + (1.f - atk) * samp
                             : rel * env + (1.f - rel) * samp;
            float gain = 1.f;
            if (env > threshold)
                gain = threshold + (env - threshold) / ratio;
            out.planes[c][i] = std::max(-1.f, std::min(1.f,
                src.planes[c][i] * (gain / std::max(env, 1e-9f)) * makeup_gain));
        }
    }
    return out;
}

} // namespace audio
} // namespace trekker
