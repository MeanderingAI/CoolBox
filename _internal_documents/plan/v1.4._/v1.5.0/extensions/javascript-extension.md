# JavaScript Extension Eigen Include Fix (v1.5.0)

## Issue

Emscripten JavaScript builds (for example `decision_tree_js`) failed with:

- `fatal error: 'Eigen/Dense' file not found`

## Root cause

`_deliverables/libraries/bindings/emscripten_bindings/CMakeLists.txt` used a single hardcoded Eigen path under `external/eigen/eigen-3.4.0`, which is not always present in CI jobs.

## Fix

Added Eigen include resolution fallback logic in emscripten bindings CMake:

1. `${COOLBOX_ROOT}/external/eigen/eigen-3.4.0`
2. `${COOLBOX_ROOT}/build/_deps/eigen-src`
3. `/usr/include/eigen3`
4. `/opt/homebrew/include/eigen3`
5. `/usr/local/include/eigen3`
6. `find_path(... Eigen/Dense ...)` fallback

If no candidate exists, configuration now fails with a clear fatal message.

## Impact

Allows JS bindings to resolve Eigen headers across vendored, fetched, and system-installed environments, removing the CI compile blocker.
