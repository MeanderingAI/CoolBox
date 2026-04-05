# Windows Makefile Configure Quoting (v1.1.34 -> v1.1.35)

## Summary
This document records the Windows build fix applied during the v1.1.34 to v1.1.35 transition for `Makefile.win` configure execution inside GitHub Actions.

## Issue Addressed

### Inline PowerShell in `Makefile.win` was still being corrupted before execution
- The Windows build failed before CMake configuration completed.
- The observed errors included PowerShell parser failures around the `&` invocation operator and an unexpected closing brace.
- The failing path came from PowerShell commands embedded directly in `Makefile.win`, where `make` and the outer shell could still interfere with quoting before PowerShell executed.

## Root Cause
- `Makefile.win` still contained top-level `$(shell powershell ...)` expressions used to probe Visual Studio and vcpkg state.
- Those expressions were evaluated while `make` parsed the Makefile, not only when the `configure` target ran.
- In the GitHub Actions Windows path, this created a fragile Bash/MSYS-to-PowerShell quoting boundary.
- The embedded `& vswhere.exe` invocation and surrounding script text could be mangled before PowerShell parsed it, producing the parser errors seen in CI.

## Changes Implemented

### 1. Remove fragile top-level PowerShell probes from `Makefile.win`
- Deleted the top-level `VS_GENERATOR := $(shell powershell ...)` probe.
- Deleted the top-level `VCPKG_TOOLCHAIN := $(shell powershell ...)` probe.
- These values were not required as global Makefile state because the configure step already computed them dynamically.

### 2. Move configure logic into a dedicated PowerShell script
- Added `_scripts/configure_windows.ps1`.
- Moved generator detection, vcpkg toolchain discovery, and CMake argument construction into that script.
- The script preserves normal PowerShell syntax without Bash/MSYS re-quoting the contents of a multiline `-Command` string.

### 3. Simplify the `configure` target
- Updated `Makefile.win` so `configure` now runs:
  `powershell -NoProfile -NonInteractive -ExecutionPolicy Bypass -File .\_scripts\configure_windows.ps1`
- This reduces the Makefile target to a stable script invocation rather than inline shell code.

## Primary Files Updated
- `Makefile.win`
- `_scripts/configure_windows.ps1`

## Result
- The Windows configure path no longer depends on fragile inline PowerShell quoting inside the Makefile.
- Visual Studio detection through `vswhere.exe` now runs inside native PowerShell script execution.
- The CI failure mode caused by `&` parsing and broken multiline quoting is removed at the source.