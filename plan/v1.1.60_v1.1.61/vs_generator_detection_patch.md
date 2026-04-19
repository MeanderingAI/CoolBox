# Patch Description: Visual Studio Generator Auto-Detection for Windows Builds

## Summary
This patch updates the Windows build section of `.github/workflows/generate-purchase-rust.yaml` to automatically detect and use the latest available Visual Studio generator, instead of hardcoding Visual Studio 2022.

## Details
- **Problem:**
  - The workflow was hardcoded to use `-G "Visual Studio 17 2022"`, which fails if only newer versions (e.g., 2026) are installed.
- **Solution:**
  - Added a step to detect all installed Visual Studio generators using `cmake --help` and PowerShell.
  - The build step now uses the latest detected generator for maximum compatibility.
- **Effect:**
  - The workflow will work with any installed Visual Studio version (2015, 2017, 2019, 2022, 2026, etc.), always picking the newest available.
  - No manual workflow edits are needed when upgrading Visual Studio.

## Motivation
This change ensures the pipeline is robust to future Visual Studio releases and works out-of-the-box on developer machines and CI runners with different VS versions.

## Reference
- File: `.github/workflows/generate-purchase-rust.yaml`
- Change applied: v1.1.60 → v1.1.61
