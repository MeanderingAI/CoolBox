# Python Vendor Sync Source Path Fix (v1.5.0)

## Issue

Python bindings CI failed in `setup.py` during vendor sync with:

- `FileNotFoundError: Required vendored file not found: .../vendor_include/GRAPHICS/charts/headers/graphics.h`

## Root cause

`setup.py` still sourced graphics/wave vendor files from legacy `_libraries/packages/...` paths, while the active repository layout uses `_deliverables/libraries/groups/...`.

## Fix

Updated `_deliverables/libraries/bindings/python_bindings/setup.py` to resolve source files using candidate paths:

- prefer `_deliverables/libraries/groups/...`
- fallback `_libraries/packages/...` for backward compatibility

Applied to both headers and source files used by vendor sync:

- `GRAPHICS/charts/headers/graphics.h`
- `MISC/wave_generator/headers/wave_generator.hpp`
- `GRAPHICS/charts/source/graphics.cpp`
- `MISC/wave_generator/source/wave_generator.cpp`

## Impact

Prevents vendor sync failure on fresh CI checkouts where vendored files are not pre-populated and legacy `_libraries/packages` paths are absent.
