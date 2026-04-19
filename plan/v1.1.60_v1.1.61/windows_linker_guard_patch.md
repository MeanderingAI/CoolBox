# Patch Description: Guard Against Linking m (Math Library) on Windows

## Summary
This patch updates the CMake configuration for both `_libraries/c_bindings` and `_libraries/go_bindings/cbridge` to prevent linking against the Unix math library (`m` or `-lm`) on Windows, which caused linker errors during Windows builds.

## Details
- **Problem:**
  - The build system attempted to link `m.lib` (the math library) on Windows/MSVC, but this library does not exist on Windows, resulting in `LINK : fatal error LNK1181: cannot open input file 'm.lib'`.
- **Solution:**
  - Added CMake guards so that `target_link_libraries(... m)` is only invoked on non-Windows platforms (`if(NOT WIN32)` guards).
  - This change was applied to both `_libraries/c_bindings/CMakeLists.txt` and `_libraries/go_bindings/cbridge/CMakeLists.txt`.
- **Effect:**
  - Prevents linker errors on Windows/MSVC.
  - Maintains correct math library linkage on Linux/macOS.

## Motivation
This change was made to ensure cross-platform compatibility and successful builds on Windows, where math functions are provided by the C runtime and not a separate `m` library.

## Reference
- Files:
  - `_libraries/c_bindings/CMakeLists.txt`
  - `_libraries/go_bindings/cbridge/CMakeLists.txt`
- Change applied: v1.1.60 → v1.1.61
