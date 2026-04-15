#ifndef COOLBOX__LIBRARIES_PACKAGES_SP_FOURIER_TRANFORMS_HEADERS_FOURIER_TRANFORMS_HPP
#define COOLBOX__LIBRARIES_PACKAGES_SP_FOURIER_TRANFORMS_HEADERS_FOURIER_TRANFORMS_HPP

#include <complex>
#include <cstddef>
#include <vector>

#include "matrix_dense.h"

namespace sp {
namespace fourier_tranforms {

using Complex = std::complex<double>;

struct ComplexMatrix {
    matrix::DenseMatrix real;
    matrix::DenseMatrix imag;

    ComplexMatrix(std::size_t rows, std::size_t cols);
    explicit ComplexMatrix(const std::vector<std::vector<Complex>>& values);

    std::size_t rows() const;
    std::size_t cols() const;
    Complex at(std::size_t row, std::size_t col) const;
    void set(std::size_t row, std::size_t col, Complex value);
    std::vector<std::vector<Complex>> to_nested_vector() const;
};

enum class WindowType {
    Rectangular,
    Hann,
    Hamming,
    Blackman
};

std::vector<Complex> dft(const std::vector<Complex>& samples);
std::vector<Complex> idft(const std::vector<Complex>& spectrum);
std::vector<Complex> fft(const std::vector<Complex>& samples);
std::vector<Complex> ifft(const std::vector<Complex>& spectrum);

std::vector<Complex> real_fft(const std::vector<double>& samples);
std::vector<double> inverse_real_fft(const std::vector<Complex>& spectrum);

ComplexMatrix dft2d(const ComplexMatrix& samples);
ComplexMatrix idft2d(const ComplexMatrix& spectrum);
ComplexMatrix fft2d(const ComplexMatrix& samples);
ComplexMatrix ifft2d(const ComplexMatrix& spectrum);

std::vector<std::vector<Complex>> stft(const std::vector<double>& signal,
                                       std::size_t window_size,
                                       std::size_t hop_size,
                                       WindowType window_type = WindowType::Hann);
std::vector<double> inverse_stft(const std::vector<std::vector<Complex>>& frames,
                                 std::size_t window_size,
                                 std::size_t hop_size,
                                 WindowType window_type = WindowType::Hann);

std::vector<double> window(WindowType type, std::size_t size);
std::vector<double> frequency_bins(std::size_t sample_count, double sample_rate_hz);
std::vector<double> magnitude_spectrum(const std::vector<Complex>& spectrum);
std::vector<double> power_spectrum(const std::vector<Complex>& spectrum);
std::vector<double> phase_spectrum(const std::vector<Complex>& spectrum);
std::vector<Complex> cross_power_spectrum(const std::vector<Complex>& lhs,
                                          const std::vector<Complex>& rhs);
std::vector<Complex> fft_shift(const std::vector<Complex>& spectrum);

std::vector<double> dct(const std::vector<double>& samples);
std::vector<double> idct(const std::vector<double>& coefficients);
std::vector<double> dst(const std::vector<double>& samples);
std::vector<double> idst(const std::vector<double>& coefficients);
std::vector<double> hartley_transform(const std::vector<double>& samples);
std::vector<double> inverse_hartley_transform(const std::vector<double>& coefficients);

std::vector<double> circular_convolution(const std::vector<double>& lhs,
                                         const std::vector<double>& rhs);
std::vector<double> linear_convolution(const std::vector<double>& lhs,
                                       const std::vector<double>& rhs);
std::vector<double> autocorrelation_via_fft(const std::vector<double>& samples);

} // namespace fourier_tranforms
} // namespace sp

#endif  // COOLBOX__LIBRARIES_PACKAGES_SP_FOURIER_TRANFORMS_HEADERS_FOURIER_TRANFORMS_HPP