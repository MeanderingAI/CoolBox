#ifndef COOLBOX__LIBRARIES_PACKAGES_SP_WAVELETS_HEADERS_WAVELETS_HPP
#define COOLBOX__LIBRARIES_PACKAGES_SP_WAVELETS_HEADERS_WAVELETS_HPP

#include <complex>
#include <cstddef>
#include <string>
#include <vector>

namespace sp {
namespace wavelets {

// ─── Wavelet family selection ─────────────────────────────────────────────────

enum class WaveletFamily {
    Haar,        // Haar (db1)
    Daubechies2, // db2  (4-tap)
    Daubechies4, // db4  (8-tap)
    Symlet2,     // sym2 (4-tap, nearly symmetric)
    Coiflet1,    // coif1 (6-tap)
    BiorSpline13 // Biorthogonal 1.3 (6-tap analysis / 6-tap synthesis)
};

// Returns the low-pass decomposition filter for the chosen wavelet family.
std::vector<double> wavelet_filter_lo(WaveletFamily family);

// Returns the high-pass decomposition filter (QMF of lo_d).
std::vector<double> wavelet_filter_hi(WaveletFamily family);

// ─── Discrete Wavelet Transform (DWT) ────────────────────────────────────────

struct DwtCoefficients {
    std::vector<double> approx;   // approximation (low-pass) coefficients
    std::vector<double> detail;   // detail (high-pass) coefficients
};

// Single-level forward DWT: returns approx + detail at one scale.
DwtCoefficients dwt(const std::vector<double>& signal, WaveletFamily family);

// Single-level inverse DWT: reconstructs signal from approx + detail.
std::vector<double> idwt(const DwtCoefficients& coeffs, WaveletFamily family);

// Multi-level forward DWT (wavelet packet decomposition up to `levels` scales).
// output[0] is the coarsest approximation; output[1..levels] are detail levels.
std::vector<std::vector<double>> wavedec(const std::vector<double>& signal,
                                         WaveletFamily family,
                                         std::size_t levels);

// Multi-level inverse DWT: reconstructs from the multi-level decomposition.
std::vector<double> waverec(const std::vector<std::vector<double>>& coeffs,
                            WaveletFamily family);

// ─── Continuous Wavelet Transform (CWT) ──────────────────────────────────────

// Scales used to evaluate the CWT (geometric sequence from scale_min to scale_max).
std::vector<double> cwt_scales(double scale_min, double scale_max, std::size_t num_scales);

// Morlet CWT for real signals.
// Returns a num_scales × signal.size() matrix of complex scalogram values.
// `omega0` is the Morlet wavelet central frequency (default 6.0).
std::vector<std::vector<std::complex<double>>> cwt_morlet(
    const std::vector<double>& signal,
    const std::vector<double>& scales,
    double omega0 = 6.0);

// ─── Denoising ────────────────────────────────────────────────────────────────

enum class ThresholdRule { Hard, Soft };

// Universal (VisuShrink) threshold value: σ * sqrt(2 * log(N)).
double universal_threshold(const std::vector<double>& detail_coeffs);

// Apply hard or soft thresholding to coefficients.
std::vector<double> threshold(const std::vector<double>& coeffs,
                              double lambda,
                              ThresholdRule rule = ThresholdRule::Soft);

// Denoise via multi-level DWT + universal soft-thresholding + IDWT.
std::vector<double> denoise(const std::vector<double>& signal,
                            WaveletFamily family,
                            std::size_t levels);

// ─── Utility ─────────────────────────────────────────────────────────────────

// Compute the scalogram magnitude (|CWT|) as a real matrix.
std::vector<std::vector<double>> scalogram(
    const std::vector<std::vector<std::complex<double>>>& cwt_output);

// Pad a signal to the next power of two using zero-padding (needed for some DWT impls).
std::vector<double> pad_to_power_of_two(const std::vector<double>& signal);

}  // namespace wavelets
}  // namespace sp

#endif  // COOLBOX__LIBRARIES_PACKAGES_SP_WAVELETS_HEADERS_WAVELETS_HPP
