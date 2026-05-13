#include "resampling.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace sp {
namespace resampling {

namespace {

constexpr double kPi = 3.14159265358979323846;

double sinc(double x) {
    if (std::abs(x) < 1e-12) {
        return 1.0;
    }
    return std::sin(kPi * x) / (kPi * x);
}

double hann(std::size_t i, std::size_t length) {
    return 0.5 * (1.0 - std::cos(2.0 * kPi * static_cast<double>(i) /
                                  static_cast<double>(length - 1U)));
}

// Build a windowed-sinc FIR of given order with normalised cutoff (fraction of Nyquist).
std::vector<double> make_sinc_fir(std::size_t num_taps, double cutoff) {
    if ((num_taps % 2U) == 0U) {
        ++num_taps;  // ensure odd
    }
    const std::size_t half = num_taps / 2U;
    std::vector<double> h(num_taps);
    for (std::size_t i = 0; i < num_taps; ++i) {
        const double t = static_cast<double>(i) - static_cast<double>(half);
        h[i] = 2.0 * cutoff * sinc(2.0 * cutoff * t) * hann(i, num_taps);
    }
    return h;
}

// Apply a FIR filter kernel to a signal (direct convolution, causal).
std::vector<double> fir_filter(const std::vector<double>& h,
                               const std::vector<double>& signal) {
    const std::size_t n_h = h.size();
    const std::size_t n   = signal.size();
    std::vector<double> output(n, 0.0);
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t k = 0; k < n_h && k <= i; ++k) {
            output[i] += h[k] * signal[i - k];
        }
    }
    return output;
}

double clamp_index(const std::vector<double>& v, std::ptrdiff_t i) {
    if (i < 0) {
        return v[0];
    }
    if (static_cast<std::size_t>(i) >= v.size()) {
        return v.back();
    }
    return v[static_cast<std::size_t>(i)];
}

}  // namespace

// ─── Integer-ratio ────────────────────────────────────────────────────────────

std::vector<double> upsample(const std::vector<double>& signal, std::size_t factor) {
    if (factor == 0U) {
        throw std::invalid_argument("upsample factor must be > 0");
    }
    if (factor == 1U) {
        return signal;
    }
    const std::size_t n = signal.size();
    std::vector<double> output(n * factor, 0.0);
    for (std::size_t i = 0; i < n; ++i) {
        output[i * factor] = signal[i];
    }
    return output;
}

std::vector<double> downsample(const std::vector<double>& signal, std::size_t factor) {
    if (factor == 0U) {
        throw std::invalid_argument("downsample factor must be > 0");
    }
    if (factor == 1U) {
        return signal;
    }
    const std::size_t out_len = (signal.size() + factor - 1U) / factor;
    std::vector<double> output(out_len);
    for (std::size_t i = 0; i < out_len; ++i) {
        output[i] = signal[i * factor];
    }
    return output;
}

std::vector<double> decimate(const std::vector<double>& signal, std::size_t factor) {
    if (factor == 0U) {
        throw std::invalid_argument("decimate factor must be > 0");
    }
    if (factor == 1U) {
        return signal;
    }
    const double cutoff = 0.5 / static_cast<double>(factor);
    const auto h = make_sinc_fir(63U, cutoff);
    const auto filtered = fir_filter(h, signal);
    return downsample(filtered, factor);
}

std::vector<double> interpolate(const std::vector<double>& signal, std::size_t factor) {
    if (factor == 0U) {
        throw std::invalid_argument("interpolate factor must be > 0");
    }
    if (factor == 1U) {
        return signal;
    }
    const auto upsampled = upsample(signal, factor);
    const double cutoff = 0.5 / static_cast<double>(factor);
    const auto h = make_sinc_fir(63U, cutoff);
    auto filtered = fir_filter(h, upsampled);
    // Normalise gain
    for (auto& v : filtered) {
        v *= static_cast<double>(factor);
    }
    return filtered;
}

std::vector<double> resample_rational(const std::vector<double>& signal,
                                      std::size_t up,
                                      std::size_t down) {
    if (up == 0U || down == 0U) {
        throw std::invalid_argument("resample_rational: up and down must be > 0");
    }
    const auto upsampled = upsample(signal, up);
    const double cutoff = 0.5 / static_cast<double>(std::max(up, down));
    const auto h = make_sinc_fir(63U, cutoff);
    auto filtered = fir_filter(h, upsampled);
    for (auto& v : filtered) {
        v *= static_cast<double>(up);
    }
    return downsample(filtered, down);
}

// ─── Fractional ───────────────────────────────────────────────────────────────

