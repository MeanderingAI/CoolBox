# Validation Results (v1.1.26 -> v1.1.27)

## Build Validation
- `make -f .\Makefile.win build_libraries` succeeds after the fixes in this transition.
- Repo-controlled packaging and docs warnings were removed.
- Remaining non-fatal external noise was reduced to Eigen configure probe output only.

## Test Validation
- Added a stable Windows test runner script:
  - `_scripts/run_release_tests.ps1`
- Added `test_logged` target to `Makefile.win`.

### Final recorded result
- Release CTest run passed completely.
- Final result:

```text
100% tests passed, 0 tests failed out of 43
```

### Logged artifacts
- Test log written to:
  - `ctest-release.log`

## Commands validated
- `make -f .\Makefile.win build_libraries`
- `make -f .\Makefile.win test_logged`
- `powershell -NoProfile -ExecutionPolicy Bypass -File .\_scripts\run_release_tests.ps1`

## Notes
- The build and test flow is now reproducible on Windows with logged output.
- Versioned handoff notes for this transition are stored in this folder.
