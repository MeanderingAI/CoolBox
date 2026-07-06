/**
 * Emscripten JavaScript Bindings for Fourier Transforms
 * 
 * This module provides JavaScript wrappers for the C++ fourier_tranforms library,
 * enabling client-side signal processing in web applications.
 * 
 * Usage:
 *   import * as FFT from './fourier-transforms-bindings.mjs';
 *   
 *   const signal = [1, 2, 3, 4, 5];
 *   const spectrum = FFT.fft(signal);
 *   const magnitude = FFT.magnitudeSpectrum(spectrum);
 */

/**
 * Complex number representation
 * @typedef {Object} Complex
 * @property {number} real - Real part
 * @property {number} imag - Imaginary part
 */

/**
 * Create a complex number
 * @param {number} real 
 * @param {number} imag 
 * @returns {Complex}
 */
export function complex(real, imag = 0) {
  return { real, imag };
}

/**
 * Add two complex numbers
 * @param {Complex} a 
 * @param {Complex} b 
 * @returns {Complex}
 */
export function complexAdd(a, b) {
  return complex(a.real + b.real, a.imag + b.imag);
}

/**
 * Multiply two complex numbers
 * @param {Complex} a 
 * @param {Complex} b 
 * @returns {Complex}
 */
export function complexMul(a, b) {
  return complex(
    a.real * b.real - a.imag * b.imag,
    a.real * b.imag + a.imag * b.real
  );
}

/**
 * Get the magnitude of a complex number
 * @param {Complex} c 
 * @returns {number}
 */
export function complexMagnitude(c) {
  return Math.sqrt(c.real * c.real + c.imag * c.imag);
}

/**
 * Get the phase of a complex number
 * @param {Complex} c 
 * @returns {number}
 */
export function complexPhase(c) {
  return Math.atan2(c.imag, c.real);
}

/**
 * Convert complex array from interleaved format [r0, i0, r1, i1, ...]
 * @param {number[]} interleaved 
 * @returns {Complex[]}
 */
export function fromInterleaved(interleaved) {
  const result = [];
  for (let i = 0; i < interleaved.length; i += 2) {
    result.push(complex(interleaved[i], interleaved[i + 1]));
  }
  return result;
}

/**
 * Convert complex array to interleaved format [r0, i0, r1, i1, ...]
 * @param {Complex[]} complexArray 
 * @returns {number[]}
 */
export function toInterleaved(complexArray) {
  const result = [];
  for (const c of complexArray) {
    result.push(c.real, c.imag);
  }
  return result;
}

/**
 * Window types for signal processing
 * @enum {string}
 */
export const WindowType = {
  Rectangular: 'rect',
  Hann: 'hann',
  Hamming: 'hamming',
  Blackman: 'blackman'
};

/**
 * Generate a window function
 * @param {string} type - WindowType
 * @param {number} size - Window size
 * @returns {number[]}
 */
export function window(type, size) {
  const result = new Array(size);
  const coeffs = [];

  if (size === 0) return result;
  if (size === 1) {
    result[0] = 1.0;
    return result;
  }

  const pi = Math.PI;
  const twoPi = 2 * pi;
  const denominator = size - 1;

  for (let i = 0; i < size; i++) {
    const phase = twoPi * i / denominator;
    switch (type) {
      case WindowType.Rectangular:
        coeffs[i] = 1.0;
        break;
      case WindowType.Hann:
        coeffs[i] = 0.5 - 0.5 * Math.cos(phase);
        break;
      case WindowType.Hamming:
        coeffs[i] = 0.54 - 0.46 * Math.cos(phase);
        break;
      case WindowType.Blackman:
        coeffs[i] = 0.42 - 0.5 * Math.cos(phase) + 0.08 * Math.cos(2 * phase);
        break;
      default:
        coeffs[i] = 1.0;
    }
  }

  return coeffs;
}

