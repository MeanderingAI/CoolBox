# Python Purchase SDist HPP Headers

## Summary
- Fixed the Python purchase package source distribution so C++ binding headers with the `.hpp` extension are included in the published sdist.
- Removed the packaging gap that caused the Ubuntu wheel build to fail while compiling `py_ml_core.cpp`.
- Updated the recorded Python binding source list so the `graphics_misc` binding header is present in packaged sources.

## Problem
- The Ubuntu purchase-python workflow builds the wheel from an sdist rather than directly from the repository checkout.
- `py_ml_core.cpp` includes `graphics_misc/bindings.hpp`, which exists in `_libraries/python_bindings/include/graphics_misc/` in the repository.
- `_libraries/python_bindings/MANIFEST.in` included only `*.h` files under `include/`, so `.hpp` binding headers were omitted from the sdist.
- As a result, the wheel build failed on Ubuntu with `fatal error: graphics_misc/bindings.hpp: No such file or directory`.

## Files Updated
- `_libraries/python_bindings/MANIFEST.in`
- `_libraries/python_bindings/ml_toolbox.egg-info/SOURCES.txt`

## Change
- Updated the Python bindings manifest to include both `*.h` and `*.hpp` files under the `include/` tree.
- Added `include/graphics_misc/bindings.hpp` to the recorded package source list used by the Python binding metadata.
- Left the rest of the binding packaging layout unchanged so the fix stays scoped to the missing-header sdist regression.

## Result
- Ubuntu purchase-python sdists now include the C++ binding headers required by `py_ml_core.cpp`.
- The wheel build no longer fails solely because `graphics_misc/bindings.hpp` is absent from the packaged source archive.
- Future Python binding headers added under `_libraries/python_bindings/include/` with a `.hpp` extension will now ship in the sdist automatically.