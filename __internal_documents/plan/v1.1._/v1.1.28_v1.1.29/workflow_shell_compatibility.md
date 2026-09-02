# Workflow Shell Compatibility (v1.1.28 -> v1.1.29)

## Summary
This document records the shell-compatibility fix applied to the CI workflow during the v1.1.28 to v1.1.29 transition.

## Issue Addressed

### Build all tests failed on macOS runners
- The `Build all tests` step in `.github/workflows/build-libs.yaml` used `mapfile` to collect generated `*_tests` targets.
- GitHub's macOS runner shell does not reliably provide a Bash version with `mapfile` support.
- As a result, the workflow failed before any test targets were built, with `mapfile: command not found` and exit code `127`.

## Change Implemented
- Replaced the `mapfile`-based collection logic with a portable string pipeline plus `while IFS= read -r` loop.
- Kept the same target-discovery source by parsing `cmake --build . --target help` output.
- Preserved the existing behavior when no configured `*_tests` targets are found.

## Primary File Updated
- `.github/workflows/build-libs.yaml`

## Result
- The `Build all tests` step no longer depends on Bash-specific builtins that may be unavailable on macOS runners.
- The workflow remains compatible with Linux, macOS, and Windows runner shells used by the current matrix.
- The previous `mapfile` failure mode should no longer block CI test-target builds.