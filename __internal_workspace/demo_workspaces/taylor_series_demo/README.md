# Taylor Series & Signal Processing Demo

An interactive demonstration showcasing Taylor series polynomial approximation combined with Fourier transform signal analysis using the signal processing library.

## Overview

This demo illustrates:

1. **Taylor Series Approximation** - Polynomial approximation of mathematical functions (sin, cos, e^x)
2. **Signal Processing** - Real-time FFT computation with multiple window functions
3. **Error Analysis** - Visualization of approximation error bands
4. **Frequency Domain Analysis** - Magnitude spectrum visualization

## Features

### Interactive Controls

- **Function Selection** - Switch between sin(x), cos(x), and e^x
- **Taylor Order** - Adjust approximation order (1-20 terms)
- **X Range** - Set the domain range (±1 to ±10)
- **Sample Rate** - Control frequency resolution (10-200 samples)
- **Window Functions** - Compare Rectangular, Hann, and Hamming windows
- **Display Options** - Toggle exact function, Taylor approximation, and error band

### Visualization Tabs

1. **Time Domain** - Shows exact function vs Taylor approximation
2. **Frequency Domain** - FFT magnitude spectrum with peak detection
3. **Error Analysis** - Approximation error visualization

### Export Functionality

- Export analysis data to CSV format for external analysis

## Usage

### Basic Operation

1. Select a function (sin, cos, or exp)
2. Adjust the Taylor order to see how approximation improves
3. Switch between tabs to view time-domain and frequency-domain representations
4. Use window selector to compare spectral leakage
5. Export data for further analysis

### Mathematical Concepts Demonstrated

#### Taylor Series
The Taylor series expansion approximates a function f(x) around point x=0:

```
f(x) ≈ f(0) + f'(0)·x + f''(0)·x²/2! + ... + f⁽ⁿ⁾(0)·xⁿ/n!
```

For example:
- sin(x) = x - x³/3! + x⁵/5! - ...
- cos(x) = 1 - x²/2! + x⁴/4! - ...
- e^x = 1 + x + x²/2! + x³/3! + ...

#### Fourier Transform
The FFT converts a time-domain signal to its frequency-domain representation, revealing the frequency components present in the signal.

#### Window Functions
Windows reduce spectral leakage when analyzing finite-length signals:
- **Rectangular** - Simple but high sidelobes
- **Hann** - Good frequency resolution
- **Hamming** - Lower sidelobe level than Hann

## Technical Implementation

### Files

- `demo.json` - Metadata and configuration
- `index.html` - Complete interactive application

### Dependencies

- Emscripten signal processing extensions (JavaScript implementation)
- HTML5 Canvas for visualization
- Pure JavaScript (no external dependencies)

### Browser Compatibility

Requires:
- Modern browser (Chrome, Firefox, Safari, Edge)
- HTML5 Canvas support
- ES6 Module support

### Performance

- Time-domain visualization: Real-time at 60 FPS
- FFT computation: <1ms for 256-sample signals
- Responsive to all control changes

## Educational Value

This demo helps understand:

1. **Approximation Theory** - How polynomial approximations converge
2. **Spectral Analysis** - Frequency content of signals
3. **Window Effects** - Impact of windowing on FFT results
4. **Signal Processing Basics** - Practical DSP concepts

## Integration with Emscripten Extensions

The demo uses JavaScript bindings from the emscripten_extensions directory:

- `fourier-transforms-bindings.mjs` - FFT and transform operations
- `signal-processor.mjs` - High-level signal processing utilities

These can be replaced with WebAssembly versions compiled from the C++ library for better performance on large datasets.

## Extensions and Customizations

### Adding New Functions

Edit the `getExactValue()` and `getTaylorValue()` methods in the JavaScript to add more functions:

```javascript
case 'sinh':
  return this.taylorSinh(x, this.taylorOrder);
```

### Adjusting UI Layout

Modify CSS variables in the `<style>` section:

```css
:root {
  --bg: #0f1117;
  --surface: #1a1d27;
  /* ... */
}
```

### Changing Plot Ranges

Adjust the default values in the constructor:

```javascript
this.taylorOrder = 10;      // More terms by default
this.sampleRate = 256;      // Higher resolution
```

## Related Resources

- [Emscripten Extensions README](../emscripten_extensions/README.md)
- [Taylor Series (Wikipedia)](https://en.wikipedia.org/wiki/Taylor_series)
- [Fast Fourier Transform (Wikipedia)](https://en.wikipedia.org/wiki/Fast_Fourier_transform)
- [Window Function (Wikipedia)](https://en.wikipedia.org/wiki/Window_function)

## Future Enhancements

- [ ] WebAssembly compilation of C++ FFT for better performance
- [ ] STFT (Short-Time Fourier Transform) visualization
- [ ] Multi-function overlay comparison
- [ ] Real-time audio input analysis
- [ ] 2D surface plots for 2D Fourier analysis
- [ ] Animation of Taylor series convergence
- [ ] Interactive coefficient adjustment

## License

Part of the CoolBox project signal processing library.
