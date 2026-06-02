#include "fourier_tranforms.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace sp {
namespace fourier_tranforms {
namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kTwoPi = 2.0 * kPi;

std::size_t next_power_of_two(std::size_t value) {
    if (value <= 1U) {
        return 1U;
    }
    std::size_t power = 1U;
    while (power < value) {
        power <<= 1U;
    }
    return power;
}

bool is_power_of_two(std::size_t value) {
    return value != 0U && (value & (value - 1U)) == 0U;
}

std::vector<Complex> fft_impl(const std::vector<Complex>& samples, bool inverse) {
    const std::size_t count = samples.size();
    if (count <= 1U) {
        return samples;
    }

    if (!is_power_of_two(count)) {
        return inverse ? idft(samples) : dft(samples);
    }

    std::vector<Complex> even;
    std::vector<Complex> odd;
    even.reserve(count / 2U);
    odd.reserve(count / 2U);
    for (std::size_t index = 0; index < count; ++index) {
        if ((index % 2U) == 0U) {
            even.push_back(samples[index]);
        } else {
            odd.push_back(samples[index]);
        }
    }

    const auto even_fft = fft_impl(even, inverse);
    const auto odd_fft = fft_impl(odd, inverse);
    std::vector<Complex> output(count);

    const double sign = inverse ? 1.0 : -1.0;
    for (std::size_t k = 0; k < count / 2U; ++k) {
        const double angle = sign * kTwoPi * static_cast<double>(k) / static_cast<double>(count);
        const Complex twiddle = std::polar(1.0, angle) * odd_fft[k];
        output[k] = even_fft[k] + twiddle;
        output[k + (count / 2U)] = even_fft[k] - twiddle;
    }

    if (inverse) {
        for (auto& value : output) {
            value /= 2.0;
        }
    }
    return output;
}

template <typename TransformFn>
ComplexMatrix transform2d(const ComplexMatrix& input, TransformFn&& transform) {
    const std::size_t rows = input.rows();
    const std::size_t columns = input.cols();
    if (rows == 0U || columns == 0U) {
        return ComplexMatrix(0U, 0U);
    }

    ComplexMatrix row_transformed(rows, columns);
    for (std::size_t row = 0; row < rows; ++row) {
        std::vector<Complex> row_values(columns);
        for (std::size_t column = 0; column < columns; ++column) {
            row_values[column] = input.at(row, column);
        }
        const auto transformed_row = transform(row_values);
        for (std::size_t column = 0; column < columns; ++column) {
            row_transformed.set(row, column, transformed_row[column]);
        }
    }

    ComplexMatrix output(rows, columns);
    for (std::size_t column = 0; column < columns; ++column) {
        std::vector<Complex> column_values(rows);
        for (std::size_t row = 0; row < rows; ++row) {
            column_values[row] = row_transformed.at(row, column);
        }
        const auto transformed_column = transform(column_values);
        for (std::size_t row = 0; row < rows; ++row) {
            output.set(row, column, transformed_column[row]);
        }
    }
    return output;
}

} // namespace

ComplexMatrix::ComplexMatrix(std::size_t rows, std::size_t cols)
    : real(rows, cols), imag(rows, cols) {}

ComplexMatrix::ComplexMatrix(const std::vector<std::vector<Complex>>& values)
        : real(values.size(), values.empty() ? 0U : values.front().size()),
            imag(values.size(), values.empty() ? 0U : values.front().size()) {
    if (values.empty()) {
        return;
    }

    const std::size_t columns = values.front().size();
    for (const auto& row : values) {
        if (row.size() != columns) {
            throw std::invalid_argument("matrix rows must have equal length");
        }
    }

    for (std::size_t row = 0; row < values.size(); ++row) {
        for (std::size_t column = 0; column < columns; ++column) {
            set(row, column, values[row][column]);
        }
    }
}

std::size_t ComplexMatrix::rows() const {
    return real.rows();
}

std::size_t ComplexMatrix::cols() const {
    return real.cols();
}

Complex ComplexMatrix::at(std::size_t row, std::size_t col) const {
    return Complex(real.at(row, col), imag.at(row, col));
}

void ComplexMatrix::set(std::size_t row, std::size_t col, Complex value) {
    real.at(row, col) = value.real();
    imag.at(row, col) = value.imag();
}

std::vector<std::vector<Complex>> ComplexMatrix::to_nested_vector() const {
    std::vector<std::vector<Complex>> values(rows(), std::vector<Complex>(cols()));
    for (std::size_t row = 0; row < rows(); ++row) {
        for (std::size_t col = 0; col < cols(); ++col) {
            values[row][col] = at(row, col);
        }
    }
    return values;
}

