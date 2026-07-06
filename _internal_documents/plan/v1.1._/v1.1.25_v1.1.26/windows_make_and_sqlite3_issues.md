# Windows Make & SQLite3 Issues (v1.1.25 → v1.1.26)

## Summary
This document tracks all issues and fixes encountered during the transition from v1.1.25 to v1.1.26 related to Windows Makefile.win and SQLite3 integration.

## Issues Encountered

### 1. Windows Makefile.win
- PowerShell quoting and variable escaping issues (`$` vs `$$` in Makefile).
- Generator auto-detection logic did not support Visual Studio 2026.
- Stale or cached build scripts caused old typos to persist.
- Makefile indentation and target definition errors prevented configure/build steps from running.
- Batch job/terminal lockups during clean/build.

### 2. SQLite3 Integration
- CMake could not find SQLite3 (missing headers/libraries).
- No bundled sqlite3.h/.lib/.dll or FindSQLite3.cmake in repo.
- Custom FindSQLite3.cmake needed for cross-platform support.
- Syntax errors in FindSQLite3.cmake: unterminated variable references (e.g., `$ENV{ProgramFiles`), missing whitespace, and path issues.
- CMake warnings about argument separation and unterminated variables.

## Fixes Applied
- Escaped all PowerShell variables in Makefile.win with `$$`.
- Added Visual Studio 18 2026 support to generator detection.
- Added custom `cmake/FindSQLite3.cmake` with OS-specific search logic.
- Updated CMakeLists.txt to set `CMAKE_MODULE_PATH` for custom find modules.
- Documented all issues and fixes in this file.

## Next Steps
- Fix syntax errors in FindSQLite3.cmake (unterminated variables, whitespace).
- Test build on all platforms after fixes.
- Document any further issues in this folder.
