# CI Dependency Fixes (v1.1.28 -> v1.1.29)

## Summary
This document records the CI dependency-resolution fixes completed during the v1.1.28 to v1.1.29 transition.

## Issues Addressed

### 1. Fix macOS SQLite3 discovery in CI
- Updated the custom `FindSQLite3.cmake` module so it can locate Homebrew and MacPorts SQLite installations more reliably.
- Added explicit support for Homebrew keg-style prefixes such as `/opt/homebrew/opt/sqlite` and `/usr/local/opt/sqlite`.
- Added fallback `find_path` and `find_library` lookups without the earlier narrow path restriction so CMake can still resolve SQLite3 from standard search locations.

### 2. Pass SQLite3 hints from the macOS workflow
- Updated `.github/workflows/build-libs.yaml` to install `sqlite` in the macOS dependency step.
- Exported `SQLITE3_ROOT` and `SQLite3_ROOT` from `brew --prefix sqlite` into the workflow environment.
- Passed explicit SQLite-related CMake arguments during Linux/macOS configure so the macOS runner exposes the correct prefix to the build.

### 3. Fix Windows dependency pre-check behavior
- Adjusted `cmake/FindAllDependencies.cmake` so Windows configure only pre-checks the remaining external packages that must already exist.
- Kept the dependency check informative while allowing test-target configuration to rely on the repository-owned test runtime.
- Preserved the ability to skip optional dependency checks when the related feature toggles are disabled.

### 4. Install missing Windows CI packages
- Updated the Windows MSYS2 package install steps in `.github/workflows/build-libs.yaml`.
- Added `bison` to the Windows CI package list so the parser-generator dependency check passes during configure.

## Primary Files Updated
- `cmake/FindSQLite3.cmake`
- `cmake/FindAllDependencies.cmake`
- `.github/workflows/build-libs.yaml`

## Result
- macOS CI now provides explicit SQLite3 hints to CMake and the custom finder is more robust against Homebrew layout differences.
- Windows CI no longer blocks configure on a missing external test package.
- Windows CI now installs Bison explicitly instead of assuming it is already available on the runner.