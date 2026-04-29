# Windows ARM64 Hosted Cross-Compile

## Summary
- Reworked the Windows ARM64 release workflows to cross-compile with Visual Studio `-A ARM64` on the hosted x64 runner.
- Replaced the unusable `CLANGARM64` hosted-runner path with x64 helper tools plus `arm64-windows` vcpkg dependencies.
- Updated dependency discovery so Windows vcpkg lookups can prefer `arm64-windows` instead of hardcoded `x64-windows` paths.

## Problem
- GitHub Actions `windows-latest` is an x64 host runner, not a native Windows ARM64 runner.
- MSYS2 `clangarm64` tools such as `/clangarm64/bin/ninja` are ARM64 executables and fail on the hosted x64 runner with `Exec format error`.
- Fetching an ARM64 CMake binary would not solve that problem, because an ARM64 CMake executable also cannot run on the x64 hosted runner.
- A working Windows ARM64 build on the hosted runner therefore has to be a cross-compile, not a native `clangarm64` execution path.

## Files Updated
- `.github/workflows/build-products.yaml`
- `.github/workflows/build-libs.yaml`
- `cmake/FindAllDependencies.cmake`
- `cmake/FindSQLite3.cmake`
- `_scripts/configure_windows.ps1`

## Change
- Switched the hosted Windows ARM64 jobs away from MSYS2 `CLANGARM64` execution and onto Visual Studio cross-compilation with `-A ARM64`.
- Installed `arm64-windows` dependencies through vcpkg for Eigen, SQLite, and GSL.
- Kept x64 MSYS2 helper tools only for shell utilities such as `make` and `bison` that must run on the x64 host.
- Updated Windows dependency discovery to prefer the active vcpkg triplet, including `arm64-windows`.
- Updated the shared Windows configure script to honor `VS_GENERATOR_PLATFORM` and `VCPKG_TARGET_TRIPLET`.
- Skipped Windows ARM64 test execution in `build-libs`, because the hosted x64 runner cannot run the cross-compiled ARM64 test binaries.

## Result
- Windows ARM64 builds can now target ARM64 from the x64 hosted runner using the MSVC cross toolchain.
- The workflow no longer attempts to execute ARM64 MSYS2 binaries on an x64 runner.
- Dependency resolution can line up with `arm64-windows` vcpkg packages during Windows ARM64 configure.