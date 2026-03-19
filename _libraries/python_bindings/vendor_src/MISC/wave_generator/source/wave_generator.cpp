#include "wave_generator.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <stdexcept>

namespace utils {
namespace wave_generator {
namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kTwoPi = 2.0 * kPi;

void validate_config(const WaveConfig& config) {
    if (config.frequency_hz < 0.0) {
        throw std::invalid_argument("frequency_hz must be non-negative");
    }

    if (config.duty_cycle <= 0.0 || config.duty_cycle >= 1.0) {
        throw std::invalid_argument("duty_cycle must be between 0 and 1");
    }
}

double normalized_phase(double time_seconds, const WaveConfig& config) {
    const double cycles = (time_seconds * config.frequency_hz) + (config.phase_radians / kTwoPi);
    const double wrapped = cycles - std::floor(cycles);
    return wrapped < 0.0 ? wrapped + 1.0 : wrapped;
}

double deterministic_white_noise(double time_seconds, const WaveConfig& config) {
    const auto tick = static_cast<std::uint64_t>(std::llround(time_seconds * 1000000.0));
    std::mt19937 generator(static_cast<std::mt19937::result_type>(config.noise_seed ^ static_cast<std::uint32_t>(tick)));
    std::uniform_real_distribution<double> distribution(-config.amplitude, config.amplitude);
    return distribution(generator) + config.offset;
}

double base_wave(double phase, WavePattern pattern, const WaveConfig& config) {
    switch (pattern) {
    case WavePattern::Sine:
        return config.offset + (config.amplitude * std::sin(kTwoPi * phase));
    case WavePattern::Square:
        return config.offset + (phase < 0.5 ? config.amplitude : -config.amplitude);
    case WavePattern::Triangle:
        return config.offset + (config.amplitude * (1.0 - 4.0 * std::abs(phase - 0.5)));
    case WavePattern::Sawtooth:
        return config.offset + (config.amplitude * ((2.0 * phase) - 1.0));
    case WavePattern::ReverseSawtooth:
        return config.offset + (config.amplitude * (1.0 - (2.0 * phase)));
    case WavePattern::Pulse:
        return config.offset + (phase < config.duty_cycle ? config.amplitude : -config.amplitude);
    case WavePattern::WhiteNoise:
        break;
    }

    return config.offset;
}

} // namespace

double sample_at(double time_seconds, WavePattern pattern, const WaveConfig& config) {
    validate_config(config);

    if (!std::isfinite(time_seconds)) {
        throw std::invalid_argument("time_seconds must be finite");
    }

    if (pattern == WavePattern::WhiteNoise) {
        return deterministic_white_noise(time_seconds, config);
    }

    return base_wave(normalized_phase(time_seconds, config), pattern, config);
}

std::vector<double> generate_samples_for_duration(double duration_seconds,
                                                  double sample_rate_hz,
                                                  WavePattern pattern,
                                                  const WaveConfig& config) {
    if (duration_seconds < 0.0) {
        throw std::invalid_argument("duration_seconds must be non-negative");
    }

    if (sample_rate_hz <= 0.0 || !std::isfinite(sample_rate_hz)) {
        throw std::invalid_argument("sample_rate_hz must be positive and finite");
    }

    const auto sample_count = static_cast<std::size_t>(std::llround(duration_seconds * sample_rate_hz));
    return generate_samples(sample_count, sample_rate_hz, pattern, config);
}

std::vector<double> generate_samples(std::size_t sample_count,
                                     double sample_rate_hz,
                                     WavePattern pattern,
                                     const WaveConfig& config) {
    if (sample_rate_hz <= 0.0 || !std::isfinite(sample_rate_hz)) {
        throw std::invalid_argument("sample_rate_hz must be positive and finite");
    }

    validate_config(config);

    std::vector<double> samples;
    samples.reserve(sample_count);

    for (std::size_t i = 0; i < sample_count; ++i) {
        const double time_seconds = static_cast<double>(i) / sample_rate_hz;
        samples.push_back(sample_at(time_seconds, pattern, config));
    }

    return samples;
}

} // namespace wave_generator
} // namespace utils
