# audio_mixer App Asset (v1.3.0)

**Path:** `_libraries/groups/app_assets/audio_mixer/`  
**Date:** May 9 2026  
**Status:** complete

## Overview

A mixing-console library for use in GUI applications. Registered as `audio_mixer_lib` (export name `audio_mixer`, namespace `app_assets::audio_mixer`). Links `audio_processing`, `note_synthesis`, and `music_sequencer` when available.

## Directory structure

```
audio_mixer/
  CMakeLists.txt
  headers/
    audio_mixer.hpp
  source/
    mixer_console.cpp   ← full implementation
    channel_strip.cpp   ← stub
    vu_meter.cpp        ← stub
  tests/
    test_audio_mixer.cpp
```

## Public API

### `VuMeter`
- `update(stereo_samples)` — track peak and RMS from a stereo interleaved buffer
- `decay(factor)` — apply ballistic decay (call each display frame)
- `reset()` / `state() → VuMeterState { peak_l, peak_r, rms_l, rms_r, clipping }`

### `ChannelStrip`
- Holds `ChannelStripState { name, volume, pan, gain_db, send_level, muted, soloed, pre_fader_listen, midi_channel }`
- `process(mono_in, stereo_out, any_solo_active)` — applies gain (dB), fader, constant-power pan
- `set_volume / set_pan / set_gain_db / set_send_level / set_mute / set_solo / set_name`

### `MixerConsole`
- `add_channel() / remove_channel() / channel(id)`
- Per-channel setters: `set_channel_volume/pan/gain_db/send/mute/solo`
- Master section: `set_master_volume/pan/limiter_threshold/enabled`
- `set_param_callback(ParamCallback)` — fired on any parameter change
- `mix(channel_inputs, num_samples)` — solo detection → per-channel process → master fader + pan + soft limiter
- `channel_vu(id) / master_vu()` / `decay_all_vu()`

## Tests

14 tests covering: channel construction, volume/pan/gain/mute/solo setters, VuMeter peak and decay, `MixerConsole::mix` output shape, limiter threshold, param callback invocation.
