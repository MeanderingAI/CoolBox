# Windows ARM64 Bison PATH And Detection

## Related Notes
- Follow-on change to `plan/v1.1.52_v1.1.53/windows_arm64_vcpkg_bootstrap.md` for the ARM64 hosted-runner bootstrap path.
- Follow-on change to `plan/v1.1.53_v1.1.54/windows_arm64_cmake_toolchain_argument_passing.md` for the ARM64 PowerShell-driven CMake configure path.

## Summary
- Fixed Windows ARM64 hosted-runner configuration so host-side parser tools from MSYS2 are visible to the PowerShell CMake configure step.
- Removed the assumption that exposing only `mingw64/bin` is sufficient for ARM64 cross-compilation prerequisites.
- Hardened Windows Bison detection in CMake with common MSYS2 and Chocolatey fallback locations.

## Problem
- The Windows ARM64 workflow cross-compiles with MSVC, but still needs host-executable build tools such as Bison during CMake configure.
- The workflow exposed `mingw64/bin` to `PATH`, but MSYS2 `bison.exe` typically lives under `usr/bin`.
- As a result, CMake could detect the ARM64 MSVC compiler while still reporting `Bison not found` and aborting configuration.
- Local Windows runs could hit the same issue when `bison.exe` existed in MSYS2 or Chocolatey locations that were not present in `PATH`.

## Files Updated
- `.github/workflows/build-products.yaml`
- `.github/workflows/build-libs.yaml`
- `cmake/FindAllDependencies.cmake`

## Change
- Updated the Windows workflow PATH setup so hosted jobs add both the MSYS2 `mingw64/bin` directory and the MSYS2 `usr/bin` directory before Windows ARM64 CMake configure runs.
- Added the same MSYS2 tool-path export pattern to both the library and product workflows.
- Extended Windows Bison detection in `FindAllDependencies.cmake` to check these fallback paths when `find_program` does not resolve Bison from `PATH`:
  - `C:/ProgramData/chocolatey/lib/winflexbison/tools/win_bison.exe`
  - `C:/msys64/usr/bin/bison.exe`
  - `C:/tools/msys64/usr/bin/bison.exe`

## Result
- Windows ARM64 hosted builds can now find Bison from the host MSYS2 installation during PowerShell-based CMake configure.
- CMake dependency detection is more resilient on Windows when Bison is installed outside the active shell `PATH`.
- The ARM64 cross-compile workflow no longer fails at configure time solely because MSYS2 `usr/bin` was omitted from `PATH`.