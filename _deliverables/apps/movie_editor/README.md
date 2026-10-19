# CoolBox Movie Editor

A traditional non-linear video editor for CoolBox: a media bin, a multi-track
timeline (video + audio), a scrubbable preview, and export to standard
container formats. Built-in AVI/GIF/WAV encoders are complemented by FFmpeg
video import and H.264/AAC MP4 export, and miniaudio device playback.

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
  frame compositing, audio mixing, undo/redo, export to MP4/AVI/GIF/WAV, and JSON project
  save/load. See `MovieEditorCoreTests`.

The built-in encoders reuse the project's
existing `stb_image`-backed texture loader (for reading PNG/JPG/BMP frames)
and otherwise hand-roll the container/compression formats. Common video
formats are imported through an external FFmpeg installation; the editor's
readable UI uses the real stb TrueType rasterizer from the pinned stb source.

## Using the app

### Windows run and debug in VS Code

Install CMake, Visual Studio with the Desktop development with C++ workload,
and the VS Code C/C++ extension. Select **Run movie_editor** or
**Debug movie_editor (Windows)** in Run and Debug, then press F5.
Both configure and build the Debug executable before launching.
The **run-movie-editor** task uses the same Windows build.
Windows Run/Debug setup also checks for FFmpeg and ffprobe. If neither a
complete installation nor a valid `COOLBOX_FFMPEG` override is available,
it installs `Gyan.FFmpeg.Essentials` for the current user through WinGet.
This downloads software and accepts the package/source agreements. WinGet
(Microsoft App Installer) and internet access are required for installation.
Existing installations are reused; invalid overrides and installer failures
stop setup with an error rather than being silently ignored.
At runtime, the Windows editor also discovers complete FFmpeg/ffprobe pairs
on PATH and in WinGet's current-user/system package folders. Imports and
exports therefore work even when the debugger or a directly launched
executable does not inherit the setup environment. `COOLBOX_FFMPEG`, when
set, remains the explicit override.

