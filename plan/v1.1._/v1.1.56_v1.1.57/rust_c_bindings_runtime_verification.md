# Rust C Bindings Runtime Verification

## Related Note
- Follow-on change to `plan/v1.1.50_v1.1.51/rust_c_bindings_absolute_link_paths.md` for Rust native C bindings link-path setup.

## Summary
- Added an explicit verification step that checks for the native `coolbox_c_bindings` runtime artifact before Rust binding tests run.
- Converted a late dynamic-loader failure into an earlier, more actionable artifact-production failure.
- Kept the same runtime-path exports for Linux and macOS while improving failure diagnostics when the shared library is absent.

## Problem
- The Rust purchase flow builds `_libraries/c_bindings`, then runs `cargo test` for `_libraries/rust_bindings`.
- Even after exporting loader search paths, the job can still fail if the expected shared library was never produced in the build directories.
- Without an explicit verification step, the failure surfaces later as a runtime loader error inside `cargo test`, which is less precise than an immediate missing-artifact check.

## Files Updated
- `.github/workflows/generate-purchase-rust.yaml`
- `_local_build_pipeline/scripts/jobs/generate-purchase-rust.sh`

## Change
- Added a workflow step after building `_libraries/c_bindings` that checks expected runtime artifact locations for the current runner OS before running Rust build and test commands.
- On Linux, the workflow checks for `libcoolbox_c_bindings.so` in both the base build directory and the config-specific build directory.
- On macOS, the workflow checks for `libcoolbox_c_bindings.dylib` in those locations.
- On Windows, the workflow checks both `coolbox_c_bindings.dll` and `libcoolbox_c_bindings.dll` naming variants in the base and config-specific output directories.
- If no runtime artifact is found, the workflow now exits early and prints the candidate paths plus directory listings for the checked output locations.
- Added the same early-exit runtime verification pattern to the local Rust purchase script.

## Result
- Rust purchase jobs now fail faster and with clearer diagnostics when the native C bindings runtime library was not actually produced.
- Loader-path issues and missing-runtime-artifact issues are now separated more cleanly in CI logs.
- Local and GitHub workflow behavior are aligned for the Rust native runtime preflight check.