# Patch: Clean up stale MSYS2 install before setup (v1.1.65 → v1.1.66)

## Summary
A new workflow step was added to remove D:\a\_temp\msys64 before running msys2/setup-msys2@v2. This prevents install errors when the directory exists from a previous run.

## Changes
- Adds a "Clean up stale MSYS2 install" step before the MSYS2 setup step in build-libs.yaml.
- Uses PowerShell to remove the directory if it exists.

## Motivation
- Fixes: "Trying to install MSYS2 to D:\a\_temp\msys64 but that already exists, cannot continue."
- Ensures clean, repeatable MSYS2 setup on Windows runners.

## Patch Location
- .github/workflows/build-libs.yaml

---

**Author:** GitHub Copilot
**Date:** 2026-04-19
