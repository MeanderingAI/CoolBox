# Fix Windows Python bindings build: missing GSL and Bison dependencies

## Problem
- Windows pipeline for Python bindings fails during CMake configuration:
  - `Missing dependencies detected:`
  - `-- GSL (GNU Scientific Library)`
  - `-- Bison (parser generator)`
  - MSBuild error: `Project file does not exist. Switch: charts.vcxproj`
- The build cannot proceed because required dependencies are not installed, so project files are not generated.

## Solution
1. **Install GSL using vcpkg:**
   ```
   vcpkg install gsl
   ```
   - Ensure the vcpkg toolchain file is used, or add the GSL CMake path to `CMAKE_PREFIX_PATH`.
2. **Install Bison:**
   - Run the provided PowerShell script:
     ```
     powershell -ExecutionPolicy Bypass -File _scripts/install_bison.ps1
     ```
   - Or manually install winflexbison and add it to your PATH.
3. **Re-run CMake** to generate the project files after installing dependencies.

## References
- See also: `fix-python-macos-vendor-includes.md`, `fix-python-ubuntu-vendor-includes.md`, `fix-generate-purchase-python-ubuntu-vendor-includes.md` for related cross-platform dependency fixes.

## Status
- Pending in v1.1.59_v1.1.60.
- Apply these steps to resolve the Windows build error for Python bindings.
