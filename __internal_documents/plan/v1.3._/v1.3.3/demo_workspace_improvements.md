# Demo Workspace Improvements — v1.3.3

## Summary

Three areas of work: new **Sorting Algorithms** demo, improvements to the existing **K-Means ML** demo, and removal of the redundant outer tab bar from the `<demo-viewer>` shell.

---

## 1. Demo Viewer Shell — Remove Outer Tabs

**File:** `_interfaces/GUI/static/screens/package_builder/demo-viewer.mjs`

The demo viewer previously injected a `📱 Demo / </> Code / 📚 Libraries` tab bar above every demo iframe. This conflicted with demos that manage their own internal tabs. The outer tab bar was removed entirely — the iframe now renders directly into the content area. Demos that need tabs should implement them internally (as ml_demo now does).

---

## 2. K-Means ML Demo — Tab Structure + Chart Fix

**File:** `_internal_workspace/demo_workspaces/ml_demo/index.html`

### Tab bar added
Replaced the single-page layout with an internal four-tab bar:

| Tab | Contents |
|-----|----------|
| 📈 Demo | Interactive K-Means chart viewer (default) |
| `</> Code` | Python usage example (`ml_toolbox.kmeans.fit`) |
| ⚙ Setup | `pip install ml_toolbox` and `requirements.txt` snippet |
| 📦 Libraries | Cards for `graphics-chart.mjs` (JS) and `GRAPHICS::charts` (C++) |

### Bottom-right chart changed
The bottom-right small chart was changed from **Points per cluster** (raw count) to **Intra-cluster variance** (average squared Euclidean distance from each point to its centroid). This is more analytically meaningful alongside the inertia curve already shown on the left.

```js
// Intra-cluster variance per cluster ci:
variance[ci] = mean over points p in ci of: (p.x - cx)² + (p.y - cy)²
```

### Libraries tab
Shows only the two libraries actually used in the demo — not generic packages:
- `graphics-chart.mjs` — browser-side `<graphics-chart>` custom element
- `GRAPHICS::charts` — C++ charting library from the trekker group whose API the JS module mirrors

---

## 3. Sorting Algorithms Demo — New Demo + Auto-Play

### New files

| File | Description |
|------|-------------|
| `_internal_workspace/demo_workspaces/sorts_demo/index.html` | Full visual step-by-step sorting demo |
| `_internal_workspace/demo_workspaces/sorts_demo/demo.json` | Demo metadata (title, icon, libraries) |

### Algorithms visualised
Bubble, Insertion, Selection, Shell, Merge, Quick, Heap, Intro, Tim — each runs as a side-by-side card with a canvas bar chart that animates step-by-step.

### Controls
- Algorithm pills to toggle which algorithms are shown
- Array size slider (8–64), input type select (random / nearly sorted / reversed / few unique / mountain)
- Speed slider (1–10)
- Generate, ▶ Play / ⏸ Pause, ⏭ Step, ↺ Reset buttons

### Auto-play
On page load and after every Generate click the animation starts automatically — no need to locate the Play button.

### Comparison chart
After all algorithms finish, a bar chart (using `graphics-chart.mjs`) shows operation counts side-by-side.

### New C++ library wired up

| File | Change |
|------|--------|
| `_deliverables/libraries/groups/trekker/ALGORITHM/lists/sorts/sorts.h` | New header-only file — 12 sorting algorithms in `trekker::algorithm::sorts` namespace |
| `_deliverables/libraries/groups/trekker/ALGORITHM/lists/sorts/test_sorts.cpp` | 60+ tests via tyst_framework |
| `_deliverables/libraries/groups/trekker/ALGORITHM/CMakeLists.txt` | `algorithm_headers` interface target; `sorts_tests` test executable |
| `_deliverables/libraries/groups/trekker/CMakeLists.txt` | Added `add_subdirectory(ALGORITHM)` |

---

## 4. Music Synthesizer Demo — New Demo + Emscripten Bindings

### New demo files

