/**
 * Signal Processing Utilities
 * 
 * High-level signal processing functions combining multiple transforms
 * and analysis techniques.
 */

import * as FFT from './fourier-transforms-bindings.mjs';

/**
 * Generate a test signal (sine wave)
 * @param {number} frequency - Frequency in Hz
 * @param {number} duration - Duration in seconds
 * @param {number} sampleRate - Sample rate in Hz
 * @returns {number[]}
 */
export function generateSineWave(frequency, duration, sampleRate) {
  const samples = [];
  const sampleCount = Math.floor(duration * sampleRate);
  const twoPi = 2 * Math.PI;

  for (let i = 0; i < sampleCount; i++) {
    const t = i / sampleRate;
    samples.push(Math.sin(twoPi * frequency * t));
  }

  return samples;
}

/**
 * Generate a cosine wave
 * @param {number} frequency - Frequency in Hz
 * @param {number} duration - Duration in seconds
 * @param {number} sampleRate - Sample rate in Hz
 * @returns {number[]}
 */
export function generateCosineWave(frequency, duration, sampleRate) {
  const samples = [];
  const sampleCount = Math.floor(duration * sampleRate);
  const twoPi = 2 * Math.PI;

  for (let i = 0; i < sampleCount; i++) {
    const t = i / sampleRate;
    samples.push(Math.cos(twoPi * frequency * t));
  }

  return samples;
}

/**
 * Apply a window to a signal
 * @param {number[]} signal - Input signal
 * @param {string} windowType - Window type from FFT.WindowType
 * @returns {number[]}
 */
export function applyWindow(signal, windowType) {
  const windowCoeffs = FFT.window(windowType, signal.length);
  return signal.map((x, i) => x * windowCoeffs[i]);
}

/**
 * Perform spectral analysis
 * @param {number[]} signal - Input signal
 * @param {number} sampleRate - Sample rate in Hz
 * @param {string} windowType - Window type
 * @returns {Object} - Analysis results
 */
export function spectralAnalysis(signal, sampleRate, windowType = FFT.WindowType.Hann) {
  // Apply window
  const windowed = applyWindow(signal, windowType);

  // Convert to complex
  const complexSignal = windowed.map(x => FFT.complex(x, 0));

  // Perform FFT
  const spectrum = FFT.fft(complexSignal);

  // Compute magnitude and phase
  const magnitude = FFT.magnitudeSpectrum(spectrum);
  const phase = FFT.phaseSpectrum(spectrum);
  const power = FFT.powerSpectrum(spectrum);

  // Frequency bins
  const frequencies = FFT.frequencyBins(magnitude.length, sampleRate);

  // Find peaks
  const peaks = findPeaks(magnitude);

  return {
    spectrum,
    magnitude,
    phase,
    power,
    frequencies,
    peaks,
    rms: computeRMS(signal),
    peakValue: Math.max(...signal.map(Math.abs))
  };
}

/**
 * Find peaks in a signal
 * @param {number[]} signal - Input signal
 * @param {number} threshold - Minimum peak height (as fraction of max)
 * @returns {Array} - Array of {index, value} peak objects
 */
export function findPeaks(signal, threshold = 0.1) {
  const peaks = [];
  const maxVal = Math.max(...signal);

  for (let i = 1; i < signal.length - 1; i++) {
    if (signal[i] > signal[i - 1] && signal[i] > signal[i + 1] && signal[i] > threshold * maxVal) {
      peaks.push({ index: i, value: signal[i] });
    }
  }

  return peaks.sort((a, b) => b.value - a.value);
}

/**
 * Compute RMS (Root Mean Square) value
 * @param {number[]} signal - Input signal
 * @returns {number}
 */
export function computeRMS(signal) {
  if (signal.length === 0) return 0;
  const sumSquares = signal.reduce((sum, x) => sum + x * x, 0);
  return Math.sqrt(sumSquares / signal.length);
}

