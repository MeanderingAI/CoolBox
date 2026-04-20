# Plan v1.1.68 → v1.1.69: Remove `exists()` Expressions from GitHub Actions Workflows

## Context
The previous workflow version (v1.1.68) attempted to use `exists()` and `!exists()` in `if:` expressions to conditionally run steps based on file system state. GitHub Actions does not support these functions in workflow expressions, resulting in YAML parsing errors and failed workflow runs.

## Change Summary
- **Removed all `exists()` and `!exists()` expressions from workflow `if:` conditions.**
- **Replaced with runtime shell checks:**
  - For ARM64 MSYS2 setup, a shell step now checks for the existence of `D:\a\_temp\msys64` and skips installation if already present.
  - The actual MSYS2 setup step is always present, but will not run if the shell check exits early.
- **No workflow logic now relies on unsupported expression functions.**

## Motivation
- Ensure full compatibility with GitHub Actions expression syntax.
- Prevent workflow YAML parsing errors and CI/CD pipeline failures.
- Make workflow logic explicit and robust for all contributors and CI environments.

## Files Changed
- `.github/workflows/build-libs.yaml`: All `exists()`/`!exists()` expressions removed from `if:` conditions. ARM64 MSYS2 setup now uses a shell check.

## Version
- From: v1.1.68
- To:   v1.1.69

---
**Date:** 2026-04-19
**Author:** GitHub Copilot
