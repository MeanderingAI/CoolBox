# video_display App Asset (v1.3.0)

**Path:** `_libraries/groups/app_assets/video_display/`  
**Date:** May 9 2026  
**Status:** complete

## Overview

A software video-display library providing a frame buffer, overlay system, and tick-driven player. Registered as `video_display_lib` (export name `video_display`, namespace `app_assets::video_display`). Links `video_codec` and `video_filters` when available.

## Directory structure

```
video_display/
  CMakeLists.txt
  headers/
    video_display.hpp
  source/
    frame_buffer.cpp   ← full implementation
    overlay.cpp        ← stub
    video_player.cpp   ← stub
  tests/
    test_video_display.cpp
```

## Public API

### `FrameBuffer`
RGBA32 software back-buffer consumed by the GUI toolkit.

| Method | Description |
|---|---|
| `FrameBuffer(w, h)` | Allocate `w×h×4` byte buffer |
| `blit_rgb24(data, w, h)` | Stretch-blit RGB24 source into buffer |
| `blit_rgba32(data, w, h)` | Stretch-blit RGBA32 source |
| `clear(Rgba)` | Fill with solid colour |
| `set_pixel / pixel` | Single-pixel read/write |
| `resize(w, h)` | Reallocate (discards content) |
| `raw_data()` | Direct access to RGBA byte vector |

### Overlay system

- `Overlay { text, anchor, offset_x/y, style, visible }` — `OverlayAnchor`: TopLeft / TopRight / BottomLeft / BottomRight / Center
- `layout_overlays(overlays, fb_w, fb_h) → vector<OverlayDrawCmd>` — returns pixel-space layout commands for the GUI toolkit to render (font rendering is delegated)

### `VideoPlayer`

Tick-driven; does not own a decode thread. The caller provides a `FrameProvider` callback that fills the `FrameBuffer` for a given PTS.

| Method | Description |
|---|---|
| `play / pause / stop` | Transport controls |
| `seek(pts_us)` | Jump to position |
| `set_playback_rate(rate)` | 0.0 – 8.0× (clamped) |
| `set_volume(v)` | Display volume hint 0–1 |
| `set_loop(bool)` | Loop when duration reached |
| `tick(delta_us, fb)` | Advance position; calls FrameProvider when a new frame is due; returns `true` if buffer updated |
| `enable_timecode_overlay(bool)` | Auto-updating timecode overlay |

## Tests

14 tests: frame buffer construction/resize, clear & pixel access, blit_rgb24 colour mapping, overlay layout (TopLeft / BottomRight / invisible skip), player initial state, play/pause/stop, seek clamping, tick position advance, volume/rate clamping.
