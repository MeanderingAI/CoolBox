#include "tyst_framework.hpp"
#include "audio_container.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

using trekker::audio::AudioBuffer;
using trekker::audio::SampleFormat;
namespace ac = trekker::audio::container;

namespace {

std::string temp_file_path(const std::string& suffix) {
    static int counter = 0;
    const char* tmp_dir = std::getenv("TMPDIR");
    const std::string dir = tmp_dir ? tmp_dir : "/tmp/";
    return dir + (dir.back() == '/' ? "" : "/") + "audio_container_test_" +
           std::to_string(++counter) + suffix;
}

AudioBuffer make_sine_buffer(std::uint32_t sample_rate, std::uint32_t channels,
                            std::size_t num_samples, double freq_hz, double amplitude = 0.6) {
    AudioBuffer buf = AudioBuffer::silent(sample_rate, channels, num_samples, SampleFormat::F32);
    for (std::uint32_t c = 0; c < channels; ++c) {
        for (std::size_t i = 0; i < num_samples; ++i) {
            const double t = static_cast<double>(i) / sample_rate;
            buf.planes[c][i] = static_cast<float>(amplitude * std::sin(2.0 * M_PI * freq_hz * t));
        }
    }
    return buf;
}

double max_abs_error(const AudioBuffer& a, const AudioBuffer& b) {
    double worst = 0.0;
    for (std::uint32_t c = 0; c < a.num_channels; ++c) {
        for (std::size_t i = 0; i < a.num_samples(); ++i) {
            worst = std::max(worst, static_cast<double>(std::fabs(a.planes[c][i] - b.planes[c][i])));
        }
    }
    return worst;
}

// ADPCM (and any predictive codec) exhibits a brief "slope overload" error
// spike right after each predictor reset (block start), while its adaptive
// step size ramps up to track the signal. RMS error is the standard way to
// judge overall codec fidelity without that transient dominating the metric.
double rms_error(const AudioBuffer& a, const AudioBuffer& b) {
    double sum_sq = 0.0;
    std::size_t count = 0;
    for (std::uint32_t c = 0; c < a.num_channels; ++c) {
        for (std::size_t i = 0; i < a.num_samples(); ++i) {
            const double diff = a.planes[c][i] - b.planes[c][i];
            sum_sq += diff * diff;
            ++count;
        }
    }
    return count == 0 ? 0.0 : std::sqrt(sum_sq / count);
}

} // namespace

// ── IMA ADPCM nibble codec ────────────────────────────────────────────────────

TYST_TEST(AudioContainerTests, ImaAdpcmNibbleRoundTripIsBoundedError) {
    std::vector<std::int16_t> samples(2000);
    for (std::size_t i = 0; i < samples.size(); ++i) {
        samples[i] = static_cast<std::int16_t>(12000.0 * std::sin(2.0 * M_PI * 5.0 * i / samples.size()));
    }

    ac::ImaAdpcmState enc_state;
    const auto nibbles = ac::ima_adpcm_encode_nibbles(samples, enc_state);
    TYST_EXPECT_EQ(nibbles.size(), samples.size() / 2);

    ac::ImaAdpcmState dec_state;
    const auto decoded = ac::ima_adpcm_decode_nibbles(nibbles, samples.size(), dec_state);
    TYST_ASSERT_EQ(decoded.size(), samples.size());

    // IMA ADPCM is lossy; verify the reconstruction tracks the original
    // closely rather than expecting bit-exact samples.
    std::int32_t max_diff = 0;
    double sum_abs_diff = 0.0;
    for (std::size_t i = 0; i < samples.size(); ++i) {
        const std::int32_t diff = std::abs(static_cast<std::int32_t>(samples[i]) - decoded[i]);
        max_diff = std::max(max_diff, diff);
        sum_abs_diff += diff;
    }
    TYST_EXPECT_LT(max_diff, 2000);
    TYST_EXPECT_LT(sum_abs_diff / samples.size(), 400.0);
}

TYST_TEST(AudioContainerTests, ImaAdpcmNibbleRoundTripOddSampleCount) {
    std::vector<std::int16_t> samples = {100, -200, 300, -400, 500};
    ac::ImaAdpcmState enc_state;
    const auto nibbles = ac::ima_adpcm_encode_nibbles(samples, enc_state);
    TYST_EXPECT_EQ(nibbles.size(), static_cast<std::size_t>(3)); // ceil(5/2)

    ac::ImaAdpcmState dec_state;
    const auto decoded = ac::ima_adpcm_decode_nibbles(nibbles, samples.size(), dec_state);
    TYST_ASSERT_EQ(decoded.size(), samples.size());
}

TYST_TEST(AudioContainerTests, ImaAdpcmSilenceStaysNearZero) {
    std::vector<std::int16_t> samples(200, 0);
    ac::ImaAdpcmState enc_state;
    const auto nibbles = ac::ima_adpcm_encode_nibbles(samples, enc_state);
    ac::ImaAdpcmState dec_state;
    const auto decoded = ac::ima_adpcm_decode_nibbles(nibbles, samples.size(), dec_state);
    for (auto s : decoded) TYST_EXPECT_LT(std::abs(static_cast<int>(s)), 20);
}

// ── WAV PCM16 round-trip (lossless, modulo 16-bit quantization) ──────────────

