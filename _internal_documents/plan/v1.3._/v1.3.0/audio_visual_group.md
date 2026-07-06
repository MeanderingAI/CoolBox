# audio_visual_group Library Group (v1.3.0)

**Path:** `_libraries/groups/audio_visual_group/`  
**Date:** May 9 2026  
**Status:** complete

## Overview

A new top-level library group sitting alongside `trekker` and `sim_group` under `_libraries/groups/`. It provides two asset collections for audio/video processing: `VIDEO_ASSETS` and `MUSIC_ASSETS`. Registered in `_libraries/CMakeLists.txt` via `add_subdirectory(groups/audio_visual_group)`.

---

## VIDEO_ASSETS

**Path:** `audio_visual_group/VIDEO_ASSETS/`

| Package | Namespace | Primary source | Key types / functions |
|---|---|---|---|
| `video_codec` | `trekker::video` | `yuv_rgb.cpp` | `PixelFormat`, `Plane`, `VideoFrame`, `yuv_to_rgb()`, `rgb_to_yuv()`, `convert_format()` |
| `video_filters` | `trekker::video` | `spatial_filters.cpp` | `gaussian_blur()`, `unsharp_mask()`, `bilateral_filter()`, `box_blur()`, `median_filter()`, `sobel_edges()`, `color_grade()`, `apply_lut()`, `TemporalDenoise`, `MotionCompensatedDenoise` |
| `audio_processing` | `trekker::audio` | `audio_buffer.cpp`, `effects.cpp` | `AudioBuffer`, `Resampler`, `ChannelRouterMatrix`, `mix()`, `peak_normalize()`, `fade_in/out()`, `low/high_pass_filter()`, `delay_effect()`, `compress()` |
| `audio_sync` | `trekker::av_sync` | `av_timestamp.cpp` | `AvTimestamp`, `MasterClock`, `SyncController`, `LipSyncAdjuster`, `DriftDetector` |
| `video_timeline` | `trekker::timeline` | `timeline.cpp` | `Clip`, `Track`, `Timeline`, `TransitionType`, `export_edl()` |
| `motion_analysis` | `trekker::motion` | `optical_flow.cpp` | `FlowField`, `optical_flow()`, `estimate_motion_vectors()`, `detect_scene_cut()`, `SceneDetector`, `compute_block_activity()` |

---

## MUSIC_ASSETS

**Path:** `audio_visual_group/MUSIC_ASSETS/`

| Package | Namespace | Primary source | Key types / functions |
|---|---|---|---|
| `note_synthesis` | `trekker::music` | `oscillator.cpp` | `Waveform` enum (Sine/Square/Saw/Triangle/Noise/Pulse), `AdsrEnvelope`, `Oscillator`, `NoteGenerator`, `midi_to_hz()`, `note_name_to_midi()` |
| `music_theory` | `trekker::music_theory` | `scale.cpp` | `PitchClass`, `ScaleType` (15 types), `Scale`, `ChordType` (11 types), `Chord`, `build_progression()` |
| `music_sequencer` | `trekker::sequencer` | `sequencer.cpp` | `StepEvent`, `Pattern`, `TrackParams`, `MixerBus`, `Sequencer` (play/pause/stop, `render()`) |

---

## Build conventions

- Each package follows the standard trekker pattern: `SHARED` library, glob-discovered by the parent `CMakeLists.txt`, tests conditional on `BUILD_TESTING AND TARGET tyst_framework_main`.
- All logic lives in one primary `.cpp`; auxiliary files are include-only stubs to satisfy CMake without duplicating code.
