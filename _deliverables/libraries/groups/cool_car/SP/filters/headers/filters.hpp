#ifndef COOLBOX__LIBRARIES_PACKAGES_SP_FILTERS_HEADERS_FILTERS_HPP
#define COOLBOX__LIBRARIES_PACKAGES_SP_FILTERS_HEADERS_FILTERS_HPP

#include <cstddef>
#include <vector>

namespace sp {
namespace filters {

// ─── Filter coefficient containers ────────────────────────────────────────────

struct FirCoefficients {
    std::vector<double> b;  // feed-forward (numerator) taps
};

struct IirCoefficients {
    std::vector<double> b;  // feed-forward (numerator) taps
    std::vector<double> a;  // feed-back    (denominator) taps, a[0] is normalised to 1
};

// ─── FIR filter design ────────────────────────────────────────────────────────

// Windowed-sinc FIR low-pass filter.
// cutoff_normalised: fraction of Nyquist, (0, 1).
FirCoefficients fir_lowpass(std::size_t order, double cutoff_normalised);

// Windowed-sinc FIR high-pass filter.
FirCoefficients fir_highpass(std::size_t order, double cutoff_normalised);

// Windowed-sinc FIR band-pass filter.
// low_cutoff, high_cutoff: fractions of Nyquist, low < high.
FirCoefficients fir_bandpass(std::size_t order,
                             double low_cutoff_normalised,
                             double high_cutoff_normalised);

// Windowed-sinc FIR band-stop (notch) filter.
FirCoefficients fir_bandstop(std::size_t order,
                             double low_cutoff_normalised,
                             double high_cutoff_normalised);

// ─── IIR filter design ────────────────────────────────────────────────────────

// First-order Butterworth low-pass filter.
// cutoff_normalised: fraction of Nyquist, (0, 1).
IirCoefficients butterworth_lowpass(std::size_t order, double cutoff_normalised);

// First-order Butterworth high-pass filter.
IirCoefficients butterworth_highpass(std::size_t order, double cutoff_normalised);

// Butterworth band-pass filter.
IirCoefficients butterworth_bandpass(std::size_t order,
                                     double low_cutoff_normalised,
                                     double high_cutoff_normalised);

// Type-I Chebyshev low-pass filter.
// ripple_db: pass-band ripple in dB (positive value).
IirCoefficients chebyshev1_lowpass(std::size_t order,
                                   double cutoff_normalised,
                                   double ripple_db);

// Type-I Chebyshev high-pass filter.
IirCoefficients chebyshev1_highpass(std::size_t order,
                                    double cutoff_normalised,
                                    double ripple_db);

// Simple biquad notch (band-stop) IIR filter.
// centre_normalised: fraction of Nyquist.
// bandwidth_normalised: 3-dB bandwidth as fraction of Nyquist.
IirCoefficients biquad_notch(double centre_normalised, double bandwidth_normalised);

// ─── Filter application ───────────────────────────────────────────────────────

// Apply a FIR filter to a signal (linear-phase, no latency compensation).
std::vector<double> apply_fir(const FirCoefficients& coeffs,
                              const std::vector<double>& signal);

// Apply an IIR filter to a signal using Direct Form II transposed.
std::vector<double> apply_iir(const IirCoefficients& coeffs,
                               const std::vector<double>& signal);

// Zero-phase (forward–backward) IIR filtering using apply_iir in both directions.
std::vector<double> filtfilt(const IirCoefficients& coeffs,
                              const std::vector<double>& signal);

// ─── Utility / analysis ───────────────────────────────────────────────────────

// Compute frequency-response magnitude (linear) at normalised frequencies [0, 1).
std::vector<double> frequency_response(const IirCoefficients& coeffs,
                                       std::size_t num_points);

// Compute group delay (samples) of an IIR filter at normalised frequencies [0, 1).
std::vector<double> group_delay(const IirCoefficients& coeffs,
                                std::size_t num_points);

// Compute the impulse response of a FIR filter (simply returns b taps).
std::vector<double> impulse_response(const FirCoefficients& coeffs, std::size_t length);

// Moving-average (box) filter — equivalent to a uniform FIR with all taps = 1/window.
std::vector<double> moving_average(const std::vector<double>& signal, std::size_t window);

// Median filter.
std::vector<double> median_filter(const std::vector<double>& signal, std::size_t window);

// Savitzky-Golay smoothing filter.
// window must be odd and > poly_order.
std::vector<double> savitzky_golay(const std::vector<double>& signal,
                                   std::size_t window,
                                   std::size_t poly_order);

}  // namespace filters
}  // namespace sp

#endif  // COOLBOX__LIBRARIES_PACKAGES_SP_FILTERS_HEADERS_FILTERS_HPP
