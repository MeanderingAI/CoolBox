// Emscripten bindings for utils::wave_generator (wave_generator.hpp)
// Mirrors the C++ API from:
//   _deliverables/libraries/groups/trekker/MISC/wave_generator/headers/wave_generator.hpp
//
// Build (from repo root, with emsdk activated):
//   cmake --build build --config Release --target wave_generator_js
//
// Usage in browser (MODULARIZE=1):
//   const mod = await createWaveGeneratorModule();
//   const samples = mod.generate_samples_for_duration(1.0, 44100, mod.WavePattern.Sine,
//       { amplitude: 0.8, frequency_hz: 440, phase_radians: 0, offset: 0,
//         duty_cycle: 0.5, noise_seed: 0 });

#include <emscripten/bind.h>
#include "wave_generator.hpp"
#include <vector>
#include <cmath>

using namespace emscripten;
using namespace utils::wave_generator;

// ── WavePattern constants ─────────────────────────────────────────────────────

static int wp_Sine()            { return static_cast<int>(WavePattern::Sine);            }
static int wp_Square()          { return static_cast<int>(WavePattern::Square);          }
static int wp_Triangle()        { return static_cast<int>(WavePattern::Triangle);        }
static int wp_Sawtooth()        { return static_cast<int>(WavePattern::Sawtooth);        }
static int wp_ReverseSawtooth() { return static_cast<int>(WavePattern::ReverseSawtooth); }
static int wp_Pulse()           { return static_cast<int>(WavePattern::Pulse);           }
static int wp_WhiteNoise()      { return static_cast<int>(WavePattern::WhiteNoise);      }

// ── WaveConfig struct binding ─────────────────────────────────────────────────
// Emscripten value_object mirrors C++ plain struct.

// ── Free function wrappers ────────────────────────────────────────────────────

static double js_sample_at(double time_seconds, int pattern_int,
                             const WaveConfig& config) {
    return sample_at(time_seconds, static_cast<WavePattern>(pattern_int), config);
}

static std::vector<double> js_generate_samples(size_t sample_count,
                                                double sample_rate_hz,
                                                int pattern_int,
                                                const WaveConfig& config) {
    return generate_samples(sample_count, sample_rate_hz,
                            static_cast<WavePattern>(pattern_int), config);
}

static std::vector<double> js_generate_samples_for_duration(double duration_seconds,
                                                              double sample_rate_hz,
                                                              int pattern_int,
                                                              const WaveConfig& config) {
    return generate_samples_for_duration(duration_seconds, sample_rate_hz,
                                         static_cast<WavePattern>(pattern_int), config);
}

// FFT computation function
std::vector<std::complex<double>> compute_fft(const std::vector<double>& input) {
    size_t N = input.size();
    std::vector<std::complex<double>> output(N);

    for (size_t k = 0; k < N; ++k) {
        std::complex<double> sum(0.0, 0.0);
        for (size_t n = 0; n < N; ++n) {
            double angle = -2.0 * M_PI * k * n / N;
            sum += std::polar(input[n], angle);
        }
        output[k] = sum;
    }

    return output;
}

// Wrapper for JavaScript
static std::vector<std::complex<double>> js_compute_fft(const std::vector<double>& input) {
    return compute_fft(input);
}

// ── Bindings ──────────────────────────────────────────────────────────────────

EMSCRIPTEN_BINDINGS(wave_generator_module) {
    register_vector<double>("VectorDouble_Wave");

    // WavePattern constants
    function("WavePattern_Sine",            &wp_Sine);
    function("WavePattern_Square",          &wp_Square);
    function("WavePattern_Triangle",        &wp_Triangle);
    function("WavePattern_Sawtooth",        &wp_Sawtooth);
    function("WavePattern_ReverseSawtooth", &wp_ReverseSawtooth);
    function("WavePattern_Pulse",           &wp_Pulse);
    function("WavePattern_WhiteNoise",      &wp_WhiteNoise);

    // WaveConfig struct
    value_object<WaveConfig>("WaveConfig")
        .field("amplitude",      &WaveConfig::amplitude)
        .field("frequency_hz",   &WaveConfig::frequency_hz)
        .field("phase_radians",  &WaveConfig::phase_radians)
        .field("offset",         &WaveConfig::offset)
        .field("duty_cycle",     &WaveConfig::duty_cycle)
        .field("noise_seed",     &WaveConfig::noise_seed)
    ;

    // Free functions
    function("sample_at",                     &js_sample_at);
    function("generate_samples",              &js_generate_samples);
    function("generate_samples_for_duration", &js_generate_samples_for_duration);
    function("compute_fft",                   &js_compute_fft);
}
