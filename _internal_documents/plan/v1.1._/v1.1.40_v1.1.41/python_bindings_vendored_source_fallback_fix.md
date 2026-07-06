# Python Bindings Vendored Source Fallback Fix

## Summary
- Hardened the Python bindings build so `charts` and `wave_generator_utils` no longer remain hard link dependencies when their prebuilt native libraries are missing from the staged CI build tree.
- Added a fallback that compiles the vendored implementation sources directly into the extension when those libraries cannot be resolved from `COOLBOX_LIB_DIR`.

## Problem
- The earlier recursive library search fixed the flat-path assumption, but the macOS CI failure still showed the final extension link step using unresolved `-lcharts` and `-lwave_generator_utils`.
- That behavior means the expected native libraries were still unavailable to `setup.py` at build time, even after searching recursively beneath the downloaded build artifact.
- On macOS this left the extension build failing with linker errors such as:
  - `ld: library 'charts' not found`
  - `clang++: error: linker command failed with exit code 1`

## Files Updated
- `_libraries/python_bindings/setup.py`

## Result
- `setup.py` still prefers linking against resolved CMake-produced libraries when they exist.
- If `charts` or `wave_generator_utils` cannot be resolved, the build now falls back to compiling the vendored source files already tracked in the Python bindings tree.
- This removes the remaining dependency on artifact staging layout for those two libraries and prevents the build from emitting unresolved `-lcharts` or `-lwave_generator_utils` flags when the libraries are absent.
- The fallback is targeted and only applies to the specific libraries that already have vendored source equivalents in the Python bindings package.
