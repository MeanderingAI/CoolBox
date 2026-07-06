# Automate C++ bridge build before Go pipeline

## Problem
Go bindings require the coolboxbridge shared library (DLL/SO/DYLIB) to be built before running Go build/test. Manual enforcement is error-prone and slows down development.

## Solution
Automate the C++ bridge build as a prerequisite for the Go pipeline:

- Update the Go build helper (`_scripts/go_build.py`) to:
  1. Check for the shared library in standard locations.
  2. If missing, automatically invoke the CMake build for the coolboxbridge target in Release mode.
  3. Retry the Go build/test after the bridge is built.
- This ensures the Go pipeline always has the required native library, reducing manual steps and errors.

## Implementation
- The script will run:
  ```
  cmake --build _libraries/go_bindings/cbridge/build --config Release --target coolboxbridge --clean-first
  ```
  if the DLL/SO/DYLIB is missing.
- After building, it will proceed with the Go command.

## Status
- [ ] Script update pending
- [ ] Test on Windows and Linux
- [ ] Documented in v1.1.61 plan

---

**Documented by GitHub Copilot, 2026-04-18**
