# Python Purchase macOS SDist Headers

## Related Note
- Follow-on change to `plan/v1.1.56_v1.1.57/python_purchase_sdist_hpp_headers.md`, which documented the underlying Python binding sdist header-packaging fix.

## Summary
- Recorded that the same Python sdist manifest fix for `.hpp` binding headers also resolves the macOS purchase-python wheel failure.
- Confirmed the macOS error is not platform-specific compiler behavior, but the same missing packaged header seen on Ubuntu.
- Kept the remediation scoped to the Python binding source distribution rather than adding a macOS-only build workaround.

## Problem
- The macOS purchase-python workflow builds the wheel from the Python sdist and failed while compiling `py_ml_core.cpp`.
- The failing include was again `graphics_misc/bindings.hpp`.
- That header exists in `_libraries/python_bindings/include/graphics_misc/` in the repository, but was previously omitted from the packaged sdist because `_libraries/python_bindings/MANIFEST.in` only included `*.h` files under `include/`.
- As a result, the macOS wheel build failed with `fatal error: 'graphics_misc/bindings.hpp' file not found`.

## Files Covered By The Fix
- `_libraries/python_bindings/MANIFEST.in`
- `_libraries/python_bindings/ml_toolbox.egg-info/SOURCES.txt`

## Change
- Reused the existing Python packaging fix that includes both `*.h` and `*.hpp` files from `_libraries/python_bindings/include/` in the sdist.
- Reused the existing source-list update that explicitly records `include/graphics_misc/bindings.hpp` in the packaged Python binding sources.
- Did not add any macOS-only compiler, include-path, or wheel-build changes because the failure was caused by the packaged source archive contents.

## Result
- macOS purchase-python sdists now include the same required `.hpp` binding header that was missing on Ubuntu.
- The macOS wheel build should no longer fail solely because `graphics_misc/bindings.hpp` is absent from the packaged source archive.
- The Python purchase packaging fix is now documented as a cross-platform sdist correction affecting both Ubuntu and macOS purchase builds.