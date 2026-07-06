#include "wavelets.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <stdexcept>

namespace sp {
namespace wavelets {

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kSqrt2 = 1.41421356237309504880;

// Quadrature mirror filter: hi[n] = (-1)^n * lo[L-1-n]
std::vector<double> qmf(const std::vector<double>& lo) {
    const std::size_t n = lo.size();
    std::vector<double> hi(n);
    for (std::size_t i = 0; i < n; ++i) {
        const double sign = ((i % 2U) == 0U) ? 1.0 : -1.0;
        hi[i] = sign * lo[n - 1U - i];
    }
    return hi;
}

// Down-sample and convolve (per-phase convolution for DWT).
// Equivalent to: convolve(signal, filter) then keep every other sample.
std::vector<double> dwt_conv(const std::vector<double>& signal,
                             const std::vector<double>& h) {
    const std::size_t n = signal.size();
    const std::size_t len_h = h.size();
    if (n == 0U || len_h == 0U) {
        return {};
    }
    // Periodic extension (circulant) for nice boundary behaviour
    const std::size_t out_len = (n + len_h - 1U) / 2U;
    std::vector<double> output;
    output.reserve(out_len);
    for (std::size_t i = 0; i < n + len_h - 1U; i += 2U) {
        double sum = 0.0;
        for (std::size_t k = 0; k < len_h; ++k) {
            const std::ptrdiff_t idx = static_cast<std::ptrdiff_t>(i) -
                                       static_cast<std::ptrdiff_t>(k);
            if (idx >= 0 && static_cast<std::size_t>(idx) < n) {
                sum += signal[static_cast<std::size_t>(idx)] * h[k];
            }
        }
        output.push_back(sum);
    }
    return output;
}

// Up-sample and convolve for IDWT (per-phase reconstruction).
std::vector<double> idwt_conv(const std::vector<double>& coeffs,
                               const std::vector<double>& h,
                               std::size_t output_length) {
    const std::size_t n = coeffs.size();
    const std::size_t len_h = h.size();
    // Upsample: insert zeros
    std::vector<double> upsampled(n * 2U, 0.0);
    for (std::size_t i = 0; i < n; ++i) {
        upsampled[i * 2U] = coeffs[i];
    }
    // Convolve
    const std::size_t conv_len = upsampled.size() + len_h - 1U;
    std::vector<double> convolved(conv_len, 0.0);
    for (std::size_t i = 0; i < upsampled.size(); ++i) {
        for (std::size_t k = 0; k < len_h; ++k) {
            convolved[i + k] += upsampled[i] * h[k];
        }
    }
    // Trim to output_length
    const std::size_t offset = len_h - 1U;
    std::vector<double> output(output_length, 0.0);
    for (std::size_t i = 0; i < output_length && (i + offset) < convolved.size(); ++i) {
        output[i] = convolved[i + offset];
    }
    return output;
}

}  // namespace

// ─── Filter banks ─────────────────────────────────────────────────────────────

std::vector<double> wavelet_filter_lo(WaveletFamily family) {
    switch (family) {
        case WaveletFamily::Haar:
            return {1.0 / kSqrt2, 1.0 / kSqrt2};

        case WaveletFamily::Daubechies2:
            return {(1.0 + std::sqrt(3.0)) / (4.0 * kSqrt2),
                    (3.0 + std::sqrt(3.0)) / (4.0 * kSqrt2),
                    (3.0 - std::sqrt(3.0)) / (4.0 * kSqrt2),
                    (1.0 - std::sqrt(3.0)) / (4.0 * kSqrt2)};

        case WaveletFamily::Daubechies4:
            return {
                0.32580343,  0.56076545,  0.35631291, -0.09350161,
               -0.13263945,  0.06647612,  0.03216300, -0.01013089
            };

        case WaveletFamily::Symlet2:
            return {-0.12940952, -0.22414387, 0.83651630, -0.48296291};

        case WaveletFamily::Coiflet1:
            return {
               -0.01565573,  0.28390935,  0.58920970,
                0.28390935, -0.01565573, -0.05451015
            };

        case WaveletFamily::BiorSpline13:
            return {
               -0.08838834, 0.08838834, 0.69587998,
                0.69587998, 0.08838834,-0.08838834
            };
    }
    return {};  // unreachable
}

std::vector<double> wavelet_filter_hi(WaveletFamily family) {
    return qmf(wavelet_filter_lo(family));
}

// ─── DWT ──────────────────────────────────────────────────────────────────────

DwtCoefficients dwt(const std::vector<double>& signal, WaveletFamily family) {
    const auto lo = wavelet_filter_lo(family);
    const auto hi = wavelet_filter_hi(family);
    DwtCoefficients result;
    result.approx = dwt_conv(signal, lo);
    result.detail = dwt_conv(signal, hi);
    return result;
}

std::vector<double> idwt(const DwtCoefficients& coeffs, WaveletFamily family) {
    const auto lo = wavelet_filter_lo(family);
    const auto hi = wavelet_filter_hi(family);

    // Synthesis filters are time-reverses of the analysis filters
    auto lo_syn = lo;
    auto hi_syn = hi;
    std::reverse(lo_syn.begin(), lo_syn.end());
    std::reverse(hi_syn.begin(), hi_syn.end());

    const std::size_t out_len = coeffs.approx.size() * 2U;
    const auto approx_rec = idwt_conv(coeffs.approx, lo_syn, out_len);
    const auto detail_rec = idwt_conv(coeffs.detail, hi_syn, out_len);

    std::vector<double> output(out_len, 0.0);
    for (std::size_t i = 0; i < out_len; ++i) {
        output[i] = approx_rec[i] + detail_rec[i];
    }
    return output;
}

std::vector<std::vector<double>> wavedec(const std::vector<double>& signal,
                                         WaveletFamily family,
                                         std::size_t levels) {
    if (levels == 0U) {
        throw std::invalid_argument("wavedec: levels must be > 0");
    }
    std::vector<std::vector<double>> result(levels + 1U);
    std::vector<double> current = signal;
    for (std::size_t level = 0; level < levels; ++level) {
        const auto c = dwt(current, family);
        result[levels - level] = c.detail;
        current = c.approx;
    }
    result[0] = current;  // coarsest approximation
    return result;
}

std::vector<double> waverec(const std::vector<std::vector<double>>& coeffs,
                            WaveletFamily family) {
    if (coeffs.empty()) {
        return {};
    }
    const std::size_t levels = coeffs.size() - 1U;
    std::vector<double> current = coeffs[0];
    for (std::size_t level = 0; level < levels; ++level) {
        DwtCoefficients c;
        c.approx = current;
        c.detail = coeffs[level + 1U];
        current = idwt(c, family);
        // Trim to expected length (2 * prev approx size)
        const std::size_t expected = coeffs[level + 1U].size() * 2U;
        if (current.size() > expected) {
            current.resize(expected);
        }
    }
    return current;
}

// ─── CWT (Morlet) ─────────────────────────────────────────────────────────────

std::vector<double> cwt_scales(double scale_min, double scale_max, std::size_t num_scales) {
    if (num_scales < 2U) {
        throw std::invalid_argument("cwt_scales: num_scales must be >= 2");
    }
    if (scale_min <= 0.0 || scale_max <= scale_min) {
        throw std::invalid_argument("cwt_scales: need 0 < scale_min < scale_max");
    }
    std::vector<double> scales(num_scales);
    const double log_min = std::log(scale_min);
    const double log_max = std::log(scale_max);
    for (std::size_t i = 0; i < num_scales; ++i) {
        scales[i] = std::exp(log_min + static_cast<double>(i) *
                             (log_max - log_min) / static_cast<double>(num_scales - 1U));
    }
    return scales;
}

std::vector<std::vector<std::complex<double>>> cwt_morlet(
    const std::vector<double>& signal,
    const std::vector<double>& scales,
    double omega0)
{
    const std::size_t n = signal.size();
    const std::size_t num_scales = scales.size();
    std::vector<std::vector<std::complex<double>>> result(
        num_scales, std::vector<std::complex<double>>(n, {0.0, 0.0}));

    // Normalisation constant
    const double norm = std::pow(kPi, -0.25);

    for (std::size_t s = 0; s < num_scales; ++s) {
        const double scale = scales[s];
        const double inv_sqrt_scale = 1.0 / std::sqrt(scale);
        // Half-window in samples (truncate after 5 e-folding times)
        const std::ptrdiff_t half_win = static_cast<std::ptrdiff_t>(5.0 * scale);

        for (std::size_t t = 0; t < n; ++t) {
            std::complex<double> sum = {0.0, 0.0};
            for (std::ptrdiff_t tau = -half_win; tau <= half_win; ++tau) {
                const std::ptrdiff_t src = static_cast<std::ptrdiff_t>(t) + tau;
                if (src < 0 || static_cast<std::size_t>(src) >= n) {
                    continue;
                }
                const double u = static_cast<double>(tau) / scale;
                const double envelope = std::exp(-0.5 * u * u);
                const double real_part = envelope * std::cos(omega0 * u);
                const double imag_part = envelope * std::sin(omega0 * u);
                const double x = signal[static_cast<std::size_t>(src)];
                sum += x * std::complex<double>(real_part, imag_part);
            }
            result[s][t] = inv_sqrt_scale * norm * sum;
        }
    }
    return result;
}

// ─── Denoising ────────────────────────────────────────────────────────────────

double universal_threshold(const std::vector<double>& detail_coeffs) {
    const std::size_t n = detail_coeffs.size();
    if (n == 0U) {
        return 0.0;
    }
    // Robust noise estimate via median absolute deviation / 0.6745
    std::vector<double> abs_coeffs(n);
    for (std::size_t i = 0; i < n; ++i) {
        abs_coeffs[i] = std::abs(detail_coeffs[i]);
    }
    std::sort(abs_coeffs.begin(), abs_coeffs.end());
    const double median = abs_coeffs[n / 2U];
    const double sigma = median / 0.6745;
    return sigma * std::sqrt(2.0 * std::log(static_cast<double>(n)));
}

std::vector<double> threshold(const std::vector<double>& coeffs,
                              double lambda,
                              ThresholdRule rule) {
    std::vector<double> output(coeffs.size());
    for (std::size_t i = 0; i < coeffs.size(); ++i) {
        const double v = coeffs[i];
        if (rule == ThresholdRule::Hard) {
            output[i] = (std::abs(v) >= lambda) ? v : 0.0;
        } else {
            // Soft
            if (std::abs(v) <= lambda) {
                output[i] = 0.0;
            } else {
                output[i] = (v > 0.0 ? 1.0 : -1.0) * (std::abs(v) - lambda);
            }
        }
    }
    return output;
}

std::vector<double> denoise(const std::vector<double>& signal,
                            WaveletFamily family,
                            std::size_t levels) {
    auto coeffs = wavedec(signal, family, levels);
    // Threshold all detail levels
    for (std::size_t level = 1U; level <= levels; ++level) {
        const double lambda = universal_threshold(coeffs[level]);
        coeffs[level] = threshold(coeffs[level], lambda, ThresholdRule::Soft);
    }
    auto reconstructed = waverec(coeffs, family);
    // Trim to original length
    if (reconstructed.size() > signal.size()) {
        reconstructed.resize(signal.size());
    }
    return reconstructed;
}

// ─── Utility ─────────────────────────────────────────────────────────────────

std::vector<std::vector<double>> scalogram(
    const std::vector<std::vector<std::complex<double>>>& cwt_output)
{
    std::vector<std::vector<double>> result(cwt_output.size());
    for (std::size_t s = 0; s < cwt_output.size(); ++s) {
        result[s].resize(cwt_output[s].size());
        for (std::size_t t = 0; t < cwt_output[s].size(); ++t) {
            result[s][t] = std::abs(cwt_output[s][t]);
        }
    }
    return result;
}

std::vector<double> pad_to_power_of_two(const std::vector<double>& signal) {
    const std::size_t n = signal.size();
    if (n == 0U) {
        return {};
    }
    std::size_t power = 1U;
    while (power < n) {
        power <<= 1U;
    }
    std::vector<double> padded = signal;
    padded.resize(power, 0.0);
    return padded;
}

}  // namespace wavelets
}  // namespace sp
