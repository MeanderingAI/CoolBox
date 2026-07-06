#include "wavelets.hpp"

#include "tyst_framework.hpp"

#include <cmath>

namespace {

constexpr double kPi = 3.14159265358979323846;

TYST_TEST(WaveletsTest, HaarFilterBankNormalised) {
    const auto lo = sp::wavelets::wavelet_filter_lo(sp::wavelets::WaveletFamily::Haar);
    const auto hi = sp::wavelets::wavelet_filter_hi(sp::wavelets::WaveletFamily::Haar);
    TYST_ASSERT_EQ(lo.size(), 2U);
    TYST_ASSERT_EQ(hi.size(), 2U);
    // Energy of each filter should be 1
    double energy_lo = 0.0;
    double energy_hi = 0.0;
    for (std::size_t i = 0; i < lo.size(); ++i) {
        energy_lo += lo[i] * lo[i];
        energy_hi += hi[i] * hi[i];
    }
    TYST_EXPECT_NEAR(energy_lo, 1.0, 1e-9);
    TYST_EXPECT_NEAR(energy_hi, 1.0, 1e-9);
}

TYST_TEST(WaveletsTest, DwtProducesApproxAndDetailCoeffs) {
    const std::vector<double> signal = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};
    const auto result = sp::wavelets::dwt(signal, sp::wavelets::WaveletFamily::Haar);
    TYST_EXPECT_GT(result.approx.size(), 0U);
    TYST_EXPECT_GT(result.detail.size(), 0U);
    TYST_EXPECT_EQ(result.approx.size(), result.detail.size());
}

TYST_TEST(WaveletsTest, IdwtReconstructsHaarRoundTrip) {
    const std::vector<double> signal = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};
    const auto coeffs = sp::wavelets::dwt(signal, sp::wavelets::WaveletFamily::Haar);
    const auto reconstructed = sp::wavelets::idwt(coeffs, sp::wavelets::WaveletFamily::Haar);
    TYST_ASSERT_GE(reconstructed.size(), signal.size());
    for (std::size_t i = 0; i < signal.size(); ++i) {
        TYST_EXPECT_NEAR(reconstructed[i], signal[i], 1e-9);
    }
}

TYST_TEST(WaveletsTest, IdwtReconstructsDb2RoundTrip) {
    const std::vector<double> signal = {0.5, 1.0, 0.5, -0.5, -1.0, -0.5, 0.25, -0.25};
    const auto coeffs = sp::wavelets::dwt(signal, sp::wavelets::WaveletFamily::Daubechies2);
    const auto reconstructed = sp::wavelets::idwt(coeffs, sp::wavelets::WaveletFamily::Daubechies2);
    TYST_ASSERT_GE(reconstructed.size(), signal.size());
    for (std::size_t i = 0; i < signal.size(); ++i) {
        TYST_EXPECT_NEAR(reconstructed[i], signal[i], 1e-8);
    }
}

TYST_TEST(WaveletsTest, WavedecProducesCorrectNumberOfLevels) {
    const std::vector<double> signal(64U, 1.0);
    const std::size_t levels = 3U;
    const auto result = sp::wavelets::wavedec(signal, sp::wavelets::WaveletFamily::Haar, levels);
    TYST_ASSERT_EQ(result.size(), levels + 1U);
}

TYST_TEST(WaveletsTest, WavedecWaverec_HaarRoundTrip) {
    const std::vector<double> signal = {3.0, 1.0, -2.0, 4.0, 0.5, -0.5, 2.0, 1.5};
    const auto coeffs = sp::wavelets::wavedec(signal, sp::wavelets::WaveletFamily::Haar, 3U);
    const auto reconstructed = sp::wavelets::waverec(coeffs, sp::wavelets::WaveletFamily::Haar);
    TYST_ASSERT_GE(reconstructed.size(), signal.size());
    for (std::size_t i = 0; i < signal.size(); ++i) {
        TYST_EXPECT_NEAR(reconstructed[i], signal[i], 1e-9);
    }
}

TYST_TEST(WaveletsTest, CwtScalesIsGeometric) {
    const auto scales = sp::wavelets::cwt_scales(1.0, 64.0, 7U);
    TYST_ASSERT_EQ(scales.size(), 7U);
    TYST_EXPECT_NEAR(scales.front(), 1.0, 1e-9);
    TYST_EXPECT_NEAR(scales.back(), 64.0, 1e-9);
    // Ratio between adjacent scales should be constant
    const double ratio = scales[1] / scales[0];
    for (std::size_t i = 2; i < scales.size(); ++i) {
        TYST_EXPECT_NEAR(scales[i] / scales[i - 1U], ratio, 1e-6);
    }
}