| File | Description |
|------|-------------|
| `_internal_workspace/demo_workspaces/music_demo/index.html` | Full interactive piano synthesizer demo |
| `_internal_workspace/demo_workspaces/music_demo/demo.json` | Demo metadata (title, icon, libraries) |

### Demo features (Demo tab)
- **Oscilloscope canvas** — triggered time-domain waveform with peak readout
- **Spectrum analyser canvas** — logarithmic-scaled FFT bar chart with dominant frequency readout
- **Wave type pills** — Sine, Square, Triangle, Sawtooth, Reverse Sawtooth, White Noise
- **Volume slider** — maps 0–100% to master gain via `engine.setMasterGain()`
- **LPF cutoff slider** — logarithmically mapped 80 Hz → 20 kHz; updates all active oscillators in real-time
- **Active notes display** — shows note name pills for every key currently held
- **2-octave piano keyboard** (C3–B4, MIDI 48–71) — mouse, touch, and keyboard input
  - Keyboard shortcuts: `a s d f g h j` = C4–B4, `w e t y u` = sharps
  - Keys generated programmatically from MIDI note definitions

### Internal tab structure

| Tab | Contents |
|-----|----------|
| 🎵 Demo | Live piano + visualizers + controls |
| `</> Code` | JS example: oscillator, pre-rendered buffer, mix + effects chain |
| ⚙ Setup | Emscripten build commands + C++ native usage |
| 📦 Libraries | Cards for `audio-synth.mjs`, `trekker::audio`, `utils::wave_generator` |

### New JS library

**File:** `_interfaces/GUI/static/extensions/audio-synth.mjs`

Browser-side JS mirror of both C++ audio libraries. Uses the Web Audio API internally. Exports:

| Export | Description |
|--------|-------------|
| `WavePattern` | Enum object: Sine, Square, Triangle, Sawtooth, ReverseSawtooth, Pulse, WhiteNoise |
| `WaveConfig` | Config struct: amplitude, frequency_hz, phase_radians, offset, duty_cycle, noise_seed |
| `sample_at` | Evaluate waveform at a point in time |
| `generate_samples` | Generate N samples at a given sample rate |
| `generate_samples_for_duration` | Generate samples for a duration in seconds |
| `AudioBuffer` | Planar F32 buffer: `planes[ch][sample]`, `from_samples()`, `clone()`, `silent()` |
| `mix` | Mix multiple AudioBuffers with per-buffer gains |
| `peak_normalize` / `rms_normalize` | Normalisation |
| `apply_gain_envelope` | Per-sample gain curve |
| `fade_in` / `fade_out` | Linear ramp in/out |
| `low_pass_filter` / `high_pass_filter` | Biquad IIR filters |
| `delay_effect` | Echo/delay with feedback |
| `compress` | Dynamics compressor |
| `AudioSynthEngine` | Real-time engine: `createOscillator()`, `play()`, `stop()`, `stopAll()`, `setMasterGain()`, `.analyser` |

`AudioSynthEngine` signal chain: all oscillators → oscillator gain → BiquadFilter → masterGain → AnalyserNode → destination.

### New emscripten binding files

| File | Description |
|------|-------------|
| `_deliverables/libraries/bindings/emscripten_bindings/audio_processing_bindings.cpp` | Bindings for `trekker::audio` — `AudioBufferWrapper`, `ResamplerWrapper`, all free-function effects |
| `_deliverables/libraries/bindings/emscripten_bindings/wave_generator_bindings.cpp` | Bindings for `utils::wave_generator` — `WaveConfig` value_object, `WavePattern` int constants, `sample_at`, `generate_samples`, `generate_samples_for_duration` |

### CMakeLists.txt additions

**File:** `_deliverables/libraries/bindings/emscripten_bindings/CMakeLists.txt`

Two new targets appended (using the existing `add_emscripten_module` macro):

| Target | Export name | Source library |
|--------|-------------|----------------|
| `wave_generator_js` | `createWaveGeneratorModule` | `trekker/MISC/wave_generator` |
| `audio_processing_js` | `createAudioProcessingModule` | `audio_visual_group/VIDEO_ASSETS/audio_processing` |
