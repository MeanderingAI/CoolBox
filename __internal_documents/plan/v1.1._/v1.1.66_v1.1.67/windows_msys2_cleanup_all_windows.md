# Patch: Ensure MSYS2 Cleanup for All Windows Builds (v1.1.67 → v1.1.68)

## Summary
Ensures the "Clean up stale MSYS2 install" step runs for all Windows jobs (x86_64 and ARM64) by keeping the condition as `runner.os == 'Windows'`. This prevents MSYS2 install errors on both architectures.

## Changes
- The cleanup step is not platform-specific and will always run for Windows jobs, cleaning up D:\a\_temp\msys64 before MSYS2 setup.

## Motivation
- Fixes: "Trying to install MSYS2 to D:\a\_temp\msys64 but that already exists, cannot continue." for both x86_64 and ARM64.
- Ensures clean, repeatable MSYS2 setup on all Windows runners.

## Patch Location
- .github/workflows/build-libs.yaml

---

**Author:** GitHub Copilot
**Date:** 2026-04-19
