# Fix Ubuntu Python bindings build: add vendor headers to CMake include path

## Problem
- Ubuntu pipeline failed for Python bindings: fatal error `MISC/wave_generator/headers/wave_generator.hpp: No such file or directory`.
- Multiple C++ warnings were also reported (e.g., -Wsign-compare, -Wunused-variable, -Wreorder).

## Root Cause
- The CMakeLists.txt for Python bindings did not add the vendor header directories (`vendor_include/GRAPHICS/charts/headers`, `vendor_include/MISC/wave_generator/headers`) to the include path.
- Source files include these headers directly, so the build fails if the include path is missing.

## Solution
- The fix is the same as for macOS: add the following lines to `_libraries/python_bindings/CMakeLists.txt`:

```cmake
include_directories(
    ${CMAKE_SOURCE_DIR}/vendor_include/GRAPHICS/charts/headers
    ${CMAKE_SOURCE_DIR}/vendor_include/MISC/wave_generator/headers
)
```
- This ensures all platforms (including Ubuntu) can find the required headers.
- See also: `fix-python-macos-vendor-includes.md` for details.

## Warnings
- The C++ warnings (e.g., -Wsign-compare, -Wunused-variable, -Wreorder) are present in the codebase and should be addressed in future cleanups.
- No new warnings were introduced by this fix.

## Status
- Fixed in v1.1.58_v1.1.59.
- Build should now succeed on Ubuntu and other platforms.
- See also: `fix-python-macos-vendor-includes.md` for the related macOS fix.
