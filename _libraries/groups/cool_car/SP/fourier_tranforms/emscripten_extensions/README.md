# Fourier Transforms - Emscripten Extensions

JavaScript/Emscripten bindings and extensions for the C++ `fourier_tranforms` library, enabling client-side signal processing in web browsers.

## Overview

This directory contains:

1. **fourier-transforms-bindings.mjs** - Low-level JavaScript bindings for FFT/DFT operations
2. **signal-processor.mjs** - High-level signal processing utilities and analysis functions

## Features

### Core Transforms
- **FFT (Fast Fourier Transform)** - Cooley-Tukey algorithm with automatic fallback to DFT
- **IFFT** - Inverse FFT for time-domain reconstruction
- **DFT/IDFT** - Discrete Fourier Transform for arbitrary sizes
- **Real FFT** - Optimized for real-valued signals
- **DCT/IDCT** - Discrete Cosine Transform

### Spectral Analysis
- Magnitude, power, and phase spectra
- Frequency bins computation
- FFT shifting (zero-frequency centering)
- Peak finding

### Signal Processing Utilities
- Window functions (Rectangular, Hann, Hamming, Blackman)
- Resampling with linear interpolation
- Downsampling/upsampling
- Moving average filtering
- RMS and crest factor computation
- Noise addition

### Waveform Generation
- Sine wave synthesis
- Cosine wave synthesis

## Usage Examples

### Basic FFT

```javascript
import * as FFT from './fourier-transforms-bindings.mjs';

// Create a complex signal
const signal = [
  FFT.complex(1, 0),
  FFT.complex(2, 0),
  FFT.complex(3, 0),
  FFT.complex(4, 0)
];

// Perform FFT
const spectrum = FFT.fft(signal);

// Get magnitude spectrum
const magnitude = FFT.magnitudeSpectrum(spectrum);
console.log('Magnitudes:', magnitude);

// Inverse transform
const reconstructed = FFT.ifft(spectrum);
```

### Real Signal Processing

```javascript
import * as FFT from './fourier-transforms-bindings.mjs';
import * as SP from './signal-processor.mjs';

// Generate a sine wave
const signal = SP.generateSineWave(
  440,      // 440 Hz
  1,        // 1 second
  44100     // 44.1 kHz sample rate
);

// Perform spectral analysis
const analysis = SP.spectralAnalysis(signal, 44100);

console.log('RMS:', analysis.rms);
console.log('Peak value:', analysis.peakValue);
console.log('Top peaks:', analysis.peaks.slice(0, 5));
console.log('Frequencies:', analysis.frequencies.slice(0, 10));
```

### Windowing

```javascript
import * as FFT from './fourier-transforms-bindings.mjs';

const signal = [1, 2, 3, 4, 5, 4, 3, 2, 1];

// Apply Hann window
const window = FFT.window(FFT.WindowType.Hann, signal.length);
const windowed = signal.map((x, i) => x * window[i]);

console.log('Windowed signal:', windowed);
```

### Resampling

```javascript
import * as SP from './signal-processor.mjs';

const signal = [1, 2, 3, 4, 5];

// Downsample by factor of 2
const downsampled = SP.downsample(signal, 2);
console.log('Downsampled:', downsampled); // [1, 3, 5]

// Upsample by factor of 3 (zero-insertion)
const upsampled = SP.upsample(signal, 3);
console.log('Upsampled length:', upsampled.length); // 15
```

### Moving Average Filter

```javascript
import * as SP from './signal-processor.mjs';

const signal = [1, 3, 2, 5, 4, 8, 7, 9];
const filtered = SP.movingAverage(signal, 3);
console.log('Filtered:', filtered);
```

## Integration with the Demo

The `taylor_series_demo` uses these bindings to:

1. Generate mathematical functions (sine, cosine, exponential)
2. Apply Taylor series approximations
3. Perform FFT analysis on the resulting signals
4. Visualize time-domain and frequency-domain representations
5. Display error bands and peak frequencies

## API Reference

### Complex Number Operations

- `complex(real, imag)` - Create a complex number
- `complexAdd(a, b)` - Add two complex numbers
- `complexMul(a, b)` - Multiply two complex numbers
- `complexMagnitude(c)` - Get magnitude
- `complexPhase(c)` - Get phase angle

### Transform Functions

- `fft(samples: Complex[])` - Fast Fourier Transform
- `ifft(spectrum: Complex[])` - Inverse FFT
- `dft(samples: Complex[])` - Discrete Fourier Transform
- `idft(spectrum: Complex[])` - Inverse DFT
- `realFFT(samples: number[])` - FFT for real signals
- `inverseRealFFT(spectrum: Complex[])` - Inverse real FFT
- `dct(samples: number[])` - Discrete Cosine Transform
- `idct(coefficients: number[])` - Inverse DCT

### Spectrum Analysis

- `magnitudeSpectrum(spectrum: Complex[])` - Magnitude at each bin
- `powerSpectrum(spectrum: Complex[])` - Power (magnitude squared)
- `phaseSpectrum(spectrum: Complex[])` - Phase angle
- `fftShift(spectrum: Complex[])` - Center zero-frequency component
- `frequencyBins(sampleCount, sampleRate)` - Frequency values

### Signal Processing

- `window(type, size)` - Generate window function
- `applyWindow(signal, windowType)` - Apply window to signal
- `spectralAnalysis(signal, sampleRate, windowType)` - Full analysis
- `findPeaks(signal, threshold)` - Find peaks in signal
- `computeRMS(signal)` - Root mean square
- `computeCrestFactor(signal)` - Peak / RMS ratio
- `downsample(signal, factor)` - Reduce sample rate
- `upsample(signal, factor)` - Increase sample rate
- `resample(signal, oldRate, newRate)` - Arbitrary resampling
- `addNoise(signal, snr)` - Add Gaussian noise
- `movingAverage(signal, windowSize)` - Low-pass filter
- `autocorrelation(signal)` - Autocorrelation function

### Waveform Generation

- `generateSineWave(frequency, duration, sampleRate)` - Sine wave
- `generateCosineWave(frequency, duration, sampleRate)` - Cosine wave

## Performance Notes

- FFT automatically detects power-of-two sizes and uses fast algorithm
- Non-power-of-two sizes fall back to DFT (slower)
- Complex arithmetic uses object representation `{ real, imag }`
- For large arrays (>10000 samples), consider Web Workers

## Future Enhancements

- [ ] Emscripten compilation of C++ library to WebAssembly
- [ ] Type definitions (TypeScript stubs)
- [ ] Worker pool for parallel FFT
- [ ] GPU acceleration via WebGL
- [ ] STFT (Short-Time Fourier Transform)
- [ ] Wavelet transforms
- [ ] Real-time streaming analysis

## Testing

To test the bindings in Node.js:

```bash
node --input-type=module --eval "
  import * as FFT from './fourier-transforms-bindings.mjs';
  const signal = [FFT.complex(1, 0), FFT.complex(0, 1), FFT.complex(-1, 0), FFT.complex(0, -1)];
  const result = FFT.fft(signal);
  console.log('FFT result:', result);
"
```

## References

- [Cooley-Tukey FFT Algorithm](https://en.wikipedia.org/wiki/Cooley%E2%80%93Tukey_FFT_algorithm)
- [Discrete Fourier Transform](https://en.wikipedia.org/wiki/Discrete_Fourier_transform)
- [Window Functions](https://en.wikipedia.org/wiki/Window_function)
- [Emscripten Documentation](https://emscripten.org/)
