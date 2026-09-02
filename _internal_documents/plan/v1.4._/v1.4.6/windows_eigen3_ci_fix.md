# v1.4.6 Windows Eigen3 CI Fix

## Issue

Windows GitHub Actions builds failed during configure when `BUILD_PYTHON_BINDINGS=ON` with:

- `Could not find a package configuration file provided by "Eigen3"`
- Reported at `_deliverables/libraries/bindings/python_bindings/CMakeLists.txt`.

The failure happened because `python_bindings` called `find_package(Eigen3 REQUIRED)` directly, which requires a system/package-manager Eigen config (`Eigen3Config.cmake` or `FindEigen3.cmake`) even when the repository had already provided `Eigen3::Eigen` via `cmake/ExternalDependencies.cmake` FetchContent.

## Fix

### 1) Python bindings CMake fallback logic

Updated `_deliverables/libraries/bindings/python_bindings/CMakeLists.txt` to:

- Prefer an already-defined `Eigen3::Eigen` target.
- Attempt `find_package(Eigen3 QUIET CONFIG)` only if the target is missing.
- Use `find_package(Eigen3 REQUIRED)` as final fallback.
- Stop depending on raw `${EIGEN3_INCLUDE_DIR}` in `include_directories` (target linkage already carries include paths).

### 2) Root CMake fallback logic

Updated `CMakeLists.txt` to:

- Avoid unconditional `find_package(Eigen3 REQUIRED)` when `Eigen3::Eigen` already exists.
- Guard global include usage behind `if(DEFINED EIGEN3_INCLUDE_DIR)`.

## Why this is safe

- Preserves existing behavior for environments where Eigen is installed system-wide.
- Unblocks CI/toolchains where Eigen is vendored or provided by FetchContent target only.
- Keeps target-based linking (`Eigen3::Eigen`) as the primary integration path.

## Validation

Recommended CI/local checks:

1. Configure/build on Windows with Python bindings enabled and no preinstalled Eigen package config.
2. Confirm configure no longer fails in `python_bindings`.
3. Confirm `ml_core` and optional `py_circuitry` still link against `Eigen3::Eigen`.
