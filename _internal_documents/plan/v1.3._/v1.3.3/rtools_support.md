# Rtools Support — v1.3.3

Add Rtools (the gcc toolchain required to compile R packages with C++ sources on Windows) as a
recognised, installable, and warnable tool — matching the existing pattern for Go, Maven, V, c3c,
and R.

---

## Problem

R packages that contain compiled C++ code (like `coolboxr`) require Rtools to be present on
Windows. Without it, `R CMD INSTALL` fails with an opaque "gcc not found" error rather than a
clear, actionable message. The GUI Install Tools panel and the build pipeline had no awareness of
Rtools.

---

## Files Changed

### 1. `_scripts/install_scripts/master_installer.py`

- Added `_rtools_dir()` — scans all known Rtools locations:
  - System: `C:\rtools45\...`, `C:\rtools44\...`, `C:\rtools43\...`, `C:\rtools42\...`
  - User-home managed: `~\rtools45\x86_64-w64-mingw32.static.posix\bin`
- Added `_rtools_installed()` — returns `True` if any gcc.exe is found.
- Added `install_rtools()` — downloads `rtools45-6768-6492.exe` (~156 MB) from CRAN and
  installs silently to `~/rtools45` (no admin/UAC required):
  ```
  rtools45.exe /VERYSILENT /NORESTART /DIR=<home>\rtools45
  ```
- Added `"rtools"` entry to the `TOOLS` registry dict with label `"Rtools (gcc for R)"`.
- Added `if tool_key == "rtools": return _rtools_installed()` branch to `_is_installed()`.

### 2. `_interfaces/GUI/main.py` — `list_tools()` (`GET /extensions/tools`)

- Added `"rtools"` to the `TOOLS` dict with all standard gcc.exe candidate paths as `extra`
  entries (same candidates as `_rtools_dir()`).
- Added `"rtools"` to the `VALID_TOOLS` set in `install_tool()` (`POST /extensions/install`).

### 3. `_interfaces/GUI/main.py` — `GET /extensions` (per-binding response)

- Added `has_rtools = None` alongside `has_r_exec`.
- When `has_r` is `True` and running on Windows (`os.name == "nt"`), probes all known
  `gcc.exe` paths; sets `has_rtools = True` on Linux/macOS (system gcc assumed present).
- `has_rtools` included in the per-binding response dict so the frontend can show warnings.

### 4. `_scripts/build_scripts/build_extensions.py`

- Added `_find_rtools_gcc()` — scans the same candidate paths as `_rtools_dir()`.
- In `_r_build()`, after the R-not-found guard, added:
  ```python
  if sys.platform == "win32" and not _find_rtools_gcc():
      print("[WARN] Rtools not found. R packages with C++ sources require Rtools to compile.")
      print("       Download from https://cran.r-project.org/bin/windows/Rtools/")
      print("       or use Install Tools in the GUI to install it automatically.")
      return {"binding": ..., "success": False, "skipped": True, "error": "Rtools not found"}
  ```
  This produces a friendly `[WARN]`-prefixed skip instead of a cryptic compile failure.

### 5. `_interfaces/GUI/static/screens/package_builder/extension-builder.mjs` — `_toolchainHint()`

- Added a new branch in `_toolchainHint()`:
  ```javascript
  if (binding.build_type === 'r' && binding.has_rtools === false)
      return '⚠️ Rtools not found — use Install Tools to set up gcc for R packages';
  ```
  This banner is shown below the R binding card whenever Rtools is absent (separate from the
  existing R-not-found warning which fires when `has_r_exec === false`).

---

## Detection Priority (Rtools)

1. `C:\rtools4x\x86_64-w64-mingw32.static.posix\bin\gcc.exe` — system-wide installs
2. `C:\rtools42\mingw64\bin\gcc.exe` — older layout
3. `~\rtools45\x86_64-w64-mingw32.static.posix\bin\gcc.exe` — user-home managed install

---

## Notes

- Rtools45 installer was already downloaded to `$env:TEMP\rtools45.exe` during testing but the
  UAC-elevation path (`/DIR=C:\rtools45`) hung. The user-home install path avoids UAC entirely.
- On Linux/macOS `gcc` is part of the system toolchain; `has_rtools` is always `True` there.
