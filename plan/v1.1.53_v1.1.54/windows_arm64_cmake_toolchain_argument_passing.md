# Windows ARM64 CMake Toolchain Argument Passing

## Related Note
- Follow-on change to `plan/v1.1.52_v1.1.53/windows_arm64_vcpkg_bootstrap.md`, which documented the earlier Windows ARM64 `vcpkg` discovery and bootstrap hardening.

## Summary
- Fixed the Windows ARM64 hosted-runner configure steps so the resolved `vcpkg.cmake` path is passed to CMake reliably under PowerShell.
- Removed the fragile inline native-command invocation that allowed `-DCMAKE_TOOLCHAIN_FILE` to reach CMake as a literal variable token.
- Standardized both ARM64 workflow configure steps on PowerShell argument-array invocation.

## Problem
- The Windows ARM64 workflows compute the vcpkg toolchain path with PowerShell before invoking CMake.
- The previous `cmake` command embedded `-DCMAKE_TOOLCHAIN_FILE=$vcpkgToolchain` directly in a native executable invocation.
- In the failing runner log, CMake reported that it was asked to use the literal string `"$vcpkgToolchain"` instead of the resolved filesystem path.
- That caused CMake to stop before configuration because the toolchain file argument never expanded to the actual `vcpkg.cmake` location.

## Files Updated
- `.github/workflows/build-products.yaml`
- `.github/workflows/build-libs.yaml`

## Change
- Kept the existing PowerShell path resolution and `Test-Path` validation for `scripts\buildsystems\vcpkg.cmake`.
- Replaced the inline ARM64 `cmake` invocation with a PowerShell `$cmakeArgs` array in both workflows.
- Passed `-DCMAKE_TOOLCHAIN_FILE=$($vcpkgToolchain)` as a single constructed array element so the resolved path is preserved exactly when invoking `cmake`.
- Invoked CMake as `& cmake @cmakeArgs` to avoid PowerShell/native-command quoting ambiguity.

## Result
- Windows ARM64 hosted builds now pass the resolved `vcpkg.cmake` path to CMake deterministically.
- The ARM64 configure step no longer fails because CMake receives `"$vcpkgToolchain"` as a literal string.
- Both Windows ARM64 workflows use the same safer argument-passing pattern for cross-compilation configure.