TYST_TEST(WaveletsTest, CwtMorletOutputDimensions) {
    const std::vector<double> signal(32U, 0.0);
    const auto scales = sp::wavelets::cwt_scales(1.0, 8.0, 4U);
    const auto output = sp::wavelets::cwt_morlet(signal, scales);
    TYST_ASSERT_EQ(output.size(), 4U);
    for (const auto& row : output) {
        TYST_EXPECT_EQ(row.size(), 32U);
    }
}

TYST_TEST(WaveletsTest, ScalogramHasNonNegativeValues) {
    const std::vector<double> signal(32U, 1.0);
    const auto scales = sp::wavelets::cwt_scales(1.0, 4.0, 3U);
    const auto cwt_out = sp::wavelets::cwt_morlet(signal, scales);
    const auto mag = sp::wavelets::scalogram(cwt_out);
    for (const auto& row : mag) {
        for (double v : row) {
            TYST_EXPECT_GE(v, 0.0);
        }
    }
}

TYST_TEST(WaveletsTest, UniversalThresholdIsPositive) {
    const std::vector<double> coeffs = {0.1, -0.5, 0.3, 0.02, -0.01, 0.8, -0.4, 0.15};
    const double lambda = sp::wavelets::universal_threshold(coeffs);
    TYST_EXPECT_GT(lambda, 0.0);
}

TYST_TEST(WaveletsTest, SoftThresholdZerosBelowLambda) {
    const std::vector<double> coeffs = {-0.1, 0.5, -0.3, 0.9};
    const auto result = sp::wavelets::threshold(coeffs, 0.4, sp::wavelets::ThresholdRule::Soft);
    TYST_EXPECT_NEAR(result[0], 0.0, 1e-12);   // |−0.1| < 0.4
    TYST_EXPECT_NEAR(result[1], 0.1, 1e-12);   // 0.5 − 0.4 = 0.1
    TYST_EXPECT_NEAR(result[2], 0.0, 1e-12);   // |−0.3| < 0.4
    TYST_EXPECT_NEAR(result[3], 0.5, 1e-12);   // 0.9 − 0.4 = 0.5
}

TYST_TEST(WaveletsTest, HardThresholdPreservesAboveLambda) {
    const std::vector<double> coeffs = {-0.1, 0.5, -0.3, 0.9};
    const auto result = sp::wavelets::threshold(coeffs, 0.4, sp::wavelets::ThresholdRule::Hard);
    TYST_EXPECT_NEAR(result[0], 0.0, 1e-12);
    TYST_EXPECT_NEAR(result[1], 0.5, 1e-12);
    TYST_EXPECT_NEAR(result[2], 0.0, 1e-12);
    TYST_EXPECT_NEAR(result[3], 0.9, 1e-12);
}

TYST_TEST(WaveletsTest, DenoisePreservesLength) {
    std::vector<double> signal(64U, 0.0);
    for (std::size_t i = 0; i < 64U; ++i) {
        signal[i] = std::sin(2.0 * kPi * static_cast<double>(i) / 16.0);
    }
    const auto result = sp::wavelets::denoise(signal, sp::wavelets::WaveletFamily::Haar, 3U);
    TYST_ASSERT_EQ(result.size(), signal.size());
}

TYST_TEST(WaveletsTest, PadToPowerOfTwoCorrectLength) {
    const std::vector<double> signal(10U, 1.0);
    const auto padded = sp::wavelets::pad_to_power_of_two(signal);
    TYST_ASSERT_EQ(padded.size(), 16U);
    for (std::size_t i = 10U; i < 16U; ++i) {
        TYST_EXPECT_NEAR(padded[i], 0.0, 1e-12);
    }
}

TYST_TEST(WaveletsTest, WavedecZeroLevelsThrows) {
    const std::vector<double> signal(16U, 1.0);
    TYST_EXPECT_THROW(
        sp::wavelets::wavedec(signal, sp::wavelets::WaveletFamily::Haar, 0U),
        std::invalid_argument);
}

TYST_TEST(WaveletsTest, CwtScalesInvalidRangeThrows) {
    TYST_EXPECT_THROW(sp::wavelets::cwt_scales(0.0, 8.0, 4U), std::invalid_argument);
    TYST_EXPECT_THROW(sp::wavelets::cwt_scales(8.0, 1.0, 4U), std::invalid_argument);
    TYST_EXPECT_THROW(sp::wavelets::cwt_scales(1.0, 8.0, 1U), std::invalid_argument);
}

}  // namespace
