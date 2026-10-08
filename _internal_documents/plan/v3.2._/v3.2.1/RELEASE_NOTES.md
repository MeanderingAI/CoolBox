# CoolBox v3.2.1 Release Notes

## Windows ARM64 Python Discovery Fix

Fixed CMake configuration failures when building Windows ARM64 targets on x64
GitHub Actions runners with Visual Studio.

### Root Cause

Visual Studio's `-A ARM64` can select an ARM64 target without setting
`CMAKE_CROSSCOMPILING`. The previous Python guards required that variable, so
pybind11 still initialized Python discovery and rejected the runner's x64
interpreter with `Wrong architecture for the interpreter`.

The external dependency configuration also forcibly changed
`PYBIND11_FINDPYTHON` to `ON`, overriding the ARM64 workflow's explicit `OFF`.

### Changes

- [ExternalDependencies.cmake](../../../../cmake/ExternalDependencies.cmake)
  detects Windows ARM64 targets using the Visual Studio platform, generator
  platform, or target processor. It skips pybind11 when cross-compiling or
  when the host processor is not ARM64, even if `CMAKE_CROSSCOMPILING` is false.
- The same host/target guard in the
  [Python bindings configuration](../../../../_deliverables/libraries/bindings/python_bindings/CMakeLists.txt)
  returns before Python interpreter discovery.
- The [root configuration](../../../../CMakeLists.txt) respects the pybind11
  skip decision before performing optional Python package discovery.
- `PYBIND11_FINDPYTHON` still defaults to `ON`, but no longer forcibly
  overrides an explicitly configured value.
- Native Windows ARM64 hosts retain Python bindings support and require a
  matching ARM64 Python installation. Windows x64 and non-Windows builds
  retain their existing discovery behavior.
- The behavior is documented in the
  [Python bindings README](../../../../_deliverables/libraries/bindings/python_bindings/README.md).

## Verification

- [x] The
  [ARM64 guard regression test](../../../../_local_build_pipeline/scripts/test_arm64_python_guard.py)
  passed all seven host/target scenarios: Visual Studio ARM64 on x64,
  generator-platform ARM64 on x64, processor ARM64 on x64, explicit ARM64
  cross-compilation, native Windows ARM64, Windows x64, and macOS ARM64.
- [x] The tests exercise both guards with CMake script mode, including the
  reported case where `CMAKE_CROSSCOMPILING` is false.
- [x] `git diff --check` passed.
- [ ] Rerun the Windows ARM64 GitHub Actions build after publishing the fix.
  The actual MSVC ARM64 build was not verified on the local macOS host.

## Windows x86 File Browser Test Performance

Updated `FileBrowserTests`, which was taking an excessive amount of time in
the Windows x86 Release test run.

### Root Cause

The `RefreshesCurrentWorkspace` test constructed the file browser with the
current working directory and recursively traversed every directory and file.
CTest launches the test from the build directory in CI, where the generated
build tree and fetched dependencies can be large.

### Changes

- The file browser test now creates a small, isolated temporary directory with
  a nested folder and sample files, and validates that those entries are found.
- The missing-path test now uses a path under its own temporary directory
  rather than relying on a sentinel path in the CTest working directory.
- `FileBrowserTests` has a CTest timeout so an unexpected traversal
  cannot stall the test suite indefinitely.

See the
[file browser tests](../../../../_deliverables/libraries/groups/app_assets/file_browser/tests/test_file_browser.cpp)
and [test registration](../../../../_deliverables/libraries/groups/app_assets/file_browser/CMakeLists.txt).

## Verification

- [x] `git diff --check` passed.
- [x] No editor diagnostics were reported for the modified test or CMake file.
- [ ] Rerun the Windows x86 Release tests after publishing the fix. The test
  executable was not available in the local macOS build directory.

## Windows Release Test Timeouts

Added a 300-second default timeout per test to the Windows Release CTest runner.
This prevents a hung test from blocking the remaining suite indefinitely while
allowing slower Windows tests more time than the previous FileBrowser-specific
30-second limit.

- The default is passed to CTest with `--timeout` by
  [`run_release_tests.ps1`](../../../../_scripts/run_release_tests.ps1) and
  applies to tests without their own explicit timeout.
- The script accepts `-TimeoutSeconds` to configure a different positive
  timeout when running it directly.
- `FileBrowserTests` now has the same explicit 300-second limit.
- Both `make -f Makefile.win test` and `test_logged` use this runner; filtered
  `test-*` runs use it as well.

## Verification

- [x] Confirmed all Windows Makefile CTest entry points invoke the shared
  Release test runner.
- [x] Confirmed no other CMake tests currently set an explicit timeout.
- [x] `git diff --check` passed.
- [ ] Rerun the Windows x86 Release CTest workflow after publishing the change.