std::vector<double> resample_linear(const std::vector<double>& signal,
                                    std::size_t output_length) {
    if (output_length == 0U) {
        return {};
    }
    const std::size_t n = signal.size();
    if (n == 0U) {
        return std::vector<double>(output_length, 0.0);
    }
    std::vector<double> output(output_length);
    const double step = static_cast<double>(n - 1U) / static_cast<double>(output_length - 1U);
    for (std::size_t i = 0; i < output_length; ++i) {
        const double pos = static_cast<double>(i) * step;
        const std::size_t lo = static_cast<std::size_t>(pos);
        const std::size_t hi = std::min(lo + 1U, n - 1U);
        const double frac = pos - static_cast<double>(lo);
        output[i] = signal[lo] * (1.0 - frac) + signal[hi] * frac;
    }
    return output;
}

std::vector<double> resample_cubic(const std::vector<double>& signal,
                                   std::size_t output_length) {
    if (output_length == 0U) {
        return {};
    }
    const std::size_t n = signal.size();
    if (n == 0U) {
        return std::vector<double>(output_length, 0.0);
    }
    std::vector<double> output(output_length);
    const double step = static_cast<double>(n - 1U) / static_cast<double>(output_length - 1U);
    for (std::size_t i = 0; i < output_length; ++i) {
        const double pos = static_cast<double>(i) * step;
        const std::ptrdiff_t x1 = static_cast<std::ptrdiff_t>(pos);
        const std::ptrdiff_t x0 = x1 - 1;
        const std::ptrdiff_t x2 = x1 + 1;
        const std::ptrdiff_t x3 = x1 + 2;
        const double t = pos - static_cast<double>(x1);
        const double p0 = clamp_index(signal, x0);
        const double p1 = clamp_index(signal, x1);
        const double p2 = clamp_index(signal, x2);
        const double p3 = clamp_index(signal, x3);
        // Catmull-Rom
        output[i] = 0.5 * ((2.0 * p1) +
                            (-p0 + p2) * t +
                            (2.0 * p0 - 5.0 * p1 + 4.0 * p2 - p3) * t * t +
                            (-p0 + 3.0 * p1 - 3.0 * p2 + p3) * t * t * t);
    }
    return output;
}

std::vector<double> resample_sinc(const std::vector<double>& signal,
                                  std::size_t output_length,
                                  std::size_t num_taps) {
    if (output_length == 0U) {
        return {};
    }
    const std::size_t n = signal.size();
    if (n == 0U) {
        return std::vector<double>(output_length, 0.0);
    }
    if (num_taps == 0U) {
        throw std::invalid_argument("resample_sinc: num_taps must be > 0");
    }
    if ((num_taps % 2U) == 0U) {
        ++num_taps;
    }
    const std::size_t half = num_taps / 2U;
    const double ratio = static_cast<double>(n) / static_cast<double>(output_length);
    const double cutoff = std::min(1.0 / (2.0 * ratio), 0.5);  // normalised to 1

    std::vector<double> output(output_length, 0.0);
    for (std::size_t i = 0; i < output_length; ++i) {
        const double pos = static_cast<double>(i) * ratio;
        for (std::size_t k = 0; k < num_taps; ++k) {
            const double t = pos - (static_cast<double>(static_cast<std::ptrdiff_t>(k) -
                                                         static_cast<std::ptrdiff_t>(half)));
            const std::ptrdiff_t src = static_cast<std::ptrdiff_t>(std::round(t));
            if (src < 0 || static_cast<std::size_t>(src) >= n) {
                continue;
            }
            const double arg = t - static_cast<double>(src);
            const double win = hann(k, num_taps);
            output[i] += signal[static_cast<std::size_t>(src)] * sinc(arg / (2.0 * cutoff)) * win;
        }
        output[i] /= ratio;
    }
    return output;
}

std::vector<double> resample_rate(const std::vector<double>& signal,
                                  double src_rate_hz,
                                  double dst_rate_hz,
                                  std::size_t num_taps) {
    if (src_rate_hz <= 0.0 || dst_rate_hz <= 0.0) {
        throw std::invalid_argument("resample_rate: sample rates must be > 0");
    }
    const double ratio = dst_rate_hz / src_rate_hz;
    const std::size_t output_length = static_cast<std::size_t>(
        std::ceil(static_cast<double>(signal.size()) * ratio));
    return resample_sinc(signal, output_length, num_taps);
}

std::vector<std::vector<double>> polyphase_decompose(const std::vector<double>& h,
                                                     std::size_t num_phases) {
    if (num_phases == 0U) {
        throw std::invalid_argument("polyphase_decompose: num_phases must be > 0");
    }
    std::vector<std::vector<double>> phases(num_phases);
    for (std::size_t i = 0; i < h.size(); ++i) {
        phases[i % num_phases].push_back(h[i]);
    }
    return phases;
}

}  // namespace resampling
}  // namespace sp
