# Patch Description: Prefer MinGW for Windows ARM64 in build-libs.yaml

## Summary
The `build-libs.yaml` workflow now tries to use the MinGW Makefiles generator for the `windows-arm64` platform first. If MinGW is not available or fails, it falls back to the Visual Studio generator. This matches the logic used for Rust and ensures ARM64 builds are more robust.

## Details
- **Problem:**
  - The workflow previously only attempted to use Visual Studio for Windows ARM64, which is often not available on CI runners, causing build failures.
- **Solution:**
  - For `windows-arm64`, the workflow now tries `-G 'MinGW Makefiles'` first.
  - If MinGW fails, it falls back to the detected Visual Studio generator.
  - All other platforms/architectures are unchanged.
- **Effect:**
  - ARM64 builds will succeed on runners with MinGW installed, even if Visual Studio is missing.
  - If MinGW is not available, the workflow will still attempt to use Visual Studio as a fallback.

## Motivation
This change ensures that Windows ARM64 builds are robust and do not require Visual Studio, which is often unavailable on CI runners for this architecture.

## Reference
- File: `.github/workflows/build-libs.yaml`
- Change applied: v1.1.62 → v1.1.63