Windows x64 builds use `build_windows`, separate from the `build` directory used
by Linux/WSL. Do not reuse a CMake cache between Windows and WSL.
The Windows helper enables `COOLBOX_MOVIE_EDITOR_ONLY`, which configures only
the editor and its existing library dependencies. GSL, Bison, Doxygen, vcpkg,
and Python bindings are not required for this focused build. Git and internet
access are needed on the first configure to fetch the pinned stb headers and
miniaudio 0.11.23 (commit `f40cf03f80cdb7e741d43e53b7e706e8c1394bcf`).
To build from PowerShell:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\_scripts\build_movie_editor_windows.ps1
.\build_windows\_deliverables\apps\movie_editor\Debug\movie_editor.exe
```

To build and run the editor core and container regression suites:

```powershell
cmake --build build_windows --config Debug --target movie_editor_core_tests movie_editor_playback_tests movie_editor_ui_tests video_container_tests audio_container_tests
ctest --test-dir build_windows -C Debug -R 'MovieEditor|VideoContainerTests|AudioContainerTests' --output-on-failure
```

### Linux and macOS

Use a separate native build directory:

```sh
cmake -S . -B build_movie_native -DCOOLBOX_MOVIE_EDITOR_ONLY=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build_movie_native --target movie_editor
./build_movie_native/_deliverables/apps/movie_editor/movie_editor
```

Linux needs GTK 3.20 or newer and X11 development headers at build time, a running X11/XWayland
display, and a working PulseAudio or ALSA runtime/device for playback.
For Debian/Ubuntu, install `libgtk-3-dev libx11-dev pkg-config libpulse0` (or the distribution's ALSA
runtime package) and `ffmpeg`. macOS uses Cocoa and CoreAudio; install FFmpeg
separately, for example with Homebrew. Playback fails with an explicit error
if no real audio output is available; a silent/null backend is never used as
a production fallback.

All three platforms use the same editing, history, transport and export
code. Windows x64 and Linux ARM64/X11 have build and regression coverage;
native macOS execution has not been verified in this development environment.

### GUI mode

```
movie_editor                      # start with an empty project
movie_editor --project my.json    # start with an existing project loaded
```

The window has a media bin (left), a preview + transport controls (center),
an inspector (right), and a multi-track timeline (bottom). Editing actions
are available through buttons, menus, mouse gestures and portable shortcuts:

- **Native menus**: File, Edit and Help use Win32 menus on Windows, the
  system menu bar on macOS, and GTK 3 menus on Linux/X11, with native fonts,
  colors and keyboard navigation. The canvas contains only quick-action
  buttons, not duplicate menu headings. Canvas text uses anti-aliased
  Segoe UI on Windows (or a system TrueType font elsewhere); set
  `COOLBOX_UI_FONT` to a `.ttf` path to override that font.
  Menu commands use the shared [GRAPHICS OS-generic menu API](../../libraries/groups/app_builder/GRAPHICS/os_generic/README.md).
- **Responsive input**: menu and button clicks are delivered as events,
  including quick clicks between rendered frames. Playback and background
  import/export continue while the application is open; shutdown waits
  for pending jobs to finish.
- **Import Media**: choose video, photos, or WAV audio. Video imports run
  in the background, with errors reported in a dialog and status bar.
- **Add Clip**: appends the selected media to a compatible track,
  creating a video/audio track if needed. Newly imported media is selected
  automatically. The timeline fits the project after adding media.
- **File menu**: open/save project, import an image-sequence folder / still
  image / video / WAV file, export MP4 / AVI / GIF / WAV.
- **Media bin drag and drop**: drag a media row directly onto a compatible,
  unlocked timeline track at the desired time. Dropping onto empty timeline
  space creates a matching video/audio track and its clip in one undoable
  operation, including when starting with no tracks. A green/red outline
  previews compatible/incompatible drops. No **Add Clip** click is needed;
  that button remains available as an alternative.
- **Delete track**: right-click a track header or lane, then choose
  **Delete Track** (or select a track and use **Edit > Delete Selected Track**).
  This deletes the track and all its clips, but keeps media in the bin.
  Undo restores the track, its clips and its position in the track order.
  Locked tracks cannot be deleted. Escape or clicking outside dismisses
  the context menu.
- **Text / Title**: one action-bar button and Edit-menu command opens the
  shared editor for titles and overlays. Toggle **Background: Transparent**
  or **Background: Color**, with adjustable background opacity (0-100%).
  Drag the text in the live preview to position it, or enter horizontal and
  vertical percentages. Click the text/background color field to select its
  target in the reusable GRAPHICS color picker: saturation/value square,
  hue strip and opacity slider. Hex colors remain editable.
  Set text, duration, font size and fade-in/out times in seconds. Font size is
  relative to a 720p frame and scales with the preview/export resolution.
  Click a field to replace it, Tab/Shift+Tab to move between fields,
  Shift+Enter for a line break, Enter/**Apply** to commit, or Escape/**Cancel**
  to discard. Text is rendered with a system TrueType font.
  A new text clip starts at the playhead on a new top video track; existing
  clips do not shift. The editor preview shows the draft without fades and
  does not alter the timeline until Apply; Cancel discards all draft changes.
  Applying settings previews the middle of the text clip,
  rather than its fully faded-out first frame. Select it and choose **Edit Text / Title** in the
  inspector (or **Edit > Edit Selected Text**) to change its settings.
  Titles and overlays are saved in the project, appear in playback and all
  video exports, and support move/trim/split/delete and Undo/Redo.
- **Timeline lane**: click empty space with a pending media item selected to
  drop a clip there; click an existing clip to select it (for Split/Delete
  and the inspector); click the track header to just select the track.
- **Move and trim**: drag a clip's body to move it, including between
  matching unlocked tracks. Drag its white edge handles to trim/extend
  within the source bounds. The gesture shows a preview and commits only
  on release; Escape or focus loss cancels it. Horizontal edits snap to
  24 fps increments. Overlaps are allowed; this is not a magnetic/ripple timeline.
- **Undo/Redo**: buttons, Edit menu, or shortcuts restore the last 100
  timeline operations. A drag, including a drop that creates a track, is
  one operation. Adding a track manually is a separate operation. Importing media is not undone;
  media remains available when undo removes its timeline clip.
- **Ruler**: click to move the playhead.
- **Preview/export framing**: media keeps its original aspect ratio, with
  black bars where needed instead of stretching.
- **Timeline zoom**: use `-`, `+`, and **Fit** in the action bar. Use the
  mouse wheel over the media bin or track lanes to scroll those lists;
  Shift+wheel over the lanes pans horizontally when zoomed in.
- **Transport buttons**: `|<` / `Play`/`Pause` / `Stop` / `>|` / `Split` /
  `Delete` / `+Video Trk` / `+Audio Trk`.
- **Playback**: video follows the audio device's consumed-sample clock,
  adjusted for backend buffering. Embedded video audio and WAV tracks are
  mixed to 48 kHz stereo. Audio is produced off the real-time device
  callback, with silence on buffer underrun rather than an advancing video
  clock. Pause, seek, edit and undo stop the old stream; Play restarts at
  the current playhead.
- **Export**: the action-bar Export button writes a 1280x720, 24 fps MP4
  with H.264 video, AAC stereo audio and fast-start metadata. Encoding runs
  in the background; other editing actions wait until it finishes. A
  failed encode leaves an existing destination untouched, and source
  media cannot be overwritten.

| Shortcut | Action |
|----------|--------|
| Space | Play/pause |
| Left / Right | Step one 24 fps frame |
| Home / End | Timeline start/end |
| Enter | Add selected media |
| Delete / Backspace | Delete selected clip |
| Ctrl/Cmd+B | Split the clip at the playhead on the selected track |
| Ctrl/Cmd+Z | Undo |
| Ctrl/Cmd+Shift+Z or Ctrl+Y | Redo |
| Ctrl/Cmd+I / O | Import media / open project |
| Ctrl/Cmd+S / Ctrl/Cmd+Shift+S | Save / save as |
| Ctrl/Cmd+E | Export MP4 |
| Escape | Cancel drag or dismiss menu |

The Help menu also lists shortcuts. Existing project JSON files remain
compatible; undo/redo history is session-only and resets when opening a
project.

Imported image sequences default to 24 fps and still images default to a 5s
duration (there's no on-canvas text input, so these aren't currently
editable from the GUI — edit the saved project JSON directly if you need a
different value).

### FFmpeg video import

Windows Run/Debug setup installs FFmpeg **and ffprobe** automatically when
missing. It discovers the WinGet installation even when VS Code has an
outdated PATH, and supplies its location to both run tasks and the debugger
without requiring a restart. The debugger reads the generated
`build_windows/movie_editor.env`; it is regenerated by the pre-launch task.
Launching the executable outside these setup tasks does not install software.
For manual installation on Windows:

```powershell
winget install --id Gyan.FFmpeg.Essentials --exact --source winget
```

Alternatively, set `COOLBOX_FFMPEG` to the absolute path of `ffmpeg.exe`
before starting VS Code/the editor; `ffprobe.exe` must be in the same folder.
MP4, MOV, M4V, MKV, WebM, and compressed AVI imports depend on codecs in
that FFmpeg build. GUI video imports use FFmpeg, while CLI/project loading
can still read GIF and uncompressed AVI without it.

FFmpeg creates original-resolution, losslessly compressed **PNG** editing frames at **24 fps**, plus
48 kHz stereo PCM audio when present, in a uniquely named temporary folder.
Frames are loaded on demand rather than retaining the entire video in RAM.
Temporary files are removed when their media assets are released. Import
can take time; PNG caches use less space for compressible footage without
reducing resolution or image quality. Compression depends on content:
noisy footage may still need substantial temporary disk space. Audio uses
about 11 MiB per minute when present. Saved projects reference the original media, not the temporary
files; reopening regenerates the editing frames. Source files must remain
available. Embedded video audio is included in playback and MP4/WAV/AVI exports.

For the optional real-decoder regression test, generate a one-second red
MP4 with audio, then set `COOLBOX_TEST_VIDEO` to its absolute path before
running `MovieEditorCoreTests`. Without that fixture this one integration
test is skipped; the other tests still run.
Set `COOLBOX_FFMPEG` to enable the MP4 encoder integration test as well.
Playback tests always exercise buffering/seek/stop with an explicitly
selected null device. Set `COOLBOX_TEST_SYSTEM_AUDIO=1` to additionally
exercise the real system device (plays a short quiet test tone); it fails,
rather than skips, if no working device is available.

```powershell
ffmpeg -y -f lavfi -i "color=c=red:s=64x48:r=24:d=1" -f lavfi -i "sine=frequency=440:duration=1" -c:v libx264 -pix_fmt yuv420p -c:a aac -shortest build_windows\test_clip.mp4
$env:COOLBOX_TEST_VIDEO = (Resolve-Path build_windows\test_clip.mp4).Path
ctest --test-dir build_windows -C Debug -R "^MovieEditorCoreTests$" --output-on-failure
```

### Headless / CLI mode

For scripting or CI, render an existing project without opening a window:

```
movie_editor --cli --project my.json \
  --export-mp4 out.mp4 \
  --export-avi out.avi --fps 24 --width 1280 --height 720 \
  --export-gif preview.gif --gif-start-us 0 --gif-end-us 5000000 \
  --export-wav out.wav --wav-encoding adpcm
```

Any `--export-*` flag implies headless mode even without `--cli`. Run
`movie_editor --help` for the full flag list.
CLI dimensions retain their existing 640x360 default; use `--width 1280
--height 720` for 720p. MP4 accepts finite frame rates from 1 to 120 and
even dimensions from 2x2 through 7680x4320, and requires an FFmpeg build
with the `libx264` and AAC encoders. MP4 audio is always 48 kHz stereo.

## Known limitations

- Video track compositing is simple front-to-back priority (the topmost
  unmuted track with a clip at the playhead wins) — there's no cross-fade/
  alpha blending between video tracks yet.
- Speed changes resample audio with the video and change its pitch;
  pitch-preserving time stretching is not implemented. Audio scrubbing
  while paused is not implemented.
- Waveforms, titles, transitions, color correction, stabilization,
  background proxy generation and a magnetic/ripple timeline are not
  implemented. This is a core editing milestone, not full iMovie feature parity.
  Imports use 24 fps editing frames, not a source-frame-rate-preserving pipeline.
- Import and MP4 export cannot currently be cancelled; closing waits for
  the worker to finish. MP4 video is streamed directly into FFmpeg through
  a bounded operating-system pipe; no raw-video temporary file is created.
  Only PCM audio (about 11 MiB per minute) and a compressed MP4 staging
  file are written, in addition to any import caches. The staging MP4 lives
  beside the destination and is published only after encoding succeeds.
  MP4 audio is mixed in chunks, but each decoded
  source WAV is still cached in memory; long audio sources can use substantial RAM.
- `import_image_sequence`/`import_still_image` fps/duration are fixed
  defaults in the GUI (editable via the project JSON or future dialog work).
