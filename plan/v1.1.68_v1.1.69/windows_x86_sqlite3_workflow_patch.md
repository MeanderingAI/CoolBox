
# Patch: Add SQLite3 to Windows x86_64 Workflow (v1.1.67 → v1.1.68)

## Context
A CMake error was reported for missing SQLite3 during the Windows x86_64 build. The workflow previously installed `mingw-w64-x86_64-gsl` and `bison` but omitted the required `mingw-w64-x86_64-sqlite3` package.

## Change
- Updated `.github/workflows/build-libs.yaml`:
	- The Windows x86_64 dependency installation step now includes `mingw-w64-x86_64-sqlite3`.

## Patch
```yaml
- pacman -S --noconfirm --needed mingw-w64-x86_64-gsl bison
+ pacman -S --noconfirm --needed mingw-w64-x86_64-gsl mingw-w64-x86_64-sqlite3 bison
```

## Rationale
This ensures that the SQLite3 library is available for CMake to find and link against, resolving the build error on Windows x86_64.

## Version
- From: v1.1.67
- To:   v1.1.68

---
**Date:** 2026-04-19
**Author:** GitHub Copilot
