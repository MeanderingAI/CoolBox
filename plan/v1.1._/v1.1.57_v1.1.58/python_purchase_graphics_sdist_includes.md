# Python Purchase Graphics SDist Includes

## Summary
- Fixed the Python graphics bindings so sdist builds no longer depend on repository-relative `GRAPHICS` headers that are absent from the published source archive.
- Added packaged graphics component headers to the Python bindings include tree so `src/graphics_misc/bindings.cpp` can compile from an sdist on Linux.
- Repointed the graphics bindings source to use shipped include paths instead of `../../../packages/...` paths.

## Problem
- The Ubuntu purchase-python wheel build compiles `src/graphics_misc/bindings.cpp` from an sdist.
- That source file still included `../../../packages/GRAPHICS/charts/headers/graphics.h` and `../../../packages/GRAPHICS/components/headers/components.hpp`.
- Those repository-relative paths do not exist inside the packaged sdist, so the build failed with `fatal error: ../../../packages/GRAPHICS/charts/headers/graphics.h: No such file or directory`.
- The package already shipped `graphics.h`, but it did not ship the required graphics component headers used by the bindings source.

## Files Updated
- `_libraries/python_bindings/src/graphics_misc/bindings.cpp`
- `_libraries/python_bindings/include/GRAPHICS/graphics_object.hpp`
- `_libraries/python_bindings/include/GRAPHICS/components/headers/components.hpp`
- `_libraries/python_bindings/ml_toolbox.egg-info/SOURCES.txt`

## Change
- Updated `bindings.cpp` to include packaged Python bindings headers under `GRAPHICS/...` and `MISC/...` rather than repository-relative package paths.
- Added a packaged copy of `graphics_object.hpp` to the Python bindings include tree.
- Added a packaged copy of the graphics component model declarations required by the Python bindings to `include/GRAPHICS/components/headers/components.hpp`.
- Updated the Python bindings source list so those packaged graphics headers are included in the published source archive.

## Result
- Ubuntu purchase-python sdists now contain the graphics headers required by `src/graphics_misc/bindings.cpp`.
- The Python wheel build no longer fails solely because the graphics bindings source references headers outside the sdist.
- The Python graphics bindings source is now aligned with the packaged include layout instead of the repository checkout layout.