Adding a movie editor

## Summary

Added a full non-linear video editor app (`movie_editor`) with a media bin,
multi-track timeline, live preview, transport controls, and AVI/GIF/WAV
export, plus three new from-scratch libraries to support it since the repo
had no video/audio encoding support at all beforehand.

## New libraries (`_deliverables/libraries/groups/audio_visual_group/VIDEO_ASSETS/`)

- `video_container` — hand-rolled AVI (RIFF) writer (uncompressed BGR24
  video + optional PCM16 audio stream) and a GIF89a encoder (median-cut
  color quantizer + GIF-flavoured variable-width LZW compressor). Later
  extended with `read_gif()`/`read_avi()` decoders so the app can *import*
  GIF/AVI files, not just export them — the decoders are the exact inverse
  of the encoders and round-trip losslessly (AVI) / within quantization
  tolerance (GIF).
- `audio_container` — WAV reader (PCM8/16/32/float, IMA ADPCM) and writer
  (PCM16 + hand-rolled IMA ADPCM, ~4:1 compression).
- `movie_editor_core` — the actual NLE engine: `MediaBin` (import image
  sequences/stills/audio/GIF/AVI), `EditorProject` (wraps the existing
  `video_timeline` library's Timeline/Track/Clip), frame compositing,
  audio mixing, AVI/GIF/WAV export, and JSON project save/load. Media
  assets imported from GIF/AVI are held as in-memory decoded frames
  (`MediaType::ImportedVideo`) rather than files on disk, since those
  containers don't naturally decode into separate image files the way an
  image-sequence folder does.

Scope decision: `.mp4`/`.mov` import/export was deliberately **not**
implemented — it would require a real H.264/H.265/VP9 codec, far beyond a
from-scratch implementation. GIF and AVI are the real, playable video
formats this editor can read and write.

## The app (`_deliverables/apps/movie_editor/`)

CLI (`--cli`, `--export-*`) and GUI modes. The GUI is a mouse-driven Canvas
UI built on `full_application_window`: media bin, preview, transport
buttons, timeline ruler/tracks, inspector, status bar, and a canvas-drawn
topbar with File/Edit/Help dropdowns (see below for why it's canvas-drawn
instead of a native menu bar).

## Shared library fixes (these affect every GUI app in the repo, not just this one)

While bringing the GUI up on Linux/X11, found and fixed several real,
previously-silent bugs in `full_application_window`
(`_deliverables/libraries/groups/app_builder/OS_GENERICS/full_application_window/`):

- `present_canvas()`, `client_size()`, and `query_pointer_state()` were
  `#if defined(_WIN32)`-only — on X11 they were no-op stubs, meaning any
  Linux GUI app relying on them (mouse input, canvas rendering, window
  sizing) silently did nothing. Implemented real X11 paths (XPutImage with
  visual-mask-based pixel packing, XGetWindowAttributes, XQueryPointer).
- `request_redraw()`'s X11 path called `XClearArea()` then waited for the
  next Expose event to actually redraw, producing a clear → (blank gap) →
  redraw "strobe" where the window was blank almost all the time. Fixed to
  render synchronously, matching the Cocoa/headless backends.
- Replaced the library's hand-rolled 5x7 bitmap font with the public-domain
  `font8x8_basic` font for legibility (this fix lives on the movie_editor
  branch's copy of `charts`; see the quantum_simulator notes for the
  separate X11-fix porting work done on that branch).
- Added `WindowConfig::min_width/min_height`, enforced via X11
  `XSetWMNormalHints` (`PMinSize`).
- The native OS menu bar (`set_menu_bar()`) only renders on Win32 — it's a
  complete no-op on X11/Cocoa. Built a genuine canvas-drawn topbar
  (File/Edit/Help dropdowns, click-to-open/click-away-to-dismiss) as a
  working substitute on Linux, backed by a single shared `menu_definitions()`
  data source used by both the native menu and the canvas topbar.

## Iteration based on direct user feedback

- Bumped default window size and enforced a larger minimum size.
- Added version embedding (git short hash + semantic version) via
  `--version` CLI flag and a Help menu item.
- Bumped ruler/preview timecode text scale for legibility.
- Added topbar menu items for binary version, importing video/audio/stills,
  and exporting video/audio.
- Bumped dropdown menu item text from scale=1 to scale=2 (inconsistent with
  the topbar titles, which were already scale=2 — this was the root cause
  of the "menu text too small" complaint), with row height/panel width
  adjusted so nothing clips.
- Added a "Import Video File (GIF/AVI)..." menu item wired to
  `MediaBin::import_video_file()`, addressing the request to import actual
  video files (as opposed to only image-sequence folders).

## Verification

Full project rebuild + full `ctest` run both clean at every checkpoint
(109/109 passing by the end). GUI verified end-to-end via X11 screenshot
capture and simulated-click testing (track add/remove, topbar dropdown
open/close, version dialog, import error handling).
