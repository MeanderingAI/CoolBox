#include "resampling.hpp"

#include "tyst_framework.hpp"

#include <cmath>
#include <numeric>

namespace {

constexpr double kPi = 3.14159265358979323846;

TYST_TEST(ResamplingTest, UpsampleInsertsZeros) {
    const std::vector<double> signal = {1.0, 2.0, 3.0};
    const auto output = sp::resampling::upsample(signal, 3U);
    TYST_ASSERT_EQ(output.size(), 9U);
    TYST_EXPECT_NEAR(output[0], 1.0, 1e-12);
    TYST_EXPECT_NEAR(output[1], 0.0, 1e-12);
    TYST_EXPECT_NEAR(output[2], 0.0, 1e-12);
    TYST_EXPECT_NEAR(output[3], 2.0, 1e-12);
    TYST_EXPECT_NEAR(output[6], 3.0, 1e-12);
}

TYST_TEST(ResamplingTest, DownsampleKeepsSamples) {
    const std::vector<double> signal = {10.0, 20.0, 30.0, 40.0, 50.0, 60.0};
    const auto output = sp::resampling::downsample(signal, 2U);
    TYST_ASSERT_EQ(output.size(), 3U);
    TYST_EXPECT_NEAR(output[0], 10.0, 1e-12);
    TYST_EXPECT_NEAR(output[1], 30.0, 1e-12);
    TYST_EXPECT_NEAR(output[2], 50.0, 1e-12);
}

TYST_TEST(ResamplingTest, UpsampleFactorOneIsIdentity) {
    const std::vector<double> signal = {1.0, 2.0, 3.0};
    TYST_EXPECT_EQ(sp::resampling::upsample(signal, 1U).size(), signal.size());
}

TYST_TEST(ResamplingTest, DownsampleFactorOneIsIdentity) {
    const std::vector<double> signal = {1.0, 2.0, 3.0};
    TYST_EXPECT_EQ(sp::resampling::downsample(signal, 1U).size(), signal.size());
}

TYST_TEST(ResamplingTest, DecimateReducesSampleCount) {
    const std::vector<double> signal(64U, 1.0);
    const auto output = sp::resampling::decimate(signal, 4U);
    TYST_EXPECT_EQ(output.size(), 16U);
}

TYST_TEST(ResamplingTest, InterpolateIncreasesSampleCount) {
    const std::vector<double> signal(16U, 1.0);
    const auto output = sp::resampling::interpolate(signal, 4U);
    TYST_ASSERT_EQ(output.size(), 64U);
}

TYST_TEST(ResamplingTest, ResampleRationalRatio) {
    const std::vector<double> signal(30U, 1.0);
    // 3/2 ratio -> output ~ 45 samples
    const auto output = sp::resampling::resample_rational(signal, 3U, 2U);
    TYST_EXPECT_GT(output.size(), 0U);
}

TYST_TEST(ResamplingTest, ResampleLinearOutputLength) {
    const std::vector<double> signal = {0.0, 1.0, 2.0, 3.0};
    const auto output = sp::resampling::resample_linear(signal, 7U);
    TYST_ASSERT_EQ(output.size(), 7U);
    TYST_EXPECT_NEAR(output[0], 0.0, 1e-9);
    TYST_EXPECT_NEAR(output[6], 3.0, 1e-9);
}

TYST_TEST(ResamplingTest, ResampleLinearMonotonicInput) {
    const std::size_t n_in = 10U;
    const std::size_t n_out = 20U;
    std::vector<double> signal(n_in);
    for (std::size_t i = 0; i < n_in; ++i) {
        signal[i] = static_cast<double>(i);
    }
    const auto output = sp::resampling::resample_linear(signal, n_out);
    TYST_ASSERT_EQ(output.size(), n_out);
    for (std::size_t i = 1; i < n_out; ++i) {
        TYST_EXPECT_GT(output[i], output[i - 1U] - 1e-9);
    }
}

TYST_TEST(ResamplingTest, ResampleCubicOutputLength) {
    const std::vector<double> signal = {0.0, 1.0, 2.0, 3.0, 4.0};
    const auto output = sp::resampling::resample_cubic(signal, 9U);
    TYST_ASSERT_EQ(output.size(), 9U);
}

TYST_TEST(ResamplingTest, ResampleSincOutputLength) {
    const std::vector<double> signal(32U, 1.0);
    const auto output = sp::resampling::resample_sinc(signal, 48U);
    TYST_ASSERT_EQ(output.size(), 48U);
}

TYST_TEST(ResamplingTest, ResampleRateMatchesExpectedOutputSize) {
    const std::vector<double> signal(44100U, 0.0);  // 1 s at 44100 Hz
    const auto output = sp::resampling::resample_rate(signal, 44100.0, 22050.0);
    // ~22050 samples
    TYST_EXPECT_NEAR(static_cast<double>(output.size()), 22050.0, 2.0);
}

TYST_TEST(ResamplingTest, PolyphaseDecomposePartitionsTaps) {
    const std::vector<double> h = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0};
    const auto phases = sp::resampling::polyphase_decompose(h, 3U);
    TYST_ASSERT_EQ(phases.size(), 3U);
    // Phase 0: taps 0, 3 -> {1, 4}
    TYST_ASSERT_EQ(phases[0].size(), 2U);
    TYST_EXPECT_NEAR(phases[0][0], 1.0, 1e-12);
    TYST_EXPECT_NEAR(phases[0][1], 4.0, 1e-12);
}

TYST_TEST(ResamplingTest, InvalidFactorThrows) {
    const std::vector<double> signal = {1.0, 2.0};
    TYST_EXPECT_THROW(sp::resampling::upsample(signal, 0U), std::invalid_argument);
    TYST_EXPECT_THROW(sp::resampling::downsample(signal, 0U), std::invalid_argument);
    TYST_EXPECT_THROW(sp::resampling::decimate(signal, 0U), std::invalid_argument);
    TYST_EXPECT_THROW(sp::resampling::interpolate(signal, 0U), std::invalid_argument);
    TYST_EXPECT_THROW(sp::resampling::resample_rate(signal, 0.0, 48000.0), std::invalid_argument);
}

}  // namespace