std::vector<Complex> dft(const std::vector<Complex>& samples) {
    const std::size_t count = samples.size();
    std::vector<Complex> output(count, Complex{});
    for (std::size_t k = 0; k < count; ++k) {
        for (std::size_t n = 0; n < count; ++n) {
            const double angle = -kTwoPi * static_cast<double>(k * n) / static_cast<double>(count);
            output[k] += samples[n] * std::polar(1.0, angle);
        }
    }
    return output;
}

std::vector<Complex> idft(const std::vector<Complex>& spectrum) {
    const std::size_t count = spectrum.size();
    std::vector<Complex> output(count, Complex{});
    for (std::size_t n = 0; n < count; ++n) {
        for (std::size_t k = 0; k < count; ++k) {
            const double angle = kTwoPi * static_cast<double>(k * n) / static_cast<double>(count);
            output[n] += spectrum[k] * std::polar(1.0, angle);
        }
        if (count != 0U) {
            output[n] /= static_cast<double>(count);
        }
    }
    return output;
}

std::vector<Complex> fft(const std::vector<Complex>& samples) {
    return fft_impl(samples, false);
}

std::vector<Complex> ifft(const std::vector<Complex>& spectrum) {
    return fft_impl(spectrum, true);
}

std::vector<Complex> real_fft(const std::vector<double>& samples) {
    std::vector<Complex> complex_samples;
    complex_samples.reserve(samples.size());
    for (double value : samples) {
        complex_samples.emplace_back(value, 0.0);
    }
    return fft(complex_samples);
}

std::vector<double> inverse_real_fft(const std::vector<Complex>& spectrum) {
    const auto complex_signal = ifft(spectrum);
    std::vector<double> output;
    output.reserve(complex_signal.size());
    for (const auto& value : complex_signal) {
        output.push_back(value.real());
    }
    return output;
}

ComplexMatrix dft2d(const ComplexMatrix& samples) {
    return transform2d(samples, [](const std::vector<Complex>& row) { return dft(row); });
}

ComplexMatrix idft2d(const ComplexMatrix& spectrum) {
    return transform2d(spectrum, [](const std::vector<Complex>& row) { return idft(row); });
}

ComplexMatrix fft2d(const ComplexMatrix& samples) {
    return transform2d(samples, [](const std::vector<Complex>& row) { return fft(row); });
}

ComplexMatrix ifft2d(const ComplexMatrix& spectrum) {
    return transform2d(spectrum, [](const std::vector<Complex>& row) { return ifft(row); });
}

std::vector<double> window(WindowType type, std::size_t size) {
    std::vector<double> coefficients(size, 1.0);
    if (size == 0U) {
        return coefficients;
    }
    if (size == 1U) {
        coefficients.front() = 1.0;
        return coefficients;
    }

    const double denominator = static_cast<double>(size - 1U);
    for (std::size_t index = 0; index < size; ++index) {
        const double phase = kTwoPi * static_cast<double>(index) / denominator;
        switch (type) {
        case WindowType::Rectangular:
            coefficients[index] = 1.0;
            break;
        case WindowType::Hann:
            coefficients[index] = 0.5 - (0.5 * std::cos(phase));
            break;
        case WindowType::Hamming:
            coefficients[index] = 0.54 - (0.46 * std::cos(phase));
            break;
        case WindowType::Blackman:
            coefficients[index] = 0.42 - (0.5 * std::cos(phase)) + (0.08 * std::cos(2.0 * phase));
            break;
        }
    }
    return coefficients;
}

std::vector<std::vector<Complex>> stft(const std::vector<double>& signal,
                                       std::size_t window_size,
                                       std::size_t hop_size,
                                       WindowType window_type) {
    if (window_size == 0U) {
        throw std::invalid_argument("window_size must be greater than zero");
    }
    if (hop_size == 0U) {
        throw std::invalid_argument("hop_size must be greater than zero");
    }
    if (signal.empty()) {
        return {};
    }

    const auto coefficients = window(window_type, window_size);
    std::vector<std::vector<Complex>> frames;
    for (std::size_t start = 0; start < signal.size(); start += hop_size) {
        std::vector<double> frame(window_size, 0.0);
        for (std::size_t index = 0; index < window_size; ++index) {
            const std::size_t sample_index = start + index;
            if (sample_index < signal.size()) {
                frame[index] = signal[sample_index] * coefficients[index];
            }
        }
        frames.push_back(real_fft(frame));
        if (start + window_size >= signal.size()) {
            break;
        }
    }
    return frames;
}

