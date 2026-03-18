#include "wave_generator.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>

namespace {

using utils::wave_generator::WaveConfig;
using utils::wave_generator::WavePattern;

TEST(WaveGeneratorTest, GeneratesExpectedSineSamples) {
    const WaveConfig config{1.0, 1.0, 0.0, 0.0, 0.5, 0};
    const auto samples = utils::wave_generator::generate_samples(4U, 4.0, WavePattern::Sine, config);

    ASSERT_EQ(samples.size(), 4U);
    EXPECT_NEAR(samples[0], 0.0, 1e-9);
    EXPECT_NEAR(samples[1], 1.0, 1e-9);
    EXPECT_NEAR(samples[2], 0.0, 1e-9);
    EXPECT_NEAR(samples[3], -1.0, 1e-9);
}

TEST(WaveGeneratorTest, PulseWaveRespectsDutyCycle) {
    const WaveConfig config{2.0, 1.0, 0.0, 0.0, 0.25, 0};

    EXPECT_DOUBLE_EQ(utils::wave_generator::sample_at(0.10, WavePattern::Pulse, config), 2.0);
    EXPECT_DOUBLE_EQ(utils::wave_generator::sample_at(0.30, WavePattern::Pulse, config), -2.0);
}

TEST(WaveGeneratorTest, SawtoothStaysWithinAmplitudeRange) {
    const WaveConfig config{3.0, 2.0, 0.0, 1.0, 0.5, 0};
    const auto samples = utils::wave_generator::generate_samples(64U, 64.0, WavePattern::Sawtooth, config);

    ASSERT_FALSE(samples.empty());
    const auto [min_it, max_it] = std::minmax_element(samples.begin(), samples.end());
    EXPECT_GE(*min_it, -2.0 - 1e-9);
    EXPECT_LE(*max_it, 4.0 + 1e-9);
}

TEST(WaveGeneratorTest, WhiteNoiseIsDeterministicForSameSeed) {
    const WaveConfig config{0.5, 0.0, 0.0, 0.0, 0.5, 42};
    const auto first = utils::wave_generator::generate_samples(8U, 8.0, WavePattern::WhiteNoise, config);
    const auto second = utils::wave_generator::generate_samples(8U, 8.0, WavePattern::WhiteNoise, config);

    EXPECT_EQ(first, second);
}

TEST(WaveGeneratorTest, RejectsInvalidDutyCycle) {
    WaveConfig config{};
    config.duty_cycle = 1.0;

    EXPECT_THROW(utils::wave_generator::sample_at(0.0, WavePattern::Pulse, config), std::invalid_argument);
}

} // namespace
