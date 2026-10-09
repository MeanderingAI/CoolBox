# CoolBox v3.2.2 Release Notes

## Windows Release Test Timeouts

Added a 300-second default timeout per test to the Windows Release CTest
runner. This gives slower Windows tests more time while ensuring a stalled test
does not block the entire suite indefinitely.

### Changes

- [`run_release_tests.ps1`](../../../../_scripts/run_release_tests.ps1) passes
  CTest's `--timeout` option, applying the default to tests without an explicit
  per-test timeout.
- The runner accepts `-TimeoutSeconds` to select a different positive timeout
  when invoked directly.
- `FileBrowserTests` has an explicit 300-second timeout to match the runner's
  default; this avoids its former 30-second limit overriding the Windows
  default.
- Windows `test`, `test_logged`, and filtered `test-*` Makefile targets all use
  the shared runner.

See [`Makefile.win`](../../../../Makefile.win) and the
[file-browser test registration](../../../../_deliverables/libraries/groups/app_assets/file_browser/CMakeLists.txt).

## Verification

- [x] Confirmed the Windows Makefile test entry points use the shared Release
  test runner.
- [x] Confirmed CTest supports the `--timeout` option.
- [x] `git diff --check` passed.
- [ ] Rerun the Windows Release CTest workflow after publishing the change;
  execution on a Windows runner was not available locally.

## Investigating Windows Graphics Segmentation Faults

The Windows log reports crashes in `ChartsTests` at BMP export,
`GraphicsFontsTests` while testing a missing font, and `GraphicsCanvasTests`
while creating/loading a BMP. These failures cluster around C++ file I/O,
suggesting a possible MinGW compiler/runtime mismatch. This is a working
hypothesis, not a confirmed crash diagnosis.

### Changes

- The Windows x64 dependency action explicitly installs and selects the MSYS2
  MINGW64 GCC compiler, and exports its directory to subsequent workflow steps.
- The Release test runner reads the configured compiler from `CMakeCache.txt`,
  verifies the matching MinGW runtime DLLs exist, and prioritizes their
  directory on `PATH` for the test process. The original `PATH` is restored
  afterward. MSVC builds are not affected.
- Release builds retain debug symbols for stack traces.
- If the Windows x64 test step fails, a bounded GDB diagnostic step reruns the
  three graphics test executables and records stack traces and loaded-library
  information under `build/Testing/graphics-debug`. These logs are included in
  the existing test-results artifact upload.

See the
[Windows dependency action](../../../../.github/workflows/fragments/deps-windows/action.yaml),
[Release test runner](../../../../_scripts/run_release_tests.ps1), and
[library workflow](../../../../.github/workflows/build-libs.yaml).

### Verification Status

- [ ] Confirm the selected runtime paths and crash behavior on Windows CI.
- [ ] Inspect debugger logs if the crashes persist.
- The local CMake Tools build could not configure, so no local reproduction
  or Windows runtime validation was possible.
- The six timeout failures are separate unresolved failures; increasing their
  timeout is not evidence that their underlying cause has been fixed.
