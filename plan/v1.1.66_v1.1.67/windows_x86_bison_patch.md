# Patch: Fix Bison Install for Windows x86_64 (v1.1.66 → v1.1.67)

## Summary
Corrects the MSYS2 dependency installation for Windows x86_64 by replacing the non-existent `mingw-w64-x86_64-bison` package with the correct native `bison` package.

## Changes
- Updates the install step in `.github/workflows/build-libs.yaml`:
  - Uses `pacman -S --noconfirm --needed mingw-w64-x86_64-gsl bison` for Windows x86_64.

## Motivation
- Fixes workflow error: `error: target not found: mingw-w64-x86_64-bison`.
- Ensures Bison is installed for MinGW builds on Windows x86_64.

## Patch Location
- .github/workflows/build-libs.yaml

---

**Author:** GitHub Copilot
**Date:** 2026-04-19
