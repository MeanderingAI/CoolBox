# Windows ARM64 vcpkg Bootstrap Hardening

## Summary
- Hardened the Windows ARM64 hosted-runner workflow so `vcpkg.exe` is discovered from common install locations before falling back to a local bootstrap.
- Removed the fragile assumption that `Get-Command vcpkg.exe` always succeeds on the hosted Windows runner.
- Tightened the local Windows `vcpkg` installer so an existing clone is only treated as valid when `vcpkg.exe` actually exists.

## Problem
- The Windows ARM64 cross-compile path depends on `vcpkg` to install `arm64-windows` dependencies before CMake configure.
- The earlier workflow logic assumed `Get-Command vcpkg.exe` would always return a valid command object and then immediately derived the install root from it.
- When the executable was not in `PATH`, PowerShell failed while trying to resolve the parent directory from a null value.
- The fallback installer also treated an existing `%USERPROFILE%\vcpkg` directory as success even if `vcpkg.exe` had never been bootstrapped.

## Files Updated
- `.github/workflows/build-products.yaml`
- `.github/workflows/build-libs.yaml`
- `_scripts/install_vcpkg.ps1`

## Change
- Added a `Find-VcpkgExe` helper in the Windows ARM64 workflow steps to search:
  - `Get-Command vcpkg.exe`
  - `VCPKG_ROOT`
  - `C:\vcpkg\vcpkg.exe`
  - `%USERPROFILE%\vcpkg\vcpkg.exe`
  - `%LOCALAPPDATA%\vcpkg\vcpkg.exe`
- If none of those locations contain `vcpkg.exe`, the workflow now runs `_scripts/install_vcpkg.ps1` and retries discovery.
- If discovery still fails after installation, the workflow now stops with an explicit error instead of dereferencing a null path.
- Updated the Windows ARM64 CMake configure steps to invoke `cmake` through a PowerShell argument array so `-DCMAKE_TOOLCHAIN_FILE` receives the resolved path instead of a literal variable token.
- Updated `_scripts/install_vcpkg.ps1` so it:
  - exits early only when `vcpkg.exe` already exists
  - reuses an existing clone when the directory exists but the executable does not
  - bootstraps the clone and verifies that `vcpkg.exe` was actually created

## Result
- Windows ARM64 hosted builds no longer fail at the first `vcpkg` lookup when the executable is not already in `PATH`.
- The fallback bootstrap path is resilient to partially initialized local `vcpkg` directories.
- The ARM64 configure step now passes the `vcpkg.cmake` path to CMake deterministically under PowerShell.
- ARM64 dependency installation now fails with a clear actionable error only if `vcpkg` is genuinely unavailable after bootstrap.