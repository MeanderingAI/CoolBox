# Python Bindings Graphics Header Fix

## Summary
- Fixed the Python graphics bindings to compile against the real graphics and component APIs instead of an incomplete local stub.
- Replaced the stub include in the bindings implementation with the actual chart and component headers from the repository.

## Problem
- `_libraries/python_bindings/src/graphics_misc/bindings.cpp` included `graphics_stub_clean.h`.
- That stub redeclared a reduced `graphics::Canvas` type with only minimal accessors.
- The bindings code attempted to expose real `graphics::Canvas` methods such as `set_pixel`, `draw_line`, `draw_rect`, `draw_circle`, `draw_text`, `save_bmp`, `save_png`, and `save_jpg`.
- During the `generate_purchase_python` build, pybind11 compiled against the stub type instead of the real API and failed with missing-member errors.

## Files Updated
- `_libraries/python_bindings/src/graphics_misc/bindings.cpp`

## Result
- The Python bindings now include the real graphics declarations from:
  - `_libraries/packages/GRAPHICS/charts/headers/graphics.h`
  - `_libraries/packages/GRAPHICS/components/headers/components.hpp`
- The bindings compile against the same `graphics::Canvas` API that the repository implementation provides.
