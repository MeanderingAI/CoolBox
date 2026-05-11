#ifndef TREKKER_VIDEO_ASSETS_AUDIO_PROCESSING_H
#define TREKKER_VIDEO_ASSETS_AUDIO_PROCESSING_H

#include <cstddef>
#include <cstdint>
#include <vector>
#include <stdexcept>
#include <string>

namespace trekker {
namespace audio {

// ── Sample formats ────────────────────────────────────────────────────────────

enum class SampleFormat {
    S16,    // 16-bit signed PCM (little-endian)
    S32,    // 32-bit signed PCM
    F32,    // 32-bit IEEE float, normalised to [-1.0, 1.0]
    F64,    // 64-bit IEEE float
};

int bytes_per_sample(SampleFormat fmt);
const char* sample_format_name(SampleFormat fmt);

// ── AudioBuffer ───────────────────────────────────────────────────────────────
// Planar (non-interleaved) multi-channel audio buffer.
// samples[channel][sample_index]

struct AudioBuffer {
    SampleFormat              format       = SampleFormat::F32;
    std::uint32_t             sample_rate  = 48000;
    std::uint32_t             num_channels = 2;
    std::int64_t              pts          = 0;   // presentation timestamp (μs)

    // Planar storage — each inner vector holds one channel.
    // For F32/F64 all values are in [-1.0, 1.0] / [-1.0, 1.0].
    // For S16/S32 each element is the raw integer sample value stored as float.
    std::vector<std::vector<float>> planes; // [channel][sample]

    std::size_t num_samples() const {
        return planes.empty() ? 0 : planes[0].size();
    }

    std::size_t duration_us() const {
        if (sample_rate == 0) return 0;
        return (num_samples() * 1'000'000ULL) / sample_rate;
    }

    // Construct a silent buffer.
    static AudioBuffer silent(std::uint32_t sample_rate, std::uint32_t channels,
                              std::size_t num_samples,
                              SampleFormat fmt = SampleFormat::F32);

    // Deep copy
    AudioBuffer clone() const;
};

// ── Resampler ─────────────────────────────────────────────────────────────────
// Linear interpolation resampler.  For production use a polyphase FIR filter
// (e.g. libsamplerate) would replace this; the interface is identical.

class Resampler {
public:
    Resampler(std::uint32_t src_rate, std::uint32_t dst_rate,
              std::uint32_t num_channels);

    // Feed a buffer; returns the resampled output (may be empty if not enough input).
    AudioBuffer process(const AudioBuffer& input);

    void reset();

private:
    std::uint32_t src_rate_, dst_rate_, channels_;
    std::vector<float> prev_samples_; // last sample per channel for interpolation
    double phase_ = 0.0;              // fractional sample position
};

// ── Channel router ────────────────────────────────────────────────────────────
// Remaps an N-channel buffer to an M-channel buffer via a gain matrix.

struct ChannelRouterMatrix {
    std::uint32_t              src_channels = 0;
    std::uint32_t              dst_channels = 0;
    std::vector<std::vector<float>> gains;  // gains[dst][src]

    static ChannelRouterMatrix stereo_to_mono();
    static ChannelRouterMatrix mono_to_stereo();
    static ChannelRouterMatrix identity(std::uint32_t channels);
};

AudioBuffer route_channels(const AudioBuffer& src, const ChannelRouterMatrix& matrix);

// ── Mixer ─────────────────────────────────────────────────────────────────────
// Mixes multiple AudioBuffers together (must share sample_rate and channels).
// Clips at ±1.0 for F32/F64 formats.

AudioBuffer mix(const std::vector<AudioBuffer>& inputs, const std::vector<float>& gains = {});

// ── Amplitude / normalisation ─────────────────────────────────────────────────

// Peak normalise: scale so the loudest sample reaches target_peak (0..1).
AudioBuffer peak_normalize(const AudioBuffer& src, float target_peak = 1.0f);

// RMS normalise: scale so the RMS level matches target_rms (0..1).
AudioBuffer rms_normalize(const AudioBuffer& src, float target_rms = 0.1f);

// Apply a per-sample gain envelope.  gains.size() must equal src.num_samples().
AudioBuffer apply_gain_envelope(const AudioBuffer& src, const std::vector<float>& gains);

// ── Fade in / out ──────────────────────────────────────────────────────────────

// Fade in: linear ramp from 0→1 over fade_samples at the start.
AudioBuffer fade_in(const AudioBuffer& src, std::size_t fade_samples);

// Fade out: linear ramp from 1→0 over fade_samples at the end.
AudioBuffer fade_out(const AudioBuffer& src, std::size_t fade_samples);

// ── Audio effects ─────────────────────────────────────────────────────────────

// First-order IIR low-pass filter.  cutoff_hz must be < sample_rate / 2.
AudioBuffer low_pass_filter(const AudioBuffer& src, float cutoff_hz);

// First-order IIR high-pass filter.
AudioBuffer high_pass_filter(const AudioBuffer& src, float cutoff_hz);

// Mono delay / echo: delay_samples offset, feedback in [0,1).
AudioBuffer delay_effect(const AudioBuffer& src, std::size_t delay_samples, float feedback);

// Simple compressor: reduces peaks above threshold by ratio.
// threshold and makeup_gain are in linear amplitude (0..1).
AudioBuffer compress(const AudioBuffer& src, float threshold, float ratio,
                     float attack_ms, float release_ms, float makeup_gain = 1.0f);

} // namespace audio
} // namespace trekker

#endif // TREKKER_VIDEO_ASSETS_AUDIO_PROCESSING_H
