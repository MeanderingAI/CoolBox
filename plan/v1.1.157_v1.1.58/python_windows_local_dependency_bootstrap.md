# Python Windows Local Dependency Bootstrap

## Summary
- Documented the Windows-specific local runner changes needed to validate Python-related build and packaging flows on developer machines.
- Updated the native Windows local pipeline bootstrap so CMake-based prerequisite builds no longer fail immediately on missing `GSL` and `Bison`.
- Brought the local Windows runner closer to the GitHub Actions Windows dependency setup by wiring in `vcpkg` discovery, package installation, and toolchain propagation.

## Problem
- Windows-local validation for Python-related workflows depends on successfully configuring and building the native C++ libraries that the Python bindings link against.
- The native Windows runner failed during CMake configure with missing dependency errors for `GSL` and `Bison`.
- After configure failed, downstream build steps could also report secondary errors like `MSB1009: Project file does not exist. Switch: charts.vcxproj` because the Visual Studio project files were never generated.
- The local native runner did not match the Windows CI workflow behavior: it did not bootstrap `vcpkg`, did not install `gsl`, and did not pass the `vcpkg` toolchain file into CMake.

## Files Updated
- `_local_build_pipeline/scripts/jobs/build-libs.windows.ps1`

## Change
- Added `vcpkg` discovery logic that checks common install locations and the current environment.
- Added a fallback bootstrap path that runs `_scripts/install_vcpkg.ps1` if `vcpkg.exe` is missing.
- Installed the Windows native dependency set required by downstream Python-related build flows: `eigen3:x64-windows`, `sqlite3:x64-windows`, `gsl:x64-windows`, and `openssl:x64-windows`.
- Added `Bison` detection for common Windows locations and a fallback install path via `_scripts/install_bison.ps1` when Chocolatey is available.
- Updated the native Windows CMake configure step to pass `-DCMAKE_TOOLCHAIN_FILE=...`, `-DVCPKG_TARGET_TRIPLET=x64-windows`, and `-A x64` so dependency resolution matches the expected hosted Windows toolchain layout.

## Result
- Local Windows runs that are used to support Python packaging and validation should no longer fail immediately on missing `GSL` and `Bison` when the bootstrap path is available.
- Secondary `charts.vcxproj` missing-project errors are avoided because the runner now has a path to complete CMake configure before attempting a build.
- The Windows local runner is now aligned more closely with the dependency provisioning already used in the GitHub Actions Windows workflows that support Python purchase/build flows.