std::vector<double> inverse_stft(const std::vector<std::vector<Complex>>& frames,
                                 std::size_t window_size,
                                 std::size_t hop_size,
                                 WindowType window_type) {
    if (window_size == 0U) {
        throw std::invalid_argument("window_size must be greater than zero");
    }
    if (hop_size == 0U) {
        throw std::invalid_argument("hop_size must be greater than zero");
    }
    if (frames.empty()) {
        return {};
    }

    const auto coefficients = window(window_type, window_size);
    const std::size_t output_size = ((frames.size() - 1U) * hop_size) + window_size;
    std::vector<double> signal(output_size, 0.0);
    std::vector<double> weights(output_size, 0.0);

    for (std::size_t frame_index = 0; frame_index < frames.size(); ++frame_index) {
        const auto time_frame = inverse_real_fft(frames[frame_index]);
        for (std::size_t sample_index = 0; sample_index < window_size && sample_index < time_frame.size(); ++sample_index) {
            const std::size_t output_index = (frame_index * hop_size) + sample_index;
            if (output_index < output_size) {
                const double window_value = coefficients[sample_index];
                signal[output_index] += time_frame[sample_index] * window_value;
                weights[output_index] += window_value * window_value;
            }
        }
    }

    for (std::size_t index = 0; index < output_size; ++index) {
        if (weights[index] > 1e-12) {
            signal[index] /= weights[index];
        }
    }
    return signal;
}

std::vector<double> frequency_bins(std::size_t sample_count, double sample_rate_hz) {
    if (sample_count == 0U) {
        return {};
    }
    if (sample_rate_hz <= 0.0 || !std::isfinite(sample_rate_hz)) {
        throw std::invalid_argument("sample_rate_hz must be positive and finite");
    }

    std::vector<double> bins(sample_count, 0.0);
    for (std::size_t index = 0; index < sample_count; ++index) {
        bins[index] = static_cast<double>(index) * sample_rate_hz / static_cast<double>(sample_count);
    }
    return bins;
}

std::vector<double> magnitude_spectrum(const std::vector<Complex>& spectrum) {
    std::vector<double> magnitudes;
    magnitudes.reserve(spectrum.size());
    for (const auto& value : spectrum) {
        magnitudes.push_back(std::abs(value));
    }
    return magnitudes;
}

std::vector<double> power_spectrum(const std::vector<Complex>& spectrum) {
    std::vector<double> power;
    power.reserve(spectrum.size());
    for (const auto& value : spectrum) {
        power.push_back(std::norm(value));
    }
    return power;
}

std::vector<double> phase_spectrum(const std::vector<Complex>& spectrum) {
    std::vector<double> phase;
    phase.reserve(spectrum.size());
    for (const auto& value : spectrum) {
        phase.push_back(std::atan2(value.imag(), value.real()));
    }
    return phase;
}

std::vector<Complex> cross_power_spectrum(const std::vector<Complex>& lhs,
                                          const std::vector<Complex>& rhs) {
    if (lhs.size() != rhs.size()) {
        throw std::invalid_argument("lhs and rhs spectra must have the same size");
    }
    std::vector<Complex> output(lhs.size());
    for (std::size_t index = 0; index < lhs.size(); ++index) {
        output[index] = lhs[index] * std::conj(rhs[index]);
    }
    return output;
}

std::vector<Complex> fft_shift(const std::vector<Complex>& spectrum) {
    std::vector<Complex> shifted = spectrum;
    if (!shifted.empty()) {
        std::rotate(shifted.begin(), shifted.begin() + static_cast<std::ptrdiff_t>(shifted.size() / 2U), shifted.end());
    }
    return shifted;
}

std::vector<double> dct(const std::vector<double>& samples) {
    const std::size_t count = samples.size();
    std::vector<double> coefficients(count, 0.0);
    for (std::size_t k = 0; k < count; ++k) {
        for (std::size_t n = 0; n < count; ++n) {
            coefficients[k] += samples[n] * std::cos(kPi * (static_cast<double>(n) + 0.5) * static_cast<double>(k) / static_cast<double>(count));
        }
    }
    return coefficients;
}

std::vector<double> idct(const std::vector<double>& coefficients) {
    const std::size_t count = coefficients.size();
    std::vector<double> samples(count, 0.0);
    for (std::size_t n = 0; n < count; ++n) {
        double value = count == 0U ? 0.0 : coefficients[0] * 0.5;
        for (std::size_t k = 1; k < count; ++k) {
            value += coefficients[k] * std::cos(kPi * static_cast<double>(k) * (static_cast<double>(n) + 0.5) / static_cast<double>(count));
        }
        if (count != 0U) {
            samples[n] = (2.0 / static_cast<double>(count)) * value;
        }
    }
    return samples;
}

