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
