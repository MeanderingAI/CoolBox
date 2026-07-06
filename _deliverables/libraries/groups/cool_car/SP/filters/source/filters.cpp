#include "filters.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <complex>
#include <numeric>
#include <stdexcept>

namespace sp {
namespace filters {

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kTwoPi = 2.0 * kPi;

// Hann window
std::vector<double> hann_window(std::size_t length) {
    std::vector<double> w(length);
    for (std::size_t i = 0; i < length; ++i) {
        w[i] = 0.5 * (1.0 - std::cos(kTwoPi * static_cast<double>(i) /
                                       static_cast<double>(length - 1U)));
    }
    return w;
}

// Sinc function: sin(pi*x) / (pi*x) with sinc(0) = 1
double sinc(double x) {
    if (std::abs(x) < 1e-12) {
        return 1.0;
    }
    return std::sin(kPi * x) / (kPi * x);
}

// Bilinear transform pre-warped cutoff from digital normalised frequency [0,1)
// to analogue radian frequency.
double prewarp(double normalised) {
    return 2.0 * std::tan(kPi * normalised * 0.5);
}

// Reverse a vector in place
template <typename T>
void reverse_inplace(std::vector<T>& v) {
    std::reverse(v.begin(), v.end());
}

// Polynomial multiplication: multiply two coefficient polynomials
std::vector<double> poly_mult(const std::vector<double>& a, const std::vector<double>& b) {
    if (a.empty() || b.empty()) {
        return {};
    }
    std::vector<double> result(a.size() + b.size() - 1U, 0.0);
    for (std::size_t i = 0; i < a.size(); ++i) {
        for (std::size_t j = 0; j < b.size(); ++j) {
            result[i + j] += a[i] * b[j];
        }
    }
    return result;
}

// Cascade first-order Butterworth sections into a single IIR.
// Each section has: b = {1, 1}, a = {1+wc, -(1-wc)/...}  — see butterworth_lowpass.
// Here we just expand the product of the per-section numerator/denominator polynomials.

}  // namespace

// ─── FIR design ───────────────────────────────────────────────────────────────

FirCoefficients fir_lowpass(std::size_t order, double cutoff_normalised) {
    if (order == 0U) {
        throw std::invalid_argument("FIR order must be > 0");
    }
    const std::size_t length = order + 1U;
    const auto window = hann_window(length);
    std::vector<double> b(length);
    const double fc = cutoff_normalised;
    const double centre = static_cast<double>(order) / 2.0;
    for (std::size_t i = 0; i < length; ++i) {
        const double t = static_cast<double>(i) - centre;
        b[i] = 2.0 * fc * sinc(2.0 * fc * t) * window[i];
    }
    return {b};
}

FirCoefficients fir_highpass(std::size_t order, double cutoff_normalised) {
    auto lp = fir_lowpass(order, cutoff_normalised);
    // Spectral inversion: negate and add delta at centre
    const std::size_t centre = order / 2U;
    for (auto& tap : lp.b) {
        tap = -tap;
    }
    lp.b[centre] += 1.0;
    return lp;
}

FirCoefficients fir_bandpass(std::size_t order,
                             double low_cutoff_normalised,
                             double high_cutoff_normalised) {
    if (low_cutoff_normalised >= high_cutoff_normalised) {
        throw std::invalid_argument("low cutoff must be less than high cutoff");
    }
    auto lp_high = fir_lowpass(order, high_cutoff_normalised);
    auto lp_low  = fir_lowpass(order, low_cutoff_normalised);
    for (std::size_t i = 0; i < lp_high.b.size(); ++i) {
        lp_high.b[i] -= lp_low.b[i];
    }
    return lp_high;
}

FirCoefficients fir_bandstop(std::size_t order,
                             double low_cutoff_normalised,
                             double high_cutoff_normalised) {
    auto bp = fir_bandpass(order, low_cutoff_normalised, high_cutoff_normalised);
    // Spectral inversion
    const std::size_t centre = order / 2U;
    for (auto& tap : bp.b) {
        tap = -tap;
    }
    bp.b[centre] += 1.0;
    return bp;
}

// ─── IIR design (bilinear transform Butterworth) ──────────────────────────────

IirCoefficients butterworth_lowpass(std::size_t order, double cutoff_normalised) {
    if (order == 0U) {
        throw std::invalid_argument("IIR order must be > 0");
    }
    const double wc = prewarp(cutoff_normalised);

    // Build cascaded first-order and second-order biquad sections.
    // For a 1st-order Butterworth LP section (analogue pole at -wc):
    //   H(z) = (wc/(wc+2)) * (1 + z^-1) / (1 - ((2-wc)/(2+wc)) z^-1)
    // We cascade `order` first-order sections (approximate for simplicity).
    IirCoefficients result;
    result.b = {1.0};
    result.a = {1.0};

    const double gain_section = wc / (wc + 2.0);
    const double pole = (2.0 - wc) / (2.0 + wc);

    for (std::size_t i = 0; i < order; ++i) {
        std::vector<double> b_sec = {gain_section, gain_section};
        std::vector<double> a_sec = {1.0, -pole};
        result.b = poly_mult(result.b, b_sec);
        result.a = poly_mult(result.a, a_sec);
    }
    return result;
}

IirCoefficients butterworth_highpass(std::size_t order, double cutoff_normalised) {
    if (order == 0U) {
        throw std::invalid_argument("IIR order must be > 0");
    }
    const double wc = prewarp(cutoff_normalised);
    // 1st-order HP section via bilinear:
    //   H(z) = (1/(1 + wc/2)) * (1 - z^-1) / (1 - ((1 - wc/2)/(1 + wc/2)) z^-1)
    const double k = 2.0 / wc;  // substitute s -> (z-1)/(z+1) scaled
    const double gain_section = k / (k + 1.0);
    const double pole = (k - 1.0) / (k + 1.0);

    IirCoefficients result;
    result.b = {1.0};
    result.a = {1.0};

    for (std::size_t i = 0; i < order; ++i) {
        std::vector<double> b_sec = {gain_section, -gain_section};
        std::vector<double> a_sec = {1.0, -pole};
        result.b = poly_mult(result.b, b_sec);
        result.a = poly_mult(result.a, a_sec);
    }
    return result;
}

IirCoefficients butterworth_bandpass(std::size_t order,
                                     double low_cutoff_normalised,
                                     double high_cutoff_normalised) {
    // Implemented as a cascade of LP and HP Butterworth filters.
    auto lp = butterworth_lowpass(order, high_cutoff_normalised);
    auto hp = butterworth_highpass(order, low_cutoff_normalised);
    IirCoefficients result;
    result.b = poly_mult(lp.b, hp.b);
    result.a = poly_mult(lp.a, hp.a);
    return result;
}

IirCoefficients chebyshev1_lowpass(std::size_t order,
                                   double cutoff_normalised,
                                   double ripple_db) {
    if (order == 0U) {
        throw std::invalid_argument("IIR order must be > 0");
    }
    if (ripple_db <= 0.0) {
        throw std::invalid_argument("ripple_db must be positive");
    }
    // ε from pass-band ripple
    const double epsilon = std::sqrt(std::pow(10.0, ripple_db / 10.0) - 1.0);
    const double wc = prewarp(cutoff_normalised);
    // Scale epsilon into the pole radius
    const double radius = wc / std::pow(epsilon, 1.0 / static_cast<double>(order));
    const double gain_section = radius / (radius + 2.0);
    const double pole = (2.0 - radius) / (2.0 + radius);

    IirCoefficients result;
    result.b = {1.0};
    result.a = {1.0};
    for (std::size_t i = 0; i < order; ++i) {
        std::vector<double> b_sec = {gain_section, gain_section};
        std::vector<double> a_sec = {1.0, -pole};
        result.b = poly_mult(result.b, b_sec);
        result.a = poly_mult(result.a, a_sec);
    }
    return result;
}

IirCoefficients chebyshev1_highpass(std::size_t order,
                                    double cutoff_normalised,
                                    double ripple_db) {
    if (order == 0U) {
        throw std::invalid_argument("IIR order must be > 0");
    }
    if (ripple_db <= 0.0) {
        throw std::invalid_argument("ripple_db must be positive");
    }
    const double epsilon = std::sqrt(std::pow(10.0, ripple_db / 10.0) - 1.0);
    const double wc = prewarp(cutoff_normalised);
    const double radius = wc * std::pow(epsilon, 1.0 / static_cast<double>(order));
    const double k = 2.0 / radius;
    const double gain_section = k / (k + 1.0);
    const double pole = (k - 1.0) / (k + 1.0);

    IirCoefficients result;
    result.b = {1.0};
    result.a = {1.0};
    for (std::size_t i = 0; i < order; ++i) {
        std::vector<double> b_sec = {gain_section, -gain_section};
        std::vector<double> a_sec = {1.0, -pole};
        result.b = poly_mult(result.b, b_sec);
        result.a = poly_mult(result.a, a_sec);
    }
    return result;
}

IirCoefficients biquad_notch(double centre_normalised, double bandwidth_normalised) {
    const double w0 = kTwoPi * centre_normalised;
    const double bw = kPi * bandwidth_normalised;
    const double r = 1.0 - bw;          // pole radius ≈ 1 - π·BW/2 for narrow notch
    const double cos_w0 = std::cos(w0);
    // Zeros on the unit circle at ±w0, poles at r·e^{±jw0}
    IirCoefficients coeff;
    coeff.b = {1.0, -2.0 * cos_w0, 1.0};
    coeff.a = {1.0, -2.0 * r * cos_w0, r * r};
    return coeff;
}

// ─── Filter application ───────────────────────────────────────────────────────

std::vector<double> apply_fir(const FirCoefficients& coeffs,
                              const std::vector<double>& signal) {
    const std::size_t n_taps  = coeffs.b.size();
    const std::size_t n_input = signal.size();
    std::vector<double> output(n_input, 0.0);
    for (std::size_t i = 0; i < n_input; ++i) {
        for (std::size_t k = 0; k < n_taps && k <= i; ++k) {
            output[i] += coeffs.b[k] * signal[i - k];
        }
    }
    return output;
}

std::vector<double> apply_iir(const IirCoefficients& coeffs,
                               const std::vector<double>& signal) {
    const std::size_t n_b = coeffs.b.size();
    const std::size_t n_a = coeffs.a.size();
    const std::size_t n   = signal.size();

    std::vector<double> output(n, 0.0);
    std::vector<double> delay(std::max(n_b, n_a), 0.0);  // delay-line (Direct Form II T)

    for (std::size_t i = 0; i < n; ++i) {
        double w = signal[i];
        for (std::size_t k = 1; k < n_a && k < delay.size(); ++k) {
            w -= coeffs.a[k] * delay[k];
        }
        output[i] = coeffs.b[0] * w;
        for (std::size_t k = 1; k < n_b && k < delay.size(); ++k) {
            output[i] += coeffs.b[k] * delay[k];
        }
        // Shift delay line
        for (std::size_t k = delay.size() - 1U; k > 0U; --k) {
            delay[k] = delay[k - 1U];
        }
        delay[0] = w;
    }
    return output;
}

std::vector<double> filtfilt(const IirCoefficients& coeffs,
                              const std::vector<double>& signal) {
    auto forward = apply_iir(coeffs, signal);
    std::reverse(forward.begin(), forward.end());
    auto backward = apply_iir(coeffs, forward);
    std::reverse(backward.begin(), backward.end());
    return backward;
}

// ─── Utility ─────────────────────────────────────────────────────────────────

std::vector<double> frequency_response(const IirCoefficients& coeffs,
                                       std::size_t num_points) {
    std::vector<double> magnitude(num_points);
    for (std::size_t i = 0; i < num_points; ++i) {
        const double omega = kPi * static_cast<double>(i) / static_cast<double>(num_points);
        std::complex<double> jw(0.0, -omega);
        std::complex<double> num(0.0, 0.0);
        std::complex<double> den(0.0, 0.0);
        for (std::size_t k = 0; k < coeffs.b.size(); ++k) {
            num += coeffs.b[k] * std::exp(static_cast<double>(k) * jw);
        }
        for (std::size_t k = 0; k < coeffs.a.size(); ++k) {
            den += coeffs.a[k] * std::exp(static_cast<double>(k) * jw);
        }
        magnitude[i] = std::abs(num) / (std::abs(den) + 1e-300);
    }
    return magnitude;
}

std::vector<double> group_delay(const IirCoefficients& coeffs, std::size_t num_points) {
    // Approximate numerical group delay: -d(phase)/d(omega)
    std::vector<double> gd(num_points, 0.0);
    const double delta = kPi / static_cast<double>(num_points * 100U);
    for (std::size_t i = 0; i < num_points; ++i) {
        const double omega = kPi * static_cast<double>(i) / static_cast<double>(num_points);
        auto compute_phase = [&](double w) {
            std::complex<double> jw(0.0, -w);
            std::complex<double> num(0.0, 0.0);
            std::complex<double> den(0.0, 0.0);
            for (std::size_t k = 0; k < coeffs.b.size(); ++k) {
                num += coeffs.b[k] * std::exp(static_cast<double>(k) * jw);
            }
            for (std::size_t k = 0; k < coeffs.a.size(); ++k) {
                den += coeffs.a[k] * std::exp(static_cast<double>(k) * jw);
            }
            return std::arg(num / (den + std::complex<double>(1e-300, 0.0)));
        };
        gd[i] = -(compute_phase(omega + delta) - compute_phase(omega - delta)) / (2.0 * delta);
    }
    return gd;
}

std::vector<double> impulse_response(const FirCoefficients& coeffs, std::size_t length) {
    std::vector<double> output(length, 0.0);
    const std::size_t copy_len = std::min(length, coeffs.b.size());
    for (std::size_t i = 0; i < copy_len; ++i) {
        output[i] = coeffs.b[i];
    }
    return output;
}

std::vector<double> moving_average(const std::vector<double>& signal, std::size_t window) {
    if (window == 0U) {
        throw std::invalid_argument("window must be > 0");
    }
    const std::size_t n = signal.size();
    std::vector<double> output(n, 0.0);
    for (std::size_t i = 0; i < n; ++i) {
        double sum = 0.0;
        std::size_t count = 0U;
        for (std::size_t k = 0; k < window && k <= i; ++k) {
            sum += signal[i - k];
            ++count;
        }
        output[i] = sum / static_cast<double>(count);
    }
    return output;
}

std::vector<double> median_filter(const std::vector<double>& signal, std::size_t window) {
    if (window == 0U) {
        throw std::invalid_argument("window must be > 0");
    }
    const std::size_t half = window / 2U;
    const std::size_t n = signal.size();
    std::vector<double> output(n);
    std::vector<double> buffer;
    buffer.reserve(window);
    for (std::size_t i = 0; i < n; ++i) {
        buffer.clear();
        const std::size_t start = (i >= half) ? (i - half) : 0U;
        const std::size_t end   = std::min(i + half + 1U, n);
        for (std::size_t j = start; j < end; ++j) {
            buffer.push_back(signal[j]);
        }
        std::sort(buffer.begin(), buffer.end());
        output[i] = buffer[buffer.size() / 2U];
    }
    return output;
}

std::vector<double> savitzky_golay(const std::vector<double>& signal,
                                   std::size_t window,
                                   std::size_t poly_order) {
    if (window == 0U || (window % 2U) == 0U) {
        throw std::invalid_argument("window must be a positive odd number");
    }
    if (poly_order >= window) {
        throw std::invalid_argument("poly_order must be less than window");
    }
    // Compute Savitzky-Golay convolution coefficients for zeroth derivative.
    // We build the Vandermonde matrix for positions in [-half, half] and
    // solve the least-squares problem via normal equations.
    const std::size_t half = window / 2U;
    const std::size_t m    = poly_order + 1U;

    // Build Vandermonde: rows = positions, cols = powers 0..poly_order
    std::vector<std::vector<double>> A(window, std::vector<double>(m, 0.0));
    for (std::size_t i = 0; i < window; ++i) {
        double x = static_cast<double>(static_cast<std::ptrdiff_t>(i) -
                                       static_cast<std::ptrdiff_t>(half));
        double xk = 1.0;
        for (std::size_t k = 0; k < m; ++k) {
            A[i][k] = xk;
            xk *= x;
        }
    }

    // Normal equations: (A^T A) c = A^T e_centre, where e_centre = unit vector at half
    std::vector<std::vector<double>> AtA(m, std::vector<double>(m, 0.0));
    for (std::size_t i = 0; i < m; ++i) {
        for (std::size_t j = 0; j < m; ++j) {
            for (std::size_t k = 0; k < window; ++k) {
                AtA[i][j] += A[k][i] * A[k][j];
            }
        }
    }
    std::vector<double> Ate(m, 0.0);
    for (std::size_t i = 0; i < m; ++i) {
        Ate[i] = A[half][i];
    }

    // Gaussian elimination to solve AtA * c = Ate
    for (std::size_t col = 0; col < m; ++col) {
        // Pivot
        double max_val = std::abs(AtA[col][col]);
        std::size_t max_row = col;
        for (std::size_t row = col + 1U; row < m; ++row) {
            if (std::abs(AtA[row][col]) > max_val) {
                max_val = std::abs(AtA[row][col]);
                max_row = row;
            }
        }
        std::swap(AtA[col], AtA[max_row]);
        std::swap(Ate[col], Ate[max_row]);

        if (std::abs(AtA[col][col]) < 1e-14) {
            throw std::runtime_error("savitzky_golay: singular Vandermonde system");
        }
        const double pivot = AtA[col][col];
        for (std::size_t k = col; k < m; ++k) {
            AtA[col][k] /= pivot;
        }
        Ate[col] /= pivot;

        for (std::size_t row = 0; row < m; ++row) {
            if (row == col) {
                continue;
            }
            const double factor = AtA[row][col];
            for (std::size_t k = 0; k < m; ++k) {
                AtA[row][k] -= factor * AtA[col][k];
            }
            Ate[row] -= factor * Ate[col];
        }
    }
    // Ate now holds the coefficients c; the SG convolution kernel = A * c
    std::vector<double> kernel(window);
    for (std::size_t i = 0; i < window; ++i) {
        kernel[i] = 0.0;
        double xk = 1.0;
        double x = static_cast<double>(static_cast<std::ptrdiff_t>(i) -
                                       static_cast<std::ptrdiff_t>(half));
        for (std::size_t k = 0; k < m; ++k) {
            kernel[i] += Ate[k] * xk;
            xk *= x;
        }
    }

    const std::size_t n = signal.size();
    std::vector<double> output(n, 0.0);
    for (std::size_t i = 0; i < n; ++i) {
        double value = 0.0;
        for (std::size_t k = 0; k < window; ++k) {
            const std::ptrdiff_t src = static_cast<std::ptrdiff_t>(i) -
                                       static_cast<std::ptrdiff_t>(half) +
                                       static_cast<std::ptrdiff_t>(k);
            if (src >= 0 && static_cast<std::size_t>(src) < n) {
                value += kernel[k] * signal[static_cast<std::size_t>(src)];
            }
        }
        output[i] = value;
    }
    return output;
}

}  // namespace filters
}  // namespace sp