std::vector<double> dst(const std::vector<double>& samples) {
    const std::size_t count = samples.size();
    std::vector<double> coefficients(count, 0.0);
    for (std::size_t k = 0; k < count; ++k) {
        for (std::size_t n = 0; n < count; ++n) {
            coefficients[k] += samples[n] * std::sin(kPi * (static_cast<double>(n) + 0.5) * (static_cast<double>(k) + 1.0) / static_cast<double>(count));
        }
    }
    return coefficients;
}

std::vector<double> idst(const std::vector<double>& coefficients) {
    const std::size_t count = coefficients.size();
    std::vector<double> samples(count, 0.0);
    for (std::size_t n = 0; n < count; ++n) {
        for (std::size_t k = 0; k < count; ++k) {
            const double weight = (k + 1U == count) ? 0.5 : 1.0;
            samples[n] += weight * coefficients[k] * std::sin(kPi * (static_cast<double>(n) + 0.5) * (static_cast<double>(k) + 1.0) / static_cast<double>(count));
        }
        if (count != 0U) {
            samples[n] *= 2.0 / static_cast<double>(count);
        }
    }
    return samples;
}

std::vector<double> hartley_transform(const std::vector<double>& samples) {
    const auto spectrum = real_fft(samples);
    std::vector<double> output;
    output.reserve(spectrum.size());
    for (const auto& value : spectrum) {
        output.push_back(value.real() - value.imag());
    }
    return output;
}

std::vector<double> inverse_hartley_transform(const std::vector<double>& coefficients) {
    return hartley_transform(coefficients);
}

std::vector<double> circular_convolution(const std::vector<double>& lhs,
                                         const std::vector<double>& rhs) {
    if (lhs.empty() || rhs.empty()) {
        return {};
    }

    const std::size_t output_size = std::max(lhs.size(), rhs.size());
    std::vector<Complex> lhs_complex(output_size, Complex{});
    std::vector<Complex> rhs_complex(output_size, Complex{});
    for (std::size_t index = 0; index < lhs.size(); ++index) {
        lhs_complex[index] = Complex(lhs[index], 0.0);
    }
    for (std::size_t index = 0; index < rhs.size(); ++index) {
        rhs_complex[index] = Complex(rhs[index], 0.0);
    }

    const auto lhs_fft = fft(lhs_complex);
    const auto rhs_fft = fft(rhs_complex);
    std::vector<Complex> product(output_size, Complex{});
    for (std::size_t index = 0; index < output_size; ++index) {
        product[index] = lhs_fft[index] * rhs_fft[index];
    }

    const auto convolved = ifft(product);
    std::vector<double> output;
    output.reserve(convolved.size());
    for (const auto& value : convolved) {
        output.push_back(value.real());
    }
    return output;
}

std::vector<double> linear_convolution(const std::vector<double>& lhs,
                                       const std::vector<double>& rhs) {
    if (lhs.empty() || rhs.empty()) {
        return {};
    }

    const std::size_t linear_size = lhs.size() + rhs.size() - 1U;
    const std::size_t fft_size = next_power_of_two(linear_size);
    std::vector<Complex> lhs_complex(fft_size, Complex{});
    std::vector<Complex> rhs_complex(fft_size, Complex{});
    for (std::size_t index = 0; index < lhs.size(); ++index) {
        lhs_complex[index] = Complex(lhs[index], 0.0);
    }
    for (std::size_t index = 0; index < rhs.size(); ++index) {
        rhs_complex[index] = Complex(rhs[index], 0.0);
    }

    const auto lhs_fft = fft(lhs_complex);
    const auto rhs_fft = fft(rhs_complex);
    std::vector<Complex> product(fft_size, Complex{});
    for (std::size_t index = 0; index < fft_size; ++index) {
        product[index] = lhs_fft[index] * rhs_fft[index];
    }

    const auto convolved = ifft(product);
    std::vector<double> output(linear_size, 0.0);
    for (std::size_t index = 0; index < linear_size; ++index) {
        output[index] = convolved[index].real();
    }
    return output;
}

std::vector<double> autocorrelation_via_fft(const std::vector<double>& samples) {
    if (samples.empty()) {
        return {};
    }

    const std::size_t fft_size = next_power_of_two(samples.size() * 2U);
    std::vector<Complex> padded(fft_size, Complex{});
    for (std::size_t index = 0; index < samples.size(); ++index) {
        padded[index] = Complex(samples[index], 0.0);
    }

    const auto spectrum = fft(padded);
    const auto power = cross_power_spectrum(spectrum, spectrum);
    const auto correlation = ifft(power);

    std::vector<double> output(samples.size(), 0.0);
    for (std::size_t index = 0; index < samples.size(); ++index) {
        output[index] = correlation[index].real();
    }
    return output;
}

} // namespace fourier_tranforms
} // namespace sp