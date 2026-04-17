# Rust C Bindings macOS DYLD Loader

## Related Notes
- Follow-on change to `plan/v1.1.50_v1.1.51/rust_c_bindings_absolute_link_paths.md` for Rust native C bindings link-path setup.
- Follow-on change to `plan/v1.1.56_v1.1.57/rust_c_bindings_runtime_verification.md` for runtime artifact existence checks before Rust tests run.

## Summary
- Recorded the macOS Rust release failure where `cargo test` aborts because `dyld` cannot load `@rpath/libcoolbox_c_bindings.dylib`.
- Confirmed the failure happens after compilation succeeds, so this is a runtime loader-path issue rather than a Rust compile or C bindings build failure.
- Kept the plan note in `v1.1.56_v1.1.57` so the subsequent Rust packaging fixes remain grouped in the same release-plan bucket.

## Problem
- The macOS Rust purchase job completes the Rust release build and then fails when running tests.
- `dyld` reports that `libcoolbox_c_bindings.dylib` cannot be loaded from any of the probed `@rpath` locations.
- The error occurs even though the Rust crate itself compiled successfully, which indicates the integration between the Rust test binary and the native C bindings runtime is not fully satisfied at execution time.
- This is distinct from the earlier Linux loader failure only in platform loader mechanics; both failures occur at runtime after a successful native link.

## Files Involved
- `.github/workflows/generate-purchase-rust.yaml`
- `_local_build_pipeline/scripts/jobs/generate-purchase-rust.sh`
- `_libraries/rust_bindings/build.rs`

## Change Context
- The Rust purchase flow already exports native C bindings build directories into the runtime loader environment and now verifies that the expected runtime artifact exists before tests run.
- The macOS `dyld` failure shows there is still a macOS-specific runtime lookup gap to resolve even after those broader runtime preflight changes.
- The issue should be addressed within the existing Rust purchase flow rather than by introducing a separate macOS-only packaging path.

## Result
- The macOS Rust loader failure is now explicitly tracked in the same versioned plan folder as the other current Rust purchase packaging changes.
- Subsequent macOS-specific Rust loader fixes can be documented without splitting the work across later version folders.