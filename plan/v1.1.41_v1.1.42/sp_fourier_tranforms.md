# SP Fourier Tranforms

## Summary
- Added a new `SP` package category under `_libraries/packages/SP`.
- Added a new library named `fourier_tranforms` under `_libraries/packages/SP/fourier_tranforms`.
- Implemented a broad Fourier-transform utility surface rather than a single FFT entry point.

## Implemented Variants
- Discrete Fourier transform: `dft`, `idft`
- Fast Fourier transform: `fft`, `ifft`
- Real-signal transforms: `real_fft`, `inverse_real_fft`
- Two-dimensional variants: `dft2d`, `idft2d`, `fft2d`, `ifft2d`
- Short-time Fourier transform: `stft`
- Inverse short-time Fourier transform: `inverse_stft`
- Spectrum helpers: `frequency_bins`, `magnitude_spectrum`, `power_spectrum`, `fft_shift`
- Additional spectrum helpers: `phase_spectrum`, `cross_power_spectrum`
- Related transform families: `dct`, `idct`, `dst`, `idst`
- Hartley family: `hartley_transform`, `inverse_hartley_transform`
- Frequency-domain utility: `circular_convolution`
- Additional convolution and correlation utilities: `linear_convolution`, `autocorrelation_via_fft`

## Build Integration
- Added `_libraries/packages/SP/CMakeLists.txt` so the new category behaves like the other top-level packages.
- Added `_libraries/packages/SP/fourier_tranforms/CMakeLists.txt` with library and test targets.
- Updated `_libraries/CMakeLists.txt` so the root build includes the new `SP` category.
- Linked `fourier_tranforms` against the `mytrix` library and moved matrix-valued Fourier storage onto `matrix::DenseMatrix`.
- Converted the Fourier test target into the first consumer of `TOOLS/tyst_framework`, linking it through `tyst_framework_main`.

## Result
- The repository now has a signal-processing package category and a reusable Fourier transform library named `fourier_tranforms`.
- The DST/IDST pair now uses the correct inverse normalization for the library's DST-II-style forward transform, so the round-trip test passes under `tyst_framework`.