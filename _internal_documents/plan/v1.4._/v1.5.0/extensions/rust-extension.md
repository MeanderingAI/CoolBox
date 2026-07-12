# Rust Extension Workflow Fixes (v1.5.0)

## Issues

1. Linux packaging step failed with:

- `cp: cannot stat 'target/package/*.crate': No such file or directory`

2. Windows native C bindings step failed with:

- `line 1: =: command not found`
- `.\_scripts\detect_vs_generator.ps1: command not found`

## Root causes

- `cargo package --manifest-path ...` produced crate output under the manifest-local `target/package` path, while the workflow copied only from repo-root `target/package`.
- The Windows generator-detection command used PowerShell syntax in a step executed under default `bash`.

## Fix

Updated `.github/workflows/generate-purchase-rust.yaml`:

- Set `shell: pwsh` for `Build native C bindings (Windows x86_64)`.
- Made crate packaging robust by searching both candidate output paths:
  - `_deliverables/libraries/bindings/rust_bindings/target/package/*.crate`
  - `target/package/*.crate`
- Added explicit failure with diagnostics if no crate is found.

## Impact

- Restores Windows x86_64 native C bindings configure/build step in Rust purchase workflow.
- Prevents Linux crate packaging failures due to path assumptions about Cargo output location.
