# Workflow Test And Target Detection (v1.1.30 -> v1.1.31)

## Summary
This document records the CI workflow fixes applied to target discovery and test detection during the v1.1.30 to v1.1.31 transition.

## Issues Addressed

### 1. Build all tests failed on macOS runners
- The `Build all tests` step in `.github/workflows/build-libs.yaml` used `mapfile`, which is not reliably available in the Bash version provided by GitHub macOS runners.
- This caused the workflow to stop with `mapfile: command not found` before test targets could be processed.

### 2. Library targets were falsely reported as skipped
- The `Build all libraries` step extracted library targets from source `CMakeLists.txt` files and compared them against raw `cmake --build . --target help` output.
- The comparison logic depended on output formatting and produced false positives for real configured targets such as `json`, `sql`, and other libraries.

### 3. Test target detection was too narrow
- The workflow assumed configured test build targets would follow a `*_tests` naming convention.
- This repository contains mixed test naming styles including `*_tests`, `*_test`, and executable names that are only surfaced through CTest registrations.
- That caused the workflow to report `No configured *_tests targets were found in the generated build system` even when tests were actually configured.

## Changes Implemented

### 1. Portable shell handling for test iteration
- Replaced the `mapfile`-based collection logic with a portable read-loop approach that works across GitHub Linux and macOS runners.

### 2. Normalized configured library target matching
- Updated the library build step to normalize configured build targets from CMake help output before comparing them to source-declared libraries.
- Switched the comparison to exact target-name matching instead of output-format-dependent regex matching.

### 3. Switched test detection to CTest
- Replaced the `*_tests` target-name heuristic with `ctest -N -C Release` discovery.
- The workflow now checks whether configured CTest entries exist rather than guessing based on build target names.
- When tests are present, the workflow performs a normal build for the configured generator and lets the `Run tests` step execute the discovered tests.

## Primary File Updated
- `.github/workflows/build-libs.yaml`

## Result
- The `Build all tests` step no longer depends on unsupported shell builtins on macOS runners.
- The library-skipped warning should now only report targets that are genuinely absent from the generated build system.
- Test detection now follows configured CTest entries, which matches the mixed naming patterns used throughout the repository.