/**
 * Fast Fourier Transform (FFT)
 * @param {Complex[]} samples - Input signal
 * @returns {Complex[]} - Frequency spectrum
 */
export function fft(samples) {
  // Cooley-Tukey FFT algorithm
  const N = samples.length;
  
  if (N <= 1) return samples.slice();
  if (!isPowerOfTwo(N)) return dft(samples);

  const even = [];
  const odd = [];

  for (let i = 0; i < N; i++) {
    if (i % 2 === 0) even.push(samples[i]);
    else odd.push(samples[i]);
  }

  const evenFFT = fft(even);
  const oddFFT = fft(odd);

  const result = new Array(N);
  const pi2 = 2 * Math.PI;

  for (let k = 0; k < N / 2; k++) {
    const angle = -pi2 * k / N;
    const w = complex(Math.cos(angle), Math.sin(angle));
    const t = complexMul(w, oddFFT[k]);
    result[k] = complexAdd(evenFFT[k], t);
    result[k + N / 2] = complexAdd(evenFFT[k], complex(-t.real, -t.imag));
  }

  return result;
}

/**
 * Inverse Fast Fourier Transform (IFFT)
 * @param {Complex[]} spectrum - Frequency spectrum
 * @returns {Complex[]} - Time domain signal
 */
export function ifft(spectrum) {
  const N = spectrum.length;
  
  // Conjugate input
  const conj = spectrum.map(c => complex(c.real, -c.imag));
  
  // Apply FFT
  const result = fft(conj);
  
  // Conjugate and scale output
  return result.map(c => complex(c.real / N, -c.imag / N));
}

/**
 * Discrete Fourier Transform (DFT)
 * @param {Complex[]} samples - Input signal
 * @returns {Complex[]} - Frequency spectrum
 */
export function dft(samples) {
  const N = samples.length;
  const output = new Array(N);
  const pi2 = 2 * Math.PI;

  for (let k = 0; k < N; k++) {
    let real = 0, imag = 0;
    for (let n = 0; n < N; n++) {
      const angle = -pi2 * k * n / N;
      const cos = Math.cos(angle);
      const sin = Math.sin(angle);
      real += samples[n].real * cos - samples[n].imag * sin;
      imag += samples[n].real * sin + samples[n].imag * cos;
    }
    output[k] = complex(real, imag);
  }

  return output;
}

/**
 * Inverse Discrete Fourier Transform (IDFT)
 * @param {Complex[]} spectrum - Frequency spectrum
 * @returns {Complex[]} - Time domain signal
 */
export function idft(spectrum) {
  const N = spectrum.length;
  const output = new Array(N);
  const pi2 = 2 * Math.PI;

  for (let n = 0; n < N; n++) {
    let real = 0, imag = 0;
    for (let k = 0; k < N; k++) {
      const angle = pi2 * k * n / N;
      const cos = Math.cos(angle);
      const sin = Math.sin(angle);
      real += spectrum[k].real * cos - spectrum[k].imag * sin;
      imag += spectrum[k].real * sin + spectrum[k].imag * cos;
    }
    output[n] = complex(real / N, imag / N);
  }

  return output;
}

/**
 * Real FFT - optimized for real-valued input
 * @param {number[]} samples - Real-valued input signal
 * @returns {Complex[]} - Frequency spectrum
 */
export function realFFT(samples) {
  const complexSamples = samples.map(x => complex(x, 0));
  return fft(complexSamples);
}

/**
 * Inverse Real FFT
 * @param {Complex[]} spectrum - Frequency spectrum
 * @returns {number[]} - Real-valued time domain signal
 */
export function inverseRealFFT(spectrum) {
  const result = ifft(spectrum);
  return result.map(c => c.real);
}

/**
 * Magnitude spectrum
 * @param {Complex[]} spectrum - Frequency spectrum
 * @returns {number[]} - Magnitude at each frequency bin
 */
