# Fix Ubuntu Python bindings build error in generate_purchase_python: vendor header include path

## Problem
- Ubuntu pipeline for `generate_purchase_python` fails with:
  - `fatal error: MISC/wave_generator/headers/wave_generator.hpp: No such file or directory`
- This is the same root cause as previous macOS/Ubuntu failures: missing vendor header include path in CMake.

## Root Cause
- The CMakeLists.txt for the Python bindings (or for the `generate_purchase_python` target) does not add the vendor header directories (`vendor_include/GRAPHICS/charts/headers`, `vendor_include/MISC/wave_generator/headers`) to the include path.
- Source files include these headers directly, so the build fails if the include path is missing.

## Solution
- Add the following lines to the relevant CMakeLists.txt (for `generate_purchase_python`):

```cmake
include_directories(
    ${CMAKE_SOURCE_DIR}/vendor_include/GRAPHICS/charts/headers
    ${CMAKE_SOURCE_DIR}/vendor_include/MISC/wave_generator/headers
)
```
- This ensures all platforms (including Ubuntu) can find the required headers.
- See also: `fix-python-macos-vendor-includes.md` and `fix-python-ubuntu-vendor-includes.md` for details.

## Status
- Pending in v1.1.59_v1.1.60.
- Apply this fix to the CMake configuration for `generate_purchase_python` to resolve the error.
