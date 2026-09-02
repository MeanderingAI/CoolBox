#include "fourier_tranforms.hpp"

#include "tyst_framework.hpp"

#include <cmath>

namespace {

using sp::fourier_tranforms::Complex;

TYST_TEST(FourierTranformsTest, FftMatchesDftForPowerOfTwoInput) {
    const std::vector<Complex> samples = {
        {1.0, 0.0},
        {2.0, -1.0},
        {0.0, 0.5},
        {-1.0, 0.0}
    };

    const auto dft_values = sp::fourier_tranforms::dft(samples);
    const auto fft_values = sp::fourier_tranforms::fft(samples);

    TYST_ASSERT_EQ(dft_values.size(), fft_values.size());
    for (std::size_t index = 0; index < dft_values.size(); ++index) {
        TYST_EXPECT_NEAR(dft_values[index].real(), fft_values[index].real(), 1e-9);
        TYST_EXPECT_NEAR(dft_values[index].imag(), fft_values[index].imag(), 1e-9);
    }
}

TYST_TEST(FourierTranformsTest, IfftReconstructsOriginalSignal) {
    const std::vector<Complex> samples = {
        {0.0, 0.0},
        {1.0, 0.0},
        {0.0, 0.0},
        {-1.0, 0.0},
        {0.5, 0.0},
        {-0.5, 0.0},
        {0.25, 0.0},
        {-0.25, 0.0}
    };

    const auto reconstructed = sp::fourier_tranforms::ifft(sp::fourier_tranforms::fft(samples));
    TYST_ASSERT_EQ(reconstructed.size(), samples.size());
    for (std::size_t index = 0; index < samples.size(); ++index) {
        TYST_EXPECT_NEAR(reconstructed[index].real(), samples[index].real(), 1e-9);
        TYST_EXPECT_NEAR(reconstructed[index].imag(), samples[index].imag(), 1e-9);
    }
}

TYST_TEST(FourierTranformsTest, RealFftAndInverseRealFftRoundTrip) {
    const std::vector<double> samples = {0.0, 1.0, 0.0, -1.0, 0.5, 0.0, -0.5, 0.0};
    const auto spectrum = sp::fourier_tranforms::real_fft(samples);
    const auto reconstructed = sp::fourier_tranforms::inverse_real_fft(spectrum);

    TYST_ASSERT_EQ(reconstructed.size(), samples.size());
    for (std::size_t index = 0; index < samples.size(); ++index) {
        TYST_EXPECT_NEAR(reconstructed[index], samples[index], 1e-9);
    }
}

TYST_TEST(FourierTranformsTest, TwoDimensionalTransformsRoundTrip) {
    const sp::fourier_tranforms::ComplexMatrix matrix({
        {{1.0, 0.0}, {2.0, 0.0}},
        {{3.0, 0.0}, {4.0, 0.0}}
    });

    const auto transformed = sp::fourier_tranforms::fft2d(matrix);
    const auto reconstructed = sp::fourier_tranforms::ifft2d(transformed);

    TYST_ASSERT_EQ(reconstructed.rows(), matrix.rows());
    TYST_ASSERT_EQ(reconstructed.cols(), matrix.cols());
    for (std::size_t row = 0; row < matrix.rows(); ++row) {
        for (std::size_t column = 0; column < matrix.cols(); ++column) {
            TYST_EXPECT_NEAR(reconstructed.at(row, column).real(), matrix.at(row, column).real(), 1e-9);
            TYST_EXPECT_NEAR(reconstructed.at(row, column).imag(), matrix.at(row, column).imag(), 1e-9);
        }
    }
}

TYST_TEST(FourierTranformsTest, StftProducesWindowedFrames) {
    const std::vector<double> signal = {1.0, 0.0, -1.0, 0.0, 1.0, 0.0, -1.0, 0.0};
    const auto frames = sp::fourier_tranforms::stft(signal, 4U, 2U, sp::fourier_tranforms::WindowType::Hann);

    TYST_ASSERT_EQ(frames.size(), 3U);
    for (const auto& frame : frames) {
        TYST_EXPECT_EQ(frame.size(), 4U);
    }
}

TYST_TEST(FourierTranformsTest, InverseStftReconstructsWindowedSignal) {
    const std::vector<double> signal = {1.0, 0.0, -1.0, 0.0, 1.0, 0.0, -1.0, 0.0};
    const auto frames = sp::fourier_tranforms::stft(signal, 4U, 2U, sp::fourier_tranforms::WindowType::Rectangular);
    const auto reconstructed = sp::fourier_tranforms::inverse_stft(frames, 4U, 2U, sp::fourier_tranforms::WindowType::Rectangular);

    TYST_ASSERT_GE(reconstructed.size(), signal.size());
    for (std::size_t index = 0; index < signal.size(); ++index) {
        TYST_EXPECT_NEAR(reconstructed[index], signal[index], 1e-9);
    }
}

TYST_TEST(FourierTranformsTest, DctAndIdctRoundTrip) {
    const std::vector<double> samples = {1.0, 2.0, 3.0, 4.0};
    const auto coefficients = sp::fourier_tranforms::dct(samples);
    const auto reconstructed = sp::fourier_tranforms::idct(coefficients);

    TYST_ASSERT_EQ(reconstructed.size(), samples.size());
    for (std::size_t index = 0; index < samples.size(); ++index) {
        TYST_EXPECT_NEAR(reconstructed[index], samples[index], 1e-9);
    }
}

TYST_TEST(FourierTranformsTest, DstAndIdstRoundTrip) {
    const std::vector<double> samples = {0.5, 1.5, -0.5, 2.0};
    const auto coefficients = sp::fourier_tranforms::dst(samples);
    const auto reconstructed = sp::fourier_tranforms::idst(coefficients);

    TYST_ASSERT_EQ(reconstructed.size(), samples.size());
    for (std::size_t index = 0; index < samples.size(); ++index) {
        TYST_EXPECT_NEAR(reconstructed[index], samples[index], 1e-9);
    }
}

TYST_TEST(FourierTranformsTest, CircularConvolutionMatchesExpectedImpulseResponse) {
    const std::vector<double> lhs = {1.0, 2.0, 0.0, 0.0};
    const std::vector<double> rhs = {1.0, 1.0, 0.0, 0.0};
    const auto result = sp::fourier_tranforms::circular_convolution(lhs, rhs);

    TYST_ASSERT_EQ(result.size(), 4U);
    TYST_EXPECT_NEAR(result[0], 1.0, 1e-9);
    TYST_EXPECT_NEAR(result[1], 3.0, 1e-9);
    TYST_EXPECT_NEAR(result[2], 2.0, 1e-9);
    TYST_EXPECT_NEAR(result[3], 0.0, 1e-9);
}

TYST_TEST(FourierTranformsTest, LinearConvolutionProducesFullLengthOutput) {
    const std::vector<double> lhs = {1.0, 2.0, 3.0};
    const std::vector<double> rhs = {4.0, 5.0};
    const auto result = sp::fourier_tranforms::linear_convolution(lhs, rhs);

    TYST_ASSERT_EQ(result.size(), 4U);
    TYST_EXPECT_NEAR(result[0], 4.0, 1e-9);
    TYST_EXPECT_NEAR(result[1], 13.0, 1e-9);
    TYST_EXPECT_NEAR(result[2], 22.0, 1e-9);
    TYST_EXPECT_NEAR(result[3], 15.0, 1e-9);
}

TYST_TEST(FourierTranformsTest, PhaseSpectrumAndCrossPowerSpectrumExposeFrequencyRelations) {
    const std::vector<Complex> lhs = {{1.0, 0.0}, {0.0, 1.0}};
    const std::vector<Complex> rhs = {{1.0, 0.0}, {0.0, -1.0}};

    const auto phases = sp::fourier_tranforms::phase_spectrum(lhs);
    const auto cps = sp::fourier_tranforms::cross_power_spectrum(lhs, rhs);

    TYST_ASSERT_EQ(phases.size(), 2U);
    TYST_EXPECT_NEAR(phases[0], 0.0, 1e-9);
    TYST_EXPECT_NEAR(phases[1], sp::fourier_tranforms::phase_spectrum({{0.0, 1.0}})[0], 1e-9);
    TYST_ASSERT_EQ(cps.size(), 2U);
    TYST_EXPECT_NEAR(cps[0].real(), 1.0, 1e-9);
    TYST_EXPECT_NEAR(cps[1].real(), -1.0, 1e-9);
}

TYST_TEST(FourierTranformsTest, HartleyTransformIsSelfInverseUpToScaleForUnitImpulse) {
    const std::vector<double> samples = {1.0, 0.0, 0.0, 0.0};
    const auto hartley = sp::fourier_tranforms::hartley_transform(samples);
    const auto inverse = sp::fourier_tranforms::inverse_hartley_transform(hartley);

    TYST_ASSERT_EQ(hartley.size(), samples.size());
    TYST_EXPECT_NEAR(hartley[0], 1.0, 1e-9);
    TYST_EXPECT_EQ(inverse.size(), samples.size());
}

TYST_TEST(FourierTranformsTest, AutocorrelationViaFftMatchesImpulsePattern) {
    const std::vector<double> samples = {1.0, 0.0, 0.0, 0.0};
    const auto autocorrelation = sp::fourier_tranforms::autocorrelation_via_fft(samples);

    TYST_ASSERT_EQ(autocorrelation.size(), samples.size());
    TYST_EXPECT_NEAR(autocorrelation[0], 1.0, 1e-9);
    TYST_EXPECT_NEAR(autocorrelation[1], 0.0, 1e-9);
}

} // namespace