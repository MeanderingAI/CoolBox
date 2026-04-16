# Windows ARM64 Ninja Generator

## Summary
- Fixed the Windows ARM64 release workflows so the `CLANGARM64` toolchain no longer configures CMake with `MinGW Makefiles`.
- Replaced the ARM64 make-tool dependency with Ninja.
- Updated the Windows ARM64 library and product workflows to verify `ninja` before configure.

## Problem
- The Windows ARM64 jobs already used the `CLANGARM64` MSYS2 environment and `/clangarm64` prefix path.
- They still configured CMake with `-G 'MinGW Makefiles'`, which expects a working `mingw32-make.exe` path.
- On GitHub Actions, the `clangarm64` environment failed CMake's first compiler probe with an `unknown error` while invoking the generated make step.

## Files Updated
- `.github/workflows/build-products.yaml`
- `.github/workflows/build-libs.yaml`

## Change
- Replaced `mingw-w64-clang-aarch64-make` with `mingw-w64-clang-aarch64-ninja` in the Windows ARM64 package installation steps.
- Added `ninja` verification in the Windows ARM64 toolchain checks.
- Switched only the Windows ARM64 configure steps from `MinGW Makefiles` to `Ninja`.
- Kept Windows x86_64 on `MinGW Makefiles` so the change stays scoped to the failing ARM64 toolchain.

## Result
- Windows ARM64 configure no longer depends on the mismatched `mingw32-make.exe` path.
- CMake compiler detection can proceed with a generator that matches the `clangarm64` environment.