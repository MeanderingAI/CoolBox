# Fix macOS Python bindings build: vendor header include path

## Problem
- macOS pipeline for Python bindings fails with:
  - `fatal error: 'MISC/wave_generator/headers/wave_generator.hpp' file not found`
- This is the same root cause as Ubuntu and Windows: missing vendor header include path in CMake.

## Solution
- Add the following lines to the relevant CMakeLists.txt for the Python bindings:

```cmake
include_directories(
    ${CMAKE_SOURCE_DIR}/vendor_include/GRAPHICS/charts/headers
    ${CMAKE_SOURCE_DIR}/vendor_include/MISC/wave_generator/headers
)
```
- Or, if using target-based CMake (recommended):

```cmake
target_include_directories(ml_core PRIVATE
    ${CMAKE_SOURCE_DIR}/vendor_include/GRAPHICS/charts/headers
    ${CMAKE_SOURCE_DIR}/vendor_include/MISC/wave_generator/headers
)
```
- This ensures all platforms (including macOS) can find the required headers.
- See also: `fix-python-ubuntu-vendor-includes.md`, `fix-python-windows-missing-deps.md` for related fixes.

## Status
- Pending in v1.1.59_v1.1.60.
- Apply this fix to resolve the macOS build error for Python bindings.
