# Patch Description: Use MinGW for Windows ARM64 Builds

## Summary
The `generate-purchase-rust.yaml` workflow now uses the MinGW Makefiles generator for the `windows-arm64` platform instead of Visual Studio. This allows ARM64 builds to proceed even when Visual Studio is not available on the runner.

## Details
- **Problem:**
  - The workflow previously attempted to use Visual Studio for all Windows builds, but Visual Studio is not available on many ARM64 runners, causing build failures.
- **Solution:**
  - For `windows-arm64`, the workflow now uses `-G "MinGW Makefiles"` with CMake to build native C bindings.
  - All other Windows builds (e.g., x86_64) continue to use the detected Visual Studio generator.
- **Effect:**
  - ARM64 builds will succeed on runners with MinGW installed, even if Visual Studio is missing.
  - No impact on other platforms or architectures.

## Motivation
This change ensures that Windows ARM64 builds are robust and do not require Visual Studio, which is often unavailable on CI runners for this architecture.

## Reference
- File: `.github/workflows/generate-purchase-rust.yaml`
- Change applied: v1.1.62 → v1.1.63
