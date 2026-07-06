#ifndef COOLBOX__LIBRARIES_PACKAGES_SP_RESAMPLING_HEADERS_RESAMPLING_HPP
#define COOLBOX__LIBRARIES_PACKAGES_SP_RESAMPLING_HEADERS_RESAMPLING_HPP

#include <cstddef>
#include <vector>

namespace sp {
namespace resampling {

// ─── Integer-ratio sample-rate conversion ─────────────────────────────────────

// Upsample by inserting `factor - 1` zeros between each sample.
std::vector<double> upsample(const std::vector<double>& signal, std::size_t factor);

// Downsample by keeping every `factor`-th sample.
std::vector<double> downsample(const std::vector<double>& signal, std::size_t factor);

// Anti-aliasing low-pass then downsample by integer `factor`.
// Applies a windowed-sinc FIR at cutoff 1/factor before decimating.
std::vector<double> decimate(const std::vector<double>& signal, std::size_t factor);

// Upsample by `factor`, low-pass filter at 1/factor, then output.
// Equivalent to ideal band-limited interpolation at integer ratio.
std::vector<double> interpolate(const std::vector<double>& signal, std::size_t factor);

// Rational sample-rate conversion: upsample by `up`, low-pass, downsample by `down`.
// Cutoff is automatically set to min(1/up, 1/down) (normalised Nyquist fraction).
std::vector<double> resample_rational(const std::vector<double>& signal,
                                      std::size_t up,
                                      std::size_t down);

// ─── Fractional / arbitrary resampling ───────────────────────────────────────

// Linear interpolation resampler — changes sample count to `output_length`.
std::vector<double> resample_linear(const std::vector<double>& signal,
                                    std::size_t output_length);

// Cubic (Catmull-Rom) interpolation resampler.
std::vector<double> resample_cubic(const std::vector<double>& signal,
                                   std::size_t output_length);

// Sinc-kernel interpolation resampler (windowed sinc).
// `num_taps` controls the quality (must be odd, e.g. 31 or 63).
std::vector<double> resample_sinc(const std::vector<double>& signal,
                                  std::size_t output_length,
                                  std::size_t num_taps = 63U);

// ─── Utility ─────────────────────────────────────────────────────────────────

// Convert a signal sampled at `src_rate_hz` to `dst_rate_hz` using sinc resampling.
std::vector<double> resample_rate(const std::vector<double>& signal,
                                  double src_rate_hz,
                                  double dst_rate_hz,
                                  std::size_t num_taps = 63U);

// Polyphase decomposition: split a filter `h` into `num_phases` polyphase sub-filters.
std::vector<std::vector<double>> polyphase_decompose(const std::vector<double>& h,
                                                     std::size_t num_phases);

}  // namespace resampling
}  // namespace sp

#endif  // COOLBOX__LIBRARIES_PACKAGES_SP_RESAMPLING_HEADERS_RESAMPLING_HPP
