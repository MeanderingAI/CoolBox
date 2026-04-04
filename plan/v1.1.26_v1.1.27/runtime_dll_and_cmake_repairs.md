# Runtime DLL and CMake Repairs (v1.1.26 -> v1.1.27)

## Summary
This document tracks the runtime loader and malformed CMake file repairs completed during the v1.1.26 to v1.1.27 transition.

## Runtime DLL Fixes

### Graphics test executables
- `components_tests.exe` failed at runtime because `components.dll` was not placed beside the executable.
- `windows_tests.exe` failed at runtime because `components.dll` and `json.dll` were not placed beside the executable.

### Fix applied
- Added explicit Windows `POST_BUILD` copy commands for graphics tests:
  - `_libraries/backages/GRAPHICS/components/CMakeLists.txt`
  - `_libraries/backages/GRAPHICS/windows/CMakeLists.txt`
- Verified output directories contain required DLLs:
  - `components.dll`
  - `json.dll`
  - `gtest.dll`
  - `gtest_main.dll`

## App runtime copy cleanup
- Removed a broken generic runtime-DLL copy helper from `apps/lsp/CMakeLists.txt` because it generated invalid MSBuild post-build commands when `TARGET_RUNTIME_DLLS` expanded to an empty list.

## Versioning update
- Updated `_libraries/backages/GRAPHICS/windows/CMakeLists.txt` to:
  - `cmake_minimum_required(VERSION 4.3.1)`

## Malformed CMake file repairs

### Fixed duplicated or truncated package CMake files
- `_libraries/backages/ML/deep_learning/CMakeLists.txt`
- `_libraries/backages/ML/gabor_patches/CMakeLists.txt`

### Fixed package-level build issues
- Repaired broken include-guard/header corruption in electronics headers such as:
  - `_libraries/backages/ELECTRONICS/curcuitry/include/component.h`
  - `_libraries/backages/ELECTRONICS/curcuitry/include/battery.h`
- Fixed Eigen propagation in:
  - `_libraries/backages/ML/hidden_markov_model/CMakeLists.txt`
- Added tuple hashing support for MSVC sparse matrix code where needed.

## Result
- Graphics runtime loader failures are resolved.
- Previously failing Windows tests now execute correctly.
