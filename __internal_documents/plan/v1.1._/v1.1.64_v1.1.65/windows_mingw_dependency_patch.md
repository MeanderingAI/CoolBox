# Patch: Robust Windows MinGW Dependency Installation (v1.1.64 → v1.1.65)

## Summary
This patch fixes a regression in the x86_64 MinGW build (missing GSL/Bison) and ensures the ARM64 MinGW build uses vcpkg for GSL and installs Bison. It guarantees robust cross-platform dependency installation for both Windows architectures.

## Changes
- **Windows x86_64 (MinGW):**
  - Installs `mingw-w64-x86_64-gsl` and `mingw-w64-x86_64-bison` via pacman before CMake configuration.
- **Windows ARM64 (MinGW):**
  - Installs Bison using `_scripts/install_bison.ps1`.
  - Ensures vcpkg is used for GSL, Eigen3, SQLite3, and OpenSSL dependencies.
  - vcpkg toolchain is always passed to CMake for ARM64 MinGW builds.

## Motivation
- Fixes missing GSL/Bison regression for x86_64 MinGW builds.
- Ensures ARM64 MinGW builds are robust and consistent with x86_64.
- Keeps all dependency installation explicit and documented for CI/CD reliability.

## Patch Location
- `.github/workflows/build-libs.yaml`

## Related Issues
- Regression: x86_64 MinGW build missing GSL/Bison after ARM64 dependency changes.
- ARM64: Ensure vcpkg and Bison are always installed for MinGW builds.

## Validation
- CI should now pass for both x86_64 and ARM64 MinGW builds with all dependencies present.

---

**Author:** GitHub Copilot
**Date:** 2026-04-19
