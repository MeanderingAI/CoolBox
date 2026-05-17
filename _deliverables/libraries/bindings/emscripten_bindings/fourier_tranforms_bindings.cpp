#include <emscripten/bind.h>

#include <complex>
#include <vector>

#include "fourier_tranforms.hpp"

using namespace emscripten;

namespace {

using sp::fourier_tranforms::Complex;

struct SpectrumPoint {
    double real;
    double imag;
    double magnitude;
    double phase;
};

std::vector<Complex> to_complex_samples(const std::vector<double>& samples) {
    std::vector<Complex> complex_samples;
    complex_samples.reserve(samples.size());
    for (double sample : samples) {
        complex_samples.emplace_back(sample, 0.0);
    }
    return complex_samples;
}

std::vector<SpectrumPoint> to_spectrum_points(const std::vector<Complex>& spectrum) {
    std::vector<SpectrumPoint> points;
    points.reserve(spectrum.size());
    for (const auto& value : spectrum) {
        points.push_back(SpectrumPoint{
            value.real(),
            value.imag(),
            std::abs(value),
            std::atan2(value.imag(), value.real())
        });
    }
    return points;
}

std::vector<SpectrumPoint> js_real_fft_spectrum(const std::vector<double>& samples) {
    return to_spectrum_points(sp::fourier_tranforms::real_fft(samples));
}

std::vector<SpectrumPoint> js_dft_spectrum(const std::vector<double>& samples) {
    return to_spectrum_points(sp::fourier_tranforms::dft(to_complex_samples(samples)));
}

std::vector<double> js_inverse_real_fft(const std::vector<SpectrumPoint>& spectrum_points) {
    std::vector<Complex> spectrum;
    spectrum.reserve(spectrum_points.size());
    for (const auto& point : spectrum_points) {
        spectrum.emplace_back(point.real, point.imag);
    }
    return sp::fourier_tranforms::inverse_real_fft(spectrum);
}

std::vector<double> js_frequency_bins(std::size_t sample_count, double sample_rate_hz) {
    return sp::fourier_tranforms::frequency_bins(sample_count, sample_rate_hz);
}

} // namespace

EMSCRIPTEN_BINDINGS(fourier_tranforms_module) {
    register_vector<double>("VectorDouble_Fourier");
    register_vector<SpectrumPoint>("VectorSpectrumPoint");

    value_object<SpectrumPoint>("SpectrumPoint")
        .field("real", &SpectrumPoint::real)
        .field("imag", &SpectrumPoint::imag)
        .field("magnitude", &SpectrumPoint::magnitude)
        .field("phase", &SpectrumPoint::phase);

    function("real_fft_spectrum", &js_real_fft_spectrum);
    function("dft_spectrum", &js_dft_spectrum);
    function("inverse_real_fft", &js_inverse_real_fft);
    function("frequency_bins", &js_frequency_bins);
}
