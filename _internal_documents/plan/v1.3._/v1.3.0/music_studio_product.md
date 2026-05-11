# music_studio Product (v1.3.0)

**Path:** `_Product/music_studio/`  
**Date:** May 9 2026  
**Status:** complete

## Overview

A GUI application that combines the `audio_visual_group` libraries with the `audio_mixer_lib` and `video_display_lib` app assets into a four-panel music production environment. Registered in `_Product/CMakeLists.txt` with an `if(EXISTS ...)` guard.

## Directory structure

```
music_studio/
  CMakeLists.txt
  src/
    main.cpp              ← entry point; mounts all four views
    sequencer_view.hpp/.cpp
    mixer_view.hpp/.cpp
    theory_view.hpp/.cpp
    video_view.hpp/.cpp
```

## CMake dependencies

All dependencies resolved via `if(TARGET ...)` — the application compiles as a headless stub if any library is absent.

| Library | Purpose |
|---|---|
| `full_application_window` | GUI window framework |
| `audio_mixer_lib` | Mixing console backend |
| `video_display_lib` | Frame buffer + video player |
| `note_synthesis` | Oscillator / ADSR synthesis |
| `music_theory` | Scale / chord / progression data |
| `music_sequencer` | Step-sequencer render engine |
| `audio_processing` | Resampling, effects, normalisation |
| `audio_sync` | A/V sync and drift correction |
| `video_codec` | Pixel format conversion |
| `video_filters` | Spatial and temporal filters |

DLL copy commands are added for all linked shared libraries on Windows.

---

## Views

### SequencerView (`sequencer_view.hpp/.cpp`)

Step-sequencer panel.

- **Grid:** 8 tracks × 32 steps; per-step note (MIDI), velocity, gate, probability
- **Transport:** play / pause / stop; BPM slider (40 – 300)
- **Per-track controls:** volume, pan, mute, waveform selector (Sine/Square/Saw/Triangle/Pulse), ADSR sliders (attack / decay / sustain / release)
- **Backed by:** `music_sequencer` `Sequencer::render()` → sends audio to mixer bus

### MixerView (`mixer_view.hpp/.cpp`)

16-channel mixing console panel.

- **Channel strip:** fader (volume), pan knob, gain (dB), send level, mute, solo, pre-fader listen (PFL), MIDI channel assignment
- **VU meters:** peak + RMS per strip and for master; clipping indicator
- **Master section:** volume, pan, limiter threshold, limiter enable toggle
- **Backed by:** `audio_mixer_lib` `MixerConsole`

### TheoryView (`theory_view.hpp/.cpp`)

Music-theory reference and composition panel.

- **Scale selector:** 15 scale types (Major / Minor / Dorian / Phrygian / Lydian / Mixolydian / Locrian / Harmonic Minor / Melodic Minor / Pentatonic Major / Pentatonic Minor / Blues / Whole Tone / Diminished / Chromatic)
- **Root note selector:** C – B (12 pitch classes)
- **Chord selector:** 11 chord types with interval display and voicing
- **Progression builder:** up to 8 chords; root offset + chord type per slot
- **Transpose utility:** semitone offset applied on export
- **Backed by:** `music_theory` scale and chord tables

### VideoView (`video_view.hpp/.cpp`)

Video playback and display panel.

- **Transport:** play / pause / stop / seek; playback rate (0.25× – 4×)
- **Volume slider:** audio track hint
- **Loop toggle**
- **Overlays:** timecode overlay toggle; custom text overlay add / clear
- **Display:** `FrameBuffer` (1280×720 default) updated via `VideoPlayer::tick()`
- **Backed by:** `video_display_lib` `VideoPlayer` + `FrameBuffer`

---

## Entry point (`main.cpp`)

Initialises `AppWindow` (1440×900), constructs all four views, calls `init()` on each, then enters the main loop calling `tick(16666)` per frame (~60 fps). Compiles headlessly without `full_application_window`.