export function magnitudeSpectrum(spectrum) {
  return spectrum.map(complexMagnitude);
}

/**
 * Power spectrum (magnitude squared)
 * @param {Complex[]} spectrum - Frequency spectrum
 * @returns {number[]} - Power at each frequency bin
 */
export function powerSpectrum(spectrum) {
  return spectrum.map(c => c.real * c.real + c.imag * c.imag);
}

/**
 * Phase spectrum
 * @param {Complex[]} spectrum - Frequency spectrum
 * @returns {number[]} - Phase at each frequency bin
 */
export function phaseSpectrum(spectrum) {
  return spectrum.map(complexPhase);
}

/**
 * FFT shift - move zero-frequency component to center
 * @param {Complex[]} spectrum - Frequency spectrum
 * @returns {Complex[]} - Shifted spectrum
 */
export function fftShift(spectrum) {
  const shifted = spectrum.slice();
  if (shifted.length === 0) return shifted;
  
  const mid = Math.floor(shifted.length / 2);
  const temp = shifted.slice(0, mid);
  for (let i = 0; i < shifted.length - mid; i++) {
    shifted[i] = shifted[mid + i];
  }
  for (let i = 0; i < mid; i++) {
    shifted[shifted.length - mid + i] = temp[i];
  }
  
  return shifted;
}

/**
 * Discrete Cosine Transform (DCT)
 * @param {number[]} samples - Input signal
 * @returns {number[]} - DCT coefficients
 */
export function dct(samples) {
  const N = samples.length;
  const result = new Array(N);
  const pi = Math.PI;

  for (let k = 0; k < N; k++) {
    let sum = 0;
    for (let n = 0; n < N; n++) {
      sum += samples[n] * Math.cos(pi * (n + 0.5) * k / N);
    }
    result[k] = sum;
  }

  return result;
}

/**
 * Inverse Discrete Cosine Transform (IDCT)
 * @param {number[]} coefficients - DCT coefficients
 * @returns {number[]} - Original signal
 */
export function idct(coefficients) {
  const N = coefficients.length;
  const result = new Array(N);
  const pi = Math.PI;

  for (let n = 0; n < N; n++) {
    let sum = N === 0 ? 0 : coefficients[0] * 0.5;
    for (let k = 1; k < N; k++) {
      sum += coefficients[k] * Math.cos(pi * k * (n + 0.5) / N);
    }
    result[n] = N === 0 ? 0 : (2 / N) * sum;
  }

  return result;
}

/**
 * Frequency bins for a given sample count and rate
 * @param {number} sampleCount - Number of samples
 * @param {number} sampleRateHz - Sample rate in Hz
 * @returns {number[]} - Frequency values for each bin
 */
export function frequencyBins(sampleCount, sampleRateHz) {
  const result = new Array(sampleCount);
  for (let i = 0; i < sampleCount; i++) {
    result[i] = (i * sampleRateHz) / sampleCount;
  }
  return result;
}

/**
 * Check if a number is a power of two
 * @private
 * @param {number} n 
 * @returns {boolean}
 */
function isPowerOfTwo(n) {
  return n > 0 && (n & (n - 1)) === 0;
}

/**
 * Get next power of two
 * @private
 * @param {number} n 
 * @returns {number}
 */
function nextPowerOfTwo(n) {
  if (n <= 1) return 1;
  let power = 1;
  while (power < n) power <<= 1;
  return power;
}

// Export for CommonJS environments
if (typeof module !== 'undefined' && module.exports) {
  module.exports = {
    complex,
    complexAdd,
    complexMul,
    complexMagnitude,
    complexPhase,
    fromInterleaved,
    toInterleaved,
    WindowType,
    window,
    fft,
    ifft,
    dft,
    idft,
    realFFT,
    inverseRealFFT,
    magnitudeSpectrum,
    powerSpectrum,
    phaseSpectrum,
    fftShift,
    dct,
    idct,
    frequencyBins
  };
}
