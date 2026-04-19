# Patch Description: Visual Studio Generator Auto-Detection in All Workflows

## Summary
All workflows that previously hardcoded the Visual Studio 2022 generator now use auto-detection logic (via PowerShell and `cmake --help`) to select the latest available Visual Studio generator. This ensures compatibility with VS 2026 and future versions.

## Details
- **Problem:**
  - Several workflows (e.g., `build-libs.yaml`, `build-products.yaml`) were hardcoded to use `-G 'Visual Studio 17 2022'`, causing failures if only newer versions are installed.
- **Solution:**
  - Updated the PowerShell build steps to dynamically detect and use the latest Visual Studio generator string from `cmake --help`.
  - This logic is now consistent with the `_scripts/detect_vs_generator.cmake` script and the Rust workflow.
- **Effect:**
  - All Windows builds in these workflows will use the newest available Visual Studio version, with no manual edits required for future upgrades.

## Motivation
This change ensures all CI/CD and local builds are robust to Visual Studio upgrades and always use the best available generator.

## Reference
- Files:
  - `.github/workflows/build-libs.yaml`
  - `.github/workflows/build-products.yaml`
- Change applied: v1.1.61 → v1.1.62
