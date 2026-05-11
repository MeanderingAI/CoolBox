# Rust C Bindings Link Search

## Summary
- Fixed the Rust purchase-generation flow so Cargo links against the C bindings from absolute crate-derived paths.
- Added native C bindings build steps before Rust build and test execution in both GitHub Actions and the local release pipeline.
- Added explicit CI environment variables for the Rust crate so Windows MSVC and macOS linkers receive absolute C bindings search directories.
- Updated Rust binding documentation so the native prerequisite is explicit.

## Problem
- The Rust crate build script emitted `cargo:rustc-link-search` entries as `../c_bindings/build` and `../c_bindings/build/Release`.
- In the GitHub Actions macOS release job, Cargo invoked the linker from a workspace context where those relative paths did not exist.
- The Rust workflow and local release script also ran Cargo without first building `_libraries/c_bindings`, so the required native library could still be absent even with correct search paths.

## Files Updated
- `_libraries/rust_bindings/build.rs`
- `.github/workflows/generate-purchase-rust.yaml`
- `_local_build_pipeline/scripts/jobs/generate-purchase-rust.sh`
- `_libraries/rust_bindings/README.md`
- `docs/core-lib/languages/Rust.md`

## Change
- Updated `build.rs` to canonicalize `CARGO_MANIFEST_DIR`, derive absolute C bindings library search paths, and honor CI-provided absolute path overrides.
- Added native `_libraries/c_bindings` configure and build steps before `cargo build` and `cargo test` in the Rust workflow.
- Exported `COOLBOX_C_BINDINGS_BUILD_DIR` and `COOLBOX_C_BINDINGS_BUILD_CONFIG_DIR` in CI and the local release script so Cargo passes absolute Windows and macOS linker search paths.
- Added the same native prerequisite build to the local `generate-purchase-rust.sh` job.
- Updated Rust build documentation to include the C bindings prerequisite.

## Result
- macOS and Windows Rust release builds no longer hand the linker missing relative search paths for `coolbox_c_bindings`.
- Rust packaging and test flows now build the native C bindings before Cargo tries to link against them.