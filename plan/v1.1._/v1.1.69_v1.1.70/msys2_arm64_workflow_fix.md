# Plan v1.1.69 → v1.1.70: MSYS2 ARM64 Workflow Fix

## Context
A previous workflow step for Windows ARM64 attempted to run the GitHub Action `msys2/setup-msys2@v2` as a PowerShell command, which is invalid. This caused errors such as:

```
The term 'msys2/setup-msys2@v2' is not recognized as a name of a cmdlet, function, script file, or executable program.
```

## Change Summary
- **Removed** the invalid PowerShell step that tried to invoke the GitHub Action as a command.
- **Ensured** that MSYS2 setup for ARM64 uses the correct `uses: msys2/setup-msys2@v2` syntax, matching the x86_64 approach.
- **Verified** that all MSYS2 and MinGW installation steps for ARM64 are handled via proper GitHub Actions YAML, not shell commands.

## Motivation
- Prevent workflow errors and ensure proper setup of MSYS2 on Windows ARM64 runners.
- Align workflow steps for ARM64 and x86_64 for maintainability and clarity.

## Files Changed
- `.github/workflows/build-libs.yaml`: Removed the invalid PowerShell step and confirmed correct usage of `uses: msys2/setup-msys2@v2` for ARM64.

## Version
- From: v1.1.69
- To:   v1.1.70

---
**Date:** 2026-04-19
**Author:** GitHub Copilot
