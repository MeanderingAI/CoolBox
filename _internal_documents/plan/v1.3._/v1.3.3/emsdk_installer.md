# Emscripten SDK (emsdk) Installer — v1.3.3

## Summary
Added Emscripten SDK (`emsdk` / `emcc`) as a first-class managed toolchain alongside Go, Maven, C3, V, and R. Users can install it from the Extensions tab → Install Tools panel without leaving the dashboard.

## Files Changed

### `_scripts/install_scripts/master_installer.py`
- Added `_emsdk_dir()` — returns `_local_build_pipeline/tmp/installers/emsdk`
- Added `_emcc_exe()` — locates `upstream/emscripten/emcc(.bat)` inside the managed install
- Added `install_emsdk()`:
  - Checks if `emcc` is already on `PATH` or in the managed location before downloading
  - **Prefers `git clone --depth=1`** from `https://github.com/emscripten-core/emsdk.git` if `git` is available
  - Falls back to downloading the `main` branch ZIP from GitHub if git is absent or fails
  - Runs `emsdk install latest` → `emsdk activate latest` (downloads LLVM/clang ~500 MB, output streamed live)
  - Adds `upstream/emscripten/` to `PATH` for the current process after install
  - Prints `emsdk_env.bat` / `source emsdk_env.sh` instructions for permanent shell activation
- Added `"emsdk": {"label": "Emscripten (emcc)", "fn": install_emsdk, "check": ("emcc",)}` to `TOOLS` registry
- Updated `_is_installed()` to call `_emcc_exe()` for the `emsdk` key so the managed path is checked even when `emcc` is not on `PATH`

### `_interfaces/GUI/main.py`
- `GET /extensions/tools` — added `"emsdk"` entry; checks `emcc` on PATH, managed path `_local_build_pipeline/tmp/installers/emsdk/upstream/emscripten/emcc.bat`, and `C:\emsdk\upstream\emscripten\emcc.bat`
- `POST /extensions/install` — added `"emsdk"` to `VALID_TOOLS`
- `GET /extensions` — `has_emcc` check now also tests the managed install path so binding cards unblock immediately after install without requiring a system restart

### `_scripts/build_scripts/build_extensions.py`
- `_emscripten_build()` — `_find_exec` fallback list for `emcc` and `emcmake` now checks the managed path `_local_build_pipeline/tmp/installers/emsdk/upstream/emscripten/` before falling back to `C:\emsdk\...`

### `_interfaces/GUI/static/screens/package_builder/extension-builder.mjs`
- Toolchain warning hint for `build_type === 'emscripten'` updated from _"install emsdk and activate it"_ to _"use Install Tools to set up emsdk"_ — directing users to the in-app installer panel
- No structural changes needed; `emsdk` appearing in `GET /extensions/tools` response causes the ⬇ Install button to render automatically

## Managed Install Location
```
_local_build_pipeline/tmp/installers/emsdk/
  emsdk(.bat)
  upstream/
    emscripten/
      emcc(.bat)
      emcmake(.bat)
      ...
```
