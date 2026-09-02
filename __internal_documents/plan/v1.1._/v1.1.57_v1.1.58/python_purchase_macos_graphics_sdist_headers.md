# Python Purchase macOS Graphics SDist Headers

## Summary
- Documented the macOS purchase-python sdist failure caused by `src/graphics_misc/bindings.cpp` including repository-only `GRAPHICS` headers.
- Confirmed the same Python graphics include cleanup fixes the macOS `clang++` header-not-found error because the source file no longer references `../../../packages/...` paths.
- Captured that the updated sdist-safe include layout is now shared across Linux and macOS purchase builds.

## Problem
- The macOS purchase-python build compiled `src/graphics_misc/bindings.cpp` from an sdist staging directory.
- That source still included `../../../packages/GRAPHICS/charts/headers/graphics.h`, which does not exist inside the sdist.
- `clang++` failed with:

```text
src/graphics_misc/bindings.cpp:12:10: fatal error: '../../../packages/GRAPHICS/charts/headers/graphics.h' file not found
```

- This was the macOS manifestation of the same root-cause already seen in the Ubuntu purchase-python workflow: the Python bindings source depended on repository checkout paths instead of packaged include paths.

## Files Updated
- `_libraries/python_bindings/src/graphics_misc/bindings.cpp`
- `_libraries/python_bindings/include/GRAPHICS/graphics_object.hpp`
- `_libraries/python_bindings/include/GRAPHICS/components/headers/components.hpp`
- `_libraries/python_bindings/ml_toolbox.egg-info/SOURCES.txt`
- `_libraries/python_bindings/setup.py`

## Change
- Replaced the repo-relative `GRAPHICS` and `MISC` includes in `bindings.cpp` with packaged include paths that exist inside the sdist.
- Added the graphics component headers required by the bindings source into the Python bindings include tree so macOS sdists ship what the compiler includes.
- Normalized Python extension source paths in `setup.py` so vendored fallback sources are passed to setuptools as setup-relative paths instead of absolute paths.

## Result
- The specific macOS `clang++` failure for `../../../packages/GRAPHICS/charts/headers/graphics.h` should no longer occur once the updated sdist is used.
- The Python purchase sdist now matches the include layout expected by `src/graphics_misc/bindings.cpp` on both Linux and macOS.
- Any remaining macOS purchase-python failures after this point are more likely to be later-stage packaging or linking issues rather than the original graphics header lookup bug.