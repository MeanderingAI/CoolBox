# Python Bindings Recursive Library Resolution Fix

## Summary
- Fixed the Python bindings extension build so it can locate and link the required CMake-produced CoolBox libraries from nested build output directories.
- Replaced the fragile assumption that the needed libraries live directly under the top-level `build` directory.

## Problem
- `_libraries/python_bindings/setup.py` linked the extension against `charts` and `wave_generator_utils` using:
  - a single flat `library_dirs` entry from `COOLBOX_LIB_DIR`
  - hard-coded `libraries=["charts", "wave_generator_utils"]`
- In CI, the build completed compilation successfully but failed at the final link step with:
  - `/usr/bin/ld: cannot find -lcharts`
  - `/usr/bin/ld: cannot find -lwave_generator_utils`
- The actual CMake outputs were placed in nested paths under the build tree, such as `_libraries/packages/GRAPHICS/charts` and `_libraries/packages/MISC/wave_generator`, not directly in `${GITHUB_WORKSPACE}/build`.

## Files Updated
- `_libraries/python_bindings/setup.py`

## Result
- The Python bindings setup now searches recursively under `COOLBOX_LIB_DIR` for platform-appropriate library filenames.
- When matching libraries are found, the extension links against the resolved library files directly and records their containing directories.
- On non-Windows platforms, the resolved library directories are also reused as runtime library directories for the extension build.
- The build remains compatible with explicit overrides through `COOLBOX_LIB_DIR` and `COOLBOX_LIBS`, but no longer requires a flat library output layout.
