#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace utils {
namespace wave_generator {

enum class WavePattern {
    Sine,
    Square,
    Triangle,
    Sawtooth,
    ReverseSawtooth,
    Pulse,
    WhiteNoise
};

struct WaveConfig {
    double amplitude = 1.0;
    double frequency_hz = 1.0;
    double phase_radians = 0.0;
    double offset = 0.0;
    double duty_cycle = 0.5;
    std::uint32_t noise_seed = 0;
};

double sample_at(double time_seconds, WavePattern pattern, const WaveConfig& config = {});
std::vector<double> generate_samples(std::size_t sample_count,
                                     double sample_rate_hz,
                                     WavePattern pattern,
                                     const WaveConfig& config = {});
std::vector<double> generate_samples_for_duration(double duration_seconds,
                                                  double sample_rate_hz,
                                                  WavePattern pattern,
                                                  const WaveConfig& config = {});

} // namespace wave_generator
} // namespace utils
