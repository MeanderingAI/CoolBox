# Patch Description: Ensure MinGW and Make Dependencies for Windows ARM64

## Summary
The `build-libs.yaml` workflow now explicitly installs MSYS2, MinGW, and make for the `windows-arm64` platform before running CMake with MinGW Makefiles. It also cleans the CMake cache before switching generators. This ensures all required tools are present and avoids generator mismatch errors.

## Details
- **Problem:**
  - Previous builds failed with errors about missing MinGW, make, or toolchain files, and generator mismatches when switching between MinGW and Visual Studio.
- **Solution:**
  - Added steps to install MSYS2, MinGW, and make for `windows-arm64`.
  - Added a step to clean the CMake cache before switching generators.
- **Effect:**
  - ARM64 builds will succeed if MinGW is available, and generator mismatches are avoided.
  - If MinGW is not available, the workflow will still attempt to use Visual Studio as a fallback.

## Motivation
This change ensures that Windows ARM64 builds are robust, have all required dependencies, and avoid common CMake generator errors.

## Reference
- File: `.github/workflows/build-libs.yaml`
- Change applied: v1.1.63 → v1.1.64