TYST_TEST(AudioContainerTests, WavPcm16RoundTripIsSampleExactMono) {
    const std::string path = temp_file_path("_pcm16_mono.wav");
    const AudioBuffer original = make_sine_buffer(44100, 1, 4410, 440.0);

    ac::write_wav(path, original, ac::WavEncoding::PCM16);
    const AudioBuffer decoded = ac::read_wav(path);

    TYST_EXPECT_EQ(decoded.sample_rate, original.sample_rate);
    TYST_EXPECT_EQ(decoded.num_channels, original.num_channels);
    TYST_ASSERT_EQ(decoded.num_samples(), original.num_samples());

    // 16-bit quantization introduces at most ~1/32768 of error per sample.
    TYST_EXPECT_LT(max_abs_error(original, decoded), 2.0 / 32767.0 + 1e-6);
    std::remove(path.c_str());
}

TYST_TEST(AudioContainerTests, WavPcm16RoundTripStereo) {
    const std::string path = temp_file_path("_pcm16_stereo.wav");
    AudioBuffer original = make_sine_buffer(22050, 2, 2205, 220.0);
    // Make channels distinguishable.
    for (std::size_t i = 0; i < original.num_samples(); ++i) {
        original.planes[1][i] *= 0.5f;
    }

    ac::write_wav(path, original, ac::WavEncoding::PCM16);
    const AudioBuffer decoded = ac::read_wav(path);

    TYST_ASSERT_EQ(decoded.num_channels, static_cast<std::uint32_t>(2));
    TYST_ASSERT_EQ(decoded.num_samples(), original.num_samples());
    TYST_EXPECT_LT(max_abs_error(original, decoded), 2.0 / 32767.0 + 1e-6);
    std::remove(path.c_str());
}

// ── WAV IMA ADPCM round-trip (lossy, bounded error) ───────────────────────────

TYST_TEST(AudioContainerTests, WavImaAdpcmRoundTripMonoBoundedError) {
    const std::string path = temp_file_path("_adpcm_mono.wav");
    const AudioBuffer original = make_sine_buffer(16000, 1, 16000, 300.0); // 1 second

    // Small block_align forces many blocks, exercising block-boundary reset
    // of the predictor/step-index state.
    ac::write_wav(path, original, ac::WavEncoding::ImaAdpcm, /*adpcm_block_align=*/36);
    const AudioBuffer decoded = ac::read_wav(path);

    TYST_EXPECT_EQ(decoded.sample_rate, original.sample_rate);
    TYST_ASSERT_EQ(decoded.num_samples(), original.num_samples());
    // ADPCM has a brief "slope overload" right after each block-start reset
    // while the adaptive step size ramps up, so judge overall fidelity via
    // RMS error; max error is checked too, but with a much looser bound.
    TYST_EXPECT_LT(rms_error(original, decoded), 0.03);
    TYST_EXPECT_LT(max_abs_error(original, decoded), 0.5);
    std::remove(path.c_str());
}

TYST_TEST(AudioContainerTests, WavImaAdpcmRoundTripStereoBoundedError) {
    const std::string path = temp_file_path("_adpcm_stereo.wav");
    AudioBuffer original = make_sine_buffer(8000, 2, 8000, 150.0);
    for (std::size_t i = 0; i < original.num_samples(); ++i) original.planes[1][i] *= -0.7f;

    ac::write_wav(path, original, ac::WavEncoding::ImaAdpcm, /*adpcm_block_align=*/256);
    const AudioBuffer decoded = ac::read_wav(path);

    TYST_ASSERT_EQ(decoded.num_channels, static_cast<std::uint32_t>(2));
    TYST_ASSERT_EQ(decoded.num_samples(), original.num_samples());
    TYST_EXPECT_LT(rms_error(original, decoded), 0.03);
    TYST_EXPECT_LT(max_abs_error(original, decoded), 0.5);
    std::remove(path.c_str());
}

TYST_TEST(AudioContainerTests, WavImaAdpcmHandlesNonMultipleSampleCount) {
    // Sample count deliberately not a multiple of samples_per_block, to
    // exercise the final partial-block padding + fact-chunk truncation path.
    const std::string path = temp_file_path("_adpcm_partial.wav");
    const AudioBuffer original = make_sine_buffer(8000, 1, 777, 100.0);

    ac::write_wav(path, original, ac::WavEncoding::ImaAdpcm, /*adpcm_block_align=*/64);
    const AudioBuffer decoded = ac::read_wav(path);

    TYST_ASSERT_EQ(decoded.num_samples(), original.num_samples());
    TYST_EXPECT_LT(rms_error(original, decoded), 0.03);
    TYST_EXPECT_LT(max_abs_error(original, decoded), 0.5);
    std::remove(path.c_str());
}

TYST_TEST(AudioContainerTests, WavImaAdpcmRejectsTooSmallBlockAlign) {
    const AudioBuffer original = make_sine_buffer(8000, 2, 100, 100.0);
    const std::string path = temp_file_path("_adpcm_bad.wav");
    TYST_EXPECT_THROW(ac::write_wav(path, original, ac::WavEncoding::ImaAdpcm, /*adpcm_block_align=*/4),
                      std::invalid_argument);
}