/**
 * Compute crest factor (peak / RMS)
 * @param {number[]} signal - Input signal
 * @returns {number}
 */
export function computeCrestFactor(signal) {
  const rms = computeRMS(signal);
  if (rms === 0) return 0;
  return Math.max(...signal.map(Math.abs)) / rms;
}

/**
 * Downsample a signal
 * @param {number[]} signal - Input signal
 * @param {number} factor - Downsampling factor
 * @returns {number[]}
 */
export function downsample(signal, factor) {
  const result = [];
  for (let i = 0; i < signal.length; i += factor) {
    result.push(signal[i]);
  }
  return result;
}

/**
 * Upsample a signal (zero-insertion)
 * @param {number[]} signal - Input signal
 * @param {number} factor - Upsampling factor
 * @returns {number[]}
 */
export function upsample(signal, factor) {
  const result = [];
  for (const sample of signal) {
    result.push(sample);
    for (let i = 1; i < factor; i++) {
      result.push(0);
    }
  }
  return result;
}

/**
 * Linear interpolation between two values
 * @param {number} a 
 * @param {number} b 
 * @param {number} t - Parameter from 0 to 1
 * @returns {number}
 */
export function lerp(a, b, t) {
  return a * (1 - t) + b * t;
}

/**
 * Resample a signal using linear interpolation
 * @param {number[]} signal - Input signal
 * @param {number} oldRate - Original sample rate
 * @param {number} newRate - Target sample rate
 * @returns {number[]}
 */
export function resample(signal, oldRate, newRate) {
  if (oldRate === newRate) return signal.slice();

  const factor = oldRate / newRate;
  const newLength = Math.ceil(signal.length * (newRate / oldRate));
  const result = [];

  for (let i = 0; i < newLength; i++) {
    const pos = i * factor;
    const idx = Math.floor(pos);
    const frac = pos - idx;

    if (idx >= signal.length - 1) {
      result.push(signal[signal.length - 1]);
    } else {
      result.push(lerp(signal[idx], signal[idx + 1], frac));
    }
  }

  return result;
}

/**
 * Add noise to a signal
 * @param {number[]} signal - Input signal
 * @param {number} snr - Signal-to-noise ratio in dB
 * @returns {number[]}
 */
export function addNoise(signal, snr) {
  const rms = computeRMS(signal);
  const noiseRms = rms / Math.pow(10, snr / 20);

  return signal.map(x => x + (Math.random() - 0.5) * 2 * noiseRms);
}

/**
 * Simple moving average filter
 * @param {number[]} signal - Input signal
 * @param {number} windowSize - Filter window size
 * @returns {number[]}
 */
export function movingAverage(signal, windowSize) {
  const result = [];
  for (let i = 0; i < signal.length; i++) {
    let sum = 0;
    let count = 0;
    for (let j = Math.max(0, i - Math.floor(windowSize / 2)); j < Math.min(signal.length, i + Math.ceil(windowSize / 2)); j++) {
      sum += signal[j];
      count++;
    }
    result.push(sum / count);
  }
  return result;
}

/**
 * Compute autocorrelation
 * @param {number[]} signal - Input signal
 * @returns {number[]}
 */
export function autocorrelation(signal) {
  const N = signal.length;
  const result = new Array(N);
  const meanSignal = signal.reduce((a, b) => a + b, 0) / N;
  const centered = signal.map(x => x - meanSignal);

  for (let lag = 0; lag < N; lag++) {
    let sum = 0;
    for (let n = 0; n < N - lag; n++) {
      sum += centered[n] * centered[n + lag];
    }
    result[lag] = sum / N;
  }

  return result;
}

export default {
  generateSineWave,
  generateCosineWave,
  applyWindow,
  spectralAnalysis,
  findPeaks,
  computeRMS,
  computeCrestFactor,
  downsample,
  upsample,
  lerp,
  resample,
  addNoise,
  movingAverage,
  autocorrelation
};
