# Windows ARM64 Hosted Cross-Compile

## Summary
- Reworked the Windows ARM64 GitHub Actions path so it cross-compiles with Visual Studio `-A ARM64` on the hosted x64 runner.
- Removed the invalid assumption that hosted `windows-latest` can execute MSYS2 `clangarm64` binaries.
- Updated Windows dependency discovery to prefer `arm64-windows` vcpkg packages during ARM64 configure.

## Problem
- The Windows ARM64 workflows originally tried to build through the MSYS2 `CLANGARM64` environment on `windows-latest`.
- GitHub-hosted Windows runners are x64 hosts, so ARM64 MSYS2 binaries such as `/clangarm64/bin/ninja` fail with `Exec format error`.
- Switching generators or downloading a different CMake build cannot fix that, because the underlying issue is attempting to execute ARM64 binaries on an x64 runner.
- A viable hosted Windows ARM64 path therefore has to be a cross-compile configuration that keeps host tools executable on x64 while targeting ARM64 outputs.

## Files Updated
- `.github/workflows/build-products.yaml`
- `.github/workflows/build-libs.yaml`
- `_scripts/configure_windows.ps1`
- `cmake/FindAllDependencies.cmake`
- `cmake/FindSQLite3.cmake`

## Change
- Changed the Windows ARM64 jobs away from native MSYS2 `CLANGARM64` execution.
- Kept x64-host-executable helper tools for Windows CI shell steps.
- Switched the Windows ARM64 configure path to `Visual Studio 17 2022` with `-A ARM64`.
- Installed `arm64-windows` vcpkg dependencies for Eigen, SQLite, and GSL before ARM64 configure.
- Updated the shared Windows configure script to honor `VS_GENERATOR_PLATFORM` and `VCPKG_TARGET_TRIPLET`.
- Updated Windows dependency discovery helpers so they can resolve `arm64-windows` package roots instead of assuming `x64-windows`.
- Skipped Windows ARM64 test execution in the hosted library workflow because the x64 runner cannot execute the produced ARM64 binaries.

## Result
- Windows ARM64 builds now use a hosted-runner-compatible cross-compilation path.
- The workflows no longer try to execute ARM64 MSYS2 tools on an x64 GitHub Actions host.
- CMake dependency discovery can align with ARM64-targeted vcpkg packages during Windows ARM64 configuration.