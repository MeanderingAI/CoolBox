# Patch Description: Visual Studio Generator Detection Script for All Workflows

## Summary
All workflows now use a dedicated PowerShell script (`_scripts/detect_vs_generator.ps1`) to robustly detect and select the latest available Visual Studio generator for Windows builds. This replaces all previous inline or error-prone generator detection logic.

## Details
- **Problem:**
  - Previous generator detection logic in workflows was fragile and failed to parse multi-part version numbers, causing build errors.
- **Solution:**
  - Created `_scripts/detect_vs_generator.ps1` to parse, sort, and select the latest Visual Studio generator from `cmake --help` output.
  - Updated all relevant workflows to call this script for generator selection.
- **Effect:**
  - All Windows builds now reliably use the newest available Visual Studio version, with robust error handling and no manual edits required for future upgrades.

## Motivation
This change ensures all CI/CD and local builds are robust to Visual Studio upgrades and always use the best available generator, with a single source of truth for generator detection.

## Reference
- Files:
  - `.github/workflows/build-libs.yaml`
  - `.github/workflows/build-products.yaml`
  - `.github/workflows/generate-purchase-rust.yaml`
  - `_scripts/detect_vs_generator.ps1`
- Change applied: v1.1.60 → v1.1.61
