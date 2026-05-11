# Windows ARM64 CLANGARM64 Workflows

## Summary
- Fixed the Windows ARM64 library and product workflows to use the supported MSYS2 `CLANGARM64` environment.
- Replaced invalid `mingw-w64-aarch64-*` package references with the supported `mingw-w64-clang-aarch64-*` package family.
- Updated PATH and CMake prefix handling so ARM64 jobs resolve libraries from `/clangarm64`.
- Switched Windows ARM64 CMake generation from `MinGW Makefiles` to `Ninja` so the `clangarm64` toolchain no longer depends on `mingw32-make.exe`.

## Problem
- The Windows ARM64 GitHub Actions jobs were configured for the wrong MSYS2 environment.
- Those jobs also attempted to install package names that do not exist in the current MSYS2 repository layout.
- After the environment fix, ARM64 configure still failed because `MinGW Makefiles` invoked `mingw32-make.exe`, which is not the right build tool path for the `clangarm64` setup on GitHub Actions.

## Files Updated
- `.github/workflows/build-libs.yaml`
- `.github/workflows/build-products.yaml`

## Change
- Switched ARM64 jobs from `MINGW64` to `CLANGARM64`.
- Replaced `mingw-w64-aarch64-*` packages with `mingw-w64-clang-aarch64-*` packages.
- Updated Windows ARM64 PATH injection to use the `clangarm64` toolchain bin directory.
- Updated ARM64 CMake prefix configuration to use `/clangarm64`.
- Replaced the ARM64 `mingw-w64-clang-aarch64-make` dependency with `mingw-w64-clang-aarch64-ninja`.
- Updated the ARM64 configure steps to use the `Ninja` generator while leaving x86_64 on `MinGW Makefiles`.

## Result
- Windows ARM64 CI now targets a supported MSYS2 toolchain and package set.
- ARM64 configure no longer depends on the broken `mingw32-make.exe` path and can run compiler detection with the `clangarm64` toolchain.