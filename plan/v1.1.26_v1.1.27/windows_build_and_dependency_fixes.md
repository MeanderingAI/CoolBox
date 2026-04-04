# Windows Build and Dependency Fixes (v1.1.26 -> v1.1.27)

## Summary
This document records the Windows build-system and dependency fixes completed during the v1.1.26 to v1.1.27 transition.

## Issues Resolved

### 1. Makefile.win execution flow
- Repaired broken target structure in `Makefile.win`.
- Fixed PowerShell quoting and variable escaping issues.
- Updated generator detection to support Visual Studio 18 2026.
- Changed `configure` to always re-run CMake so stale partial caches do not mask failed configuration.
- Added a logged test target: `make -f .\Makefile.win test_logged`.

### 2. vcpkg detection and toolchain usage
- Unified vcpkg detection across:
  - `Makefile.win`
  - `cmake/FindAllDependencies.cmake`
  - `_scripts/install_vcpkg.ps1`
- Added support for these Windows lookup locations:
  - `VCPKG_ROOT`
  - `%USERPROFILE%\vcpkg`
  - `%LOCALAPPDATA%\vcpkg`
- Ensured the vcpkg install script sets `VCPKG_ROOT` and updates user `PATH`.

### 3. Dependency resolution
- Installed and validated:
  - `gtest:x64-windows`
  - `gsl:x64-windows`
  - `sqlite3:x64-windows`
- Moved dependency checks in top-level `CMakeLists.txt` so they run after `cmake_minimum_required()` and `project()`.
- Fixed GSL enablement logic so `ENABLE_GSL=ON` is respected correctly.

### 4. CMake/package structure fixes
- Added `PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}` to the top-level export install block to remove packaging warnings.
- Suppressed repo-controlled configure noise from docs and Eigen integration.
- Patched fetched Eigen in the workspace to remove the remaining deprecation warning caused by its old `cmake_minimum_required` declaration.

## Files Touched
- `Makefile.win`
- `CMakeLists.txt`
- `cmake/FindAllDependencies.cmake`
- `cmake/ExternalDependencies.cmake`
- `_scripts/install_vcpkg.ps1`
- `_scripts/run_release_tests.ps1`
- `_libraries/CMakeLists.txt`
- `_libraries/gen_docs/CMakeLists.txt`
- `build/eigen-src/CMakeLists.txt`

## Result
- `make -f .\Makefile.win build_libraries` completes successfully on Windows.
- Dependency configuration for GTest, GSL, and SQLite3 is stable.
