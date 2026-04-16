# Rust C Bindings Absolute Link Paths

## Summary
- Fixed the Rust bindings build so Cargo emits absolute C bindings library search paths instead of fragile relative paths.
- Added explicit C bindings path environment variables for CI and the local release pipeline.
- Ensured Rust build and test flows build the native C bindings before Cargo links against them.

## Problem
- The Rust crate linked `coolbox_c_bindings` using library search paths derived as `../c_bindings/build` and `../c_bindings/build/Release`.
- On macOS and Windows GitHub Actions runners, Cargo invoked the platform linker from a context where those relative paths were not reliable.
- The Rust workflow and local release script also ran Cargo before guaranteeing `_libraries/c_bindings` had been built.

## Files Updated
- `_libraries/rust_bindings/build.rs`
- `.github/workflows/generate-purchase-rust.yaml`
- `_local_build_pipeline/scripts/jobs/generate-purchase-rust.sh`
- `_libraries/rust_bindings/README.md`
- `docs/core-lib/languages/Rust.md`

## Change
- Canonicalized `CARGO_MANIFEST_DIR` in `build.rs` and emitted absolute `cargo:rustc-link-search` entries.
- Added support for `COOLBOX_C_BINDINGS_BUILD_DIR` and `COOLBOX_C_BINDINGS_BUILD_CONFIG_DIR` as explicit CI overrides.
- Exported those variables in the Rust GitHub Actions workflow and local release pipeline.
- Added native `_libraries/c_bindings` configure and build steps before `cargo build` and `cargo test`.
- Updated Rust build documentation to include the C bindings prerequisite.

## Result
- macOS and Windows Rust release jobs no longer hand missing relative library paths to the platform linker.
- Rust packaging and test flows build the native C bindings before attempting to link `coolbox_c_bindings`.