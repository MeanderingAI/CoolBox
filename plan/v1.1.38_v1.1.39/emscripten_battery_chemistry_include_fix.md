# Emscripten Battery Chemistry Include Fix

## Summary
- Fixed the Emscripten battery bindings target so it can resolve the chemistry headers required by the battery model.
- Added the chemistry include root to the `battery_js` target in the Emscripten bindings CMake file.

## Problem
- `battery_bindings.cpp` includes `battery.h`, which includes `cell.h`.
- `cell.h` depends on `chemistry/periodic_table.h`.
- The `battery_js` target only added the battery and curcuitry include directories, but not the chemistry include directory.
- As a result, the `generate_purchase_js` Emscripten build failed with:
  - `fatal error: 'chemistry/periodic_table.h' file not found`

## Files Updated
- `_libraries/emscripten_bindings/CMakeLists.txt`

## Result
- `battery_js` now includes `_libraries/backages/CHEMISTRY/include` during the Emscripten build.
- Local reproduction remains possible using `emcmake` and `cmake --build build-emscripten --target battery_js` on Linux, macOS, or WSL.
