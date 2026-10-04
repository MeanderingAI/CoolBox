# Windows CI Toolchain Path Hardening (v1.5.0)

## Issue

Windows ARM64 configure steps failed with a null-path error:

- `Join-Path: Cannot bind argument to parameter 'Path' because it is null`

## Root cause

Workflow steps assumed `VCPKG_ROOT` is always present before constructing `scripts/buildsystems/vcpkg.cmake`.

## Fix

Updated Windows ARM64 configure steps in:

- `.github/workflows/build-products.yaml`
- `.github/workflows/fragments/deps-windows/action.yaml`

Added a shared inline resolution pattern to locate `vcpkg.cmake` from:

- `VCPKG_ROOT` (if set)
- `vcpkg.exe` on PATH
- `%USERPROFILE%\vcpkg`
- `%LOCALAPPDATA%\vcpkg`
- `C:\vcpkg`

## Impact

Prevents null `Join-Path` failures and makes ARM64 Windows configure steps resilient across runner environments.
