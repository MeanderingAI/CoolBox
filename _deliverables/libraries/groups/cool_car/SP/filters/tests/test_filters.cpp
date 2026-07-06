#include "filters.hpp"

#include "tyst_framework.hpp"

#include <cmath>
#include <numeric>

namespace {

constexpr double kPi = 3.14159265358979323846;

TYST_TEST(FiltersTest, FirLowpassPreservesLowFrequency) {
    // A 1 Hz cosine sampled at 100 Hz — should pass through a 0.1-normalised LPF.
    const std::size_t n = 128U;
    std::vector<double> signal(n);
    for (std::size_t i = 0; i < n; ++i) {
        signal[i] = std::cos(2.0 * kPi * 0.02 * static_cast<double>(i));  // fc=0.02
    }

    const auto coeffs = sp::filters::fir_lowpass(64U, 0.1);
    const auto output = sp::filters::apply_fir(coeffs, signal);

    // After filter settles (last quarter), output amplitude should be close to 1
    double rms_in = 0.0;
    double rms_out = 0.0;
    for (std::size_t i = 96U; i < n; ++i) {
        rms_in  += signal[i] * signal[i];
        rms_out += output[i] * output[i];
    }
    rms_in  = std::sqrt(rms_in / 32.0);
    rms_out = std::sqrt(rms_out / 32.0);
    TYST_EXPECT_NEAR(rms_out / rms_in, 1.0, 0.15);
}

TYST_TEST(FiltersTest, FirHighpassBlocksDC) {
    // DC signal (constant 1) should be suppressed by a high-pass filter.
    const std::size_t n = 128U;
    const std::vector<double> signal(n, 1.0);

    const auto coeffs = sp::filters::fir_highpass(64U, 0.1);
    const auto output = sp::filters::apply_fir(coeffs, signal);

    // RMS of settled output should be near zero
    double rms = 0.0;
    for (std::size_t i = 96U; i < n; ++i) {
        rms += output[i] * output[i];
    }
    rms = std::sqrt(rms / 32.0);
    TYST_EXPECT_LT(rms, 0.05);
}

TYST_TEST(FiltersTest, FirBandpassBlocksOutOfBandFrequencies) {
    const std::size_t n = 256U;
    // In-band tone at normalised freq 0.3 (between 0.2 and 0.4)
    std::vector<double> in_band(n);
    // Out-of-band tone at normalised freq 0.05 (below 0.2)
    std::vector<double> out_of_band(n);
    for (std::size_t i = 0; i < n; ++i) {
        const double t = static_cast<double>(i);
        in_band[i]     = std::cos(2.0 * kPi * 0.3 * t);
        out_of_band[i] = std::cos(2.0 * kPi * 0.05 * t);
    }

    const auto coeffs = sp::filters::fir_bandpass(64U, 0.2, 0.4);
    const auto out_in   = sp::filters::apply_fir(coeffs, in_band);
    const auto out_rej  = sp::filters::apply_fir(coeffs, out_of_band);

    double rms_in = 0.0;
    double rms_rej = 0.0;
    for (std::size_t i = 192U; i < n; ++i) {
        rms_in  += out_in[i]  * out_in[i];
        rms_rej += out_rej[i] * out_rej[i];
    }
    rms_in  = std::sqrt(rms_in  / 64.0);
    rms_rej = std::sqrt(rms_rej / 64.0);
    TYST_EXPECT_GT(rms_in, rms_rej * 3.0);
}

TYST_TEST(FiltersTest, ButterworthLowpassHasUnityGainAtDC) {
    const auto coeffs = sp::filters::butterworth_lowpass(2U, 0.4);
    const auto mag = sp::filters::frequency_response(coeffs, 64U);
    // DC (index 0) should be near 1
    TYST_EXPECT_NEAR(mag[0], 1.0, 0.05);
}

TYST_TEST(FiltersTest, ButterworthHighpassBlocksDC) {
    const auto coeffs = sp::filters::butterworth_highpass(2U, 0.1);
    const auto mag = sp::filters::frequency_response(coeffs, 64U);
    // DC (index 0) should be near 0
    TYST_EXPECT_LT(mag[0], 0.1);
}

TYST_TEST(FiltersTest, FiltfiltProducesEvenLengthOutput) {
    const std::vector<double> signal = {1.0, 2.0, 3.0, 2.0, 1.0, 0.0, -1.0, -2.0};
    const auto coeffs = sp::filters::butterworth_lowpass(1U, 0.3);
    const auto output = sp::filters::filtfilt(coeffs, signal);
    TYST_ASSERT_EQ(output.size(), signal.size());
}

TYST_TEST(FiltersTest, MovingAverageConstantSignalPassesThrough) {
    const std::vector<double> signal(64U, 3.5);
    const auto output = sp::filters::moving_average(signal, 8U);
    TYST_ASSERT_EQ(output.size(), signal.size());
    for (std::size_t i = 8U; i < output.size(); ++i) {
        TYST_EXPECT_NEAR(output[i], 3.5, 1e-12);
    }
}

TYST_TEST(FiltersTest, MedianFilterRemovesIsolatedSpikes) {
    std::vector<double> signal(32U, 0.0);
    signal[15] = 100.0;  // spike
    const auto output = sp::filters::median_filter(signal, 5U);
    TYST_EXPECT_NEAR(output[15], 0.0, 1e-12);
}

TYST_TEST(FiltersTest, SavitzkyGolayPreservesLinearSignal) {
    // A perfectly linear ramp should pass through unchanged.
    const std::size_t n = 32U;
    std::vector<double> ramp(n);
    for (std::size_t i = 0; i < n; ++i) {
        ramp[i] = static_cast<double>(i);
    }
    const auto output = sp::filters::savitzky_golay(ramp, 5U, 2U);
    TYST_ASSERT_EQ(output.size(), ramp.size());
    for (std::size_t i = 2U; i < n - 2U; ++i) {
        TYST_EXPECT_NEAR(output[i], ramp[i], 1e-9);
    }
}

TYST_TEST(FiltersTest, BiquadNotchAttenuatesCentreFrequency) {
    const double centre = 0.25;
    const auto coeffs = sp::filters::biquad_notch(centre, 0.05);
    const auto mag = sp::filters::frequency_response(coeffs, 256U);
    // At the centre frequency index the gain should be very low
    const std::size_t centre_idx = static_cast<std::size_t>(centre * 256.0);
    TYST_EXPECT_LT(mag[centre_idx], 0.1);
}

TYST_TEST(FiltersTest, InvalidOrderThrows) {
    TYST_EXPECT_THROW(sp::filters::fir_lowpass(0U, 0.3), std::invalid_argument);
    TYST_EXPECT_THROW(sp::filters::butterworth_lowpass(0U, 0.3), std::invalid_argument);
    TYST_EXPECT_THROW(sp::filters::chebyshev1_lowpass(0U, 0.3, 1.0), std::invalid_argument);
    TYST_EXPECT_THROW(sp::filters::chebyshev1_lowpass(2U, 0.3, -1.0), std::invalid_argument);
}

TYST_TEST(FiltersTest, MovingAverageZeroWindowThrows) {
    TYST_EXPECT_THROW(sp::filters::moving_average({1.0, 2.0, 3.0}, 0U), std::invalid_argument);
}

TYST_TEST(FiltersTest, SavitzkyGolayEvenWindowThrows) {
    TYST_EXPECT_THROW(sp::filters::savitzky_golay({1.0, 2.0, 3.0}, 4U, 2U), std::invalid_argument);
}

}  // namespace
