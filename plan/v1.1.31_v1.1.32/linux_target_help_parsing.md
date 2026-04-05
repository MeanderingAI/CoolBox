# Linux Target Help Parsing (v1.1.31 -> v1.1.32)

## Summary
This document records the Linux build-workflow fix applied during the v1.1.31 to v1.1.32 transition for false skipped-library warnings in `build_libs`.

## Issue Addressed

### Library targets were falsely reported as missing on Linux
- The `Build all libraries` step in `.github/workflows/build-libs.yaml` compared source-declared library names against parsed output from `cmake --build . --target help`.
- On Linux Makefile-style generators, the help output lists targets in a formatted style such as `... target_name`.
- The workflow parser was taking the wrong token from those lines, so real configured targets were interpreted as missing.
- This produced misleading warnings for valid libraries including `data_structures`, `json`, `sql`, `fuzzer`, and many other backages.

## Why This Was Not A Source-CMake Difference
- The reported targets were still being added normally through `_libraries/CMakeLists.txt`.
- `data_structures`, `json`, `fuzzer`, and other reported libraries are all included through the same top-level aggregation process.
- The warning was caused by generator-specific target-help formatting, not by those package `CMakeLists.txt` files being excluded from the configure step.

## Change Implemented
- Updated the workflow logic in `.github/workflows/build-libs.yaml` to extract configured Linux target names from the actual `... target_name` format emitted by `cmake --build . --target help`.
- Preserved the comparison against source-declared `add_library(...)` targets, but fixed the configured-target parser so the comparison uses the correct names.

## Primary File Updated
- `.github/workflows/build-libs.yaml`

## Result
- The Linux skipped-library warning should now reflect genuinely missing configured targets rather than parser artifacts.
- The workflow now treats Linux generator output consistently with the actual target names produced during configuration.