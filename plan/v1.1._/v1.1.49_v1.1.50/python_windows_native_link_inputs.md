# Python Windows Native Link Inputs

## Summary
- Fixed the Windows Python purchase-generation workflow so the extension no longer depends on mismatched downloaded native artifacts.
- Added an MSVC-specific native build tree for the Python packaging flow.
- Pointed Python packaging at the MSVC build output through `COOLBOX_LIB_DIR`.

## Problem
- The Windows Python release job builds the Python extension with MSVC.
- It previously relied on downloaded native artifacts that did not guarantee MSVC-compatible `charts` and `wave_generator_utils` import libraries.
- That caused linker failures such as missing `charts.lib` during packaging.

## Files Updated
- `.github/workflows/generate-purchase-python.yaml`

## Change
- Configured a dedicated `build-python-msvc` build tree for the Windows Python workflow.
- Explicitly built `charts` and `wave_generator_utils` with MSVC before running Python packaging.
- Updated the Windows packaging step to use `COOLBOX_LIB_DIR=${GITHUB_WORKSPACE}/build-python-msvc`.

## Result
- Windows Python packaging now resolves native link inputs from the same toolchain that builds the extension.
- Missing or incompatible `charts.lib` and `wave_generator_utils.lib` failures are avoided.