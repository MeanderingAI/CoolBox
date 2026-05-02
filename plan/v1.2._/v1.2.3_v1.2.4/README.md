Move microsoft install scripts into a install_scripts sub folder in _scripts.

fix issue with vswhere.exe

## CMake Configuration Fixes

### CMake Version Updates
- Updated CMake minimum required version from 4.3.1 to 4.2.3 across all CMakeLists.txt files:
  - `CMakeLists.txt` (main)
  - `_libraries/groups/Generics/CMakeLists.txt`
  - `_libraries/groups/Generics/tyst_framework/CMakeLists.txt`
  - `_libraries/groups/sim_group/CMakeLists.txt`
  - `_libraries/groups/trekker/CMakeLists.txt`

### Visual Studio 2026 Detection
- Fixed `_scripts/configure_windows.ps1` to properly detect Visual Studio 2026
- Changed from `Get-Command vswhere.exe` to using full path: `${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe`
- Ensures Visual Studio 18 2026 generator is correctly selected

### pybind11 Modernization
- Upgraded pybind11 from v2.11.1 to v2.13.6 in `cmake/ExternalDependencies.cmake`
- Enabled `PYBIND11_FINDPYTHON` to use modern FindPython instead of deprecated FindPythonInterp
- Added conditional check in `_libraries/bindings/python_bindings/CMakeLists.txt` to avoid find_package conflict when pybind11 is already available via FetchContent

### Emscripten Bindings Fixes
- Fixed `_libraries/bindings/emscripten_bindings/CMakeLists.txt` to only build targets when using Emscripten compiler
- Wrapped all `add_executable` calls for JS bindings with `if(EMSCRIPTEN)` guards for:
  - `advanced_logging_js`
  - `circuitry_js`
  - `data_structures_js`
  - `gabor_patches_js`
- Updated `add_emscripten_module` macro to check `if(EMSCRIPTEN AND EXISTS ...)` before creating targets
- Fixed indentation and endif() nesting issues

### Dependency Fixes
- Made `password_hash_utils` dependency conditional in `_libraries/groups/sim_group/SECURITY/auth/CMakeLists.txt`
- Added `if(TARGET password_hash_utils)` guard to prevent errors when target doesn't exist

### tyst_framework Export Fixes
- Fixed INTERFACE library export in `_libraries/groups/Generics/tyst_framework/CMakeLists.txt`
- Used generator expressions for include directories:
  - `$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/headers>`
  - `$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>`
- Added `tyst_framework` target to install exports alongside `tyst_framework_main`

### CMake Output Cleanup
- Removed informational STATUS message in `_libraries/groups/trekker/MISC/CMakeLists.txt` about legacy umbrella CMakeLists
- Converted to comment: "Legacy umbrella CMakeLists disabled; individual packages provide their own targets"
- Removed WARNING message in `_libraries/bindings/emscripten_bindings/CMakeLists.txt` about not building with Emscripten
- Converted to comment since this is expected behavior when not using Emscripten compiler
- Reduces CMake configuration output noise

### Result
✅ CMake configuration now completes successfully on Windows with Visual Studio 2026 and CMake 4.2.3