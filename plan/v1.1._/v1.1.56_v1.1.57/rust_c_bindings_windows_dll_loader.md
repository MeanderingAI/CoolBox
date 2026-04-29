# Rust C Bindings Windows DLL Loader

## Related Notes
- Follow-on change to `plan/v1.1.56_v1.1.57/rust_c_bindings_runtime_verification.md` for pre-test runtime artifact verification.
- Follow-on change to `plan/v1.1.50_v1.1.51/rust_c_bindings_absolute_link_paths.md` for Rust native C bindings link-path setup.

## Summary
- Fixed the Windows Rust purchase test flow so the native `coolbox_c_bindings` DLL output directories are added to `PATH` before `cargo test` runs.
- Addressed the Windows runtime loader failure that surfaced as `STATUS_DLL_NOT_FOUND` after the Rust crate compiled successfully.
- Kept the fix scoped to the test runtime environment rather than changing the Rust crate build configuration.

## Problem
- The Windows Rust purchase job compiled `_libraries/rust_bindings` successfully and then failed when executing the integration test binary.
- The failing process exited with `0xc0000135 (STATUS_DLL_NOT_FOUND)`, which indicates the Windows loader could not find one of the required DLLs at runtime.
- The native `coolbox_c_bindings` runtime artifact had already been verified to exist, so the remaining gap was that the Rust test process was launched without the relevant C bindings output directories on `PATH`.
- This is the Windows analogue of the Linux and macOS dynamic loader path issues, but it relies on `PATH` rather than `LD_LIBRARY_PATH` or `DYLD_LIBRARY_PATH`.

## Files Updated
- `.github/workflows/generate-purchase-rust.yaml`

## Change
- Updated the Rust test step in the GitHub workflow so that on Windows it prepends `COOLBOX_C_BINDINGS_BUILD_DIR` and `COOLBOX_C_BINDINGS_BUILD_CONFIG_DIR` to `PATH` before invoking `cargo test`.
- Left the existing Linux and macOS runtime loader environment exports in place for their respective shared-library lookup paths.
- Kept the runtime artifact verification step unchanged, since it already distinguishes missing-artifact failures from runtime loader-path failures.

## Result
- Windows Rust integration tests can now locate `coolbox_c_bindings` DLL outputs at runtime through the process `PATH`.
- The job should no longer fail with `STATUS_DLL_NOT_FOUND` solely because the C bindings output directories were absent from the Windows loader search path.
- Windows, Linux, and macOS Rust purchase flows now all explicitly configure the appropriate runtime library search environment before running tests.