# CoolBox Movie Editor

A traditional non-linear video editor for CoolBox: a media bin, a multi-track
timeline (video + audio), a scrubbable preview, and export to standard
container formats — all built on hand-rolled encoders rather than a
third-party video/audio codec SDK.

## What's new in the libraries

This app's "add additional encoding for movie/audio formats" requirement was
implemented as three new, independently-tested libraries under
`_deliverables/libraries/groups/audio_visual_group/VIDEO_ASSETS/`:

- **`video_container`** — a real AVI (RIFF) muxer (`AviWriter`, uncompressed
  BGR24 video + optional interleaved 16-bit PCM audio stream, with a proper
  `idx1` index) and a from-scratch animated GIF encoder (`GifEncoder`,
  including a median-cut color quantizer and the GIF-flavoured variable-width
  LZW compressor). See `VideoContainerTests`.
- **`audio_container`** — a RIFF/WAVE reader (PCM 8/16/32-bit, IEEE float, and
  IMA ADPCM) and writer (PCM16 and a hand-rolled ~4:1 IMA/DVI ADPCM encoder,
  `WAVE_FORMAT_IMA_ADPCM`). See `AudioContainerTests`.
- **`movie_editor_core`** — the editing engine itself: a `MediaBin` (imports
  image-sequence/still-image/audio assets), `EditorProject` (wraps the
  existing `video_timeline` library's `Timeline`/`Track`/`Clip` model),
  frame compositing, audio mixing, export to AVI/GIF/WAV, and JSON project
  save/load. See `MovieEditorCoreTests`.

None of these add new third-party dependencies — they reuse the project's
existing `stb_image`-backed texture loader (for reading PNG/JPG/BMP frames)
and otherwise hand-roll the container/compression formats, consistent with
the project's preference for self-contained implementations.

## Using the app

### GUI mode

```
movie_editor                      # start with an empty project
movie_editor --project my.json    # start with an existing project loaded
```

The window has a media bin (left), a preview + transport controls (center),
an inspector (right), and a multi-track timeline (bottom). Everything is
mouse-driven (there is no cross-platform keyboard hook in
`full_application_window`, so no action requires a keyboard):

- **File menu**: open/save project, import an image-sequence folder / still
  image / WAV file, export AVI / GIF / WAV.
- **Media bin**: click a row to select it as the "pending" clip to place.
- **Timeline lane**: click empty space with a pending media item selected to
  drop a clip there; click an existing clip to select it (for Split/Delete
  and the inspector); click the track header to just select the track.
- **Ruler**: click to move the playhead.
- **Transport buttons**: `|<` / `Play`/`Pause` / `Stop` / `>|` / `Split` /
  `Delete` / `+Video Trk` / `+Audio Trk`.

Imported image sequences default to 24 fps and still images default to a 5s
duration (there's no on-canvas text input, so these aren't currently
editable from the GUI — edit the saved project JSON directly if you need a
different value).

### Headless / CLI mode

For scripting or CI, render an existing project without opening a window:

```
movie_editor --cli --project my.json \
  --export-avi out.avi --fps 24 --width 1280 --height 720 \
  --export-gif preview.gif --gif-start-us 0 --gif-end-us 5000000 \
  --export-wav out.wav --wav-encoding adpcm
```

Any `--export-*` flag implies headless mode even without `--cli`. Run
`movie_editor --help` for the full flag list.

## Known limitations

- Video track compositing is simple front-to-back priority (the topmost
  unmuted track with a clip at the playhead wins) — there's no cross-fade/
  alpha blending between video tracks yet.
- Audio clip `speed` (time-stretching) is not applied during playback/export
  — only video clips honour `speed`.
- No live audio playback during GUI scrubbing (preview is video-only); audio
  is only rendered at export time.
- The GUI timeline view is a fixed 30-second window (no zoom/scroll yet) and
  has no scrollable area for more tracks than fit on screen.
- `import_image_sequence`/`import_still_image` fps/duration are fixed
  defaults in the GUI (editable via the project JSON or future dialog work).
