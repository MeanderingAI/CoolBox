# Workflow Makefile Delegation (v1.1.32 -> v1.1.33)

## Summary
This document records the CI workflow simplification completed during the v1.1.32 to v1.1.33 transition by delegating library and test orchestration to the repository Makefiles.

## Issues Addressed

### 1. Workflow build logic was too verbose and brittle
- The `build-libs` workflow previously maintained CI-specific shell logic to discover and build libraries and tests.
- That logic duplicated repository build behavior and had already caused parser and shell-compatibility issues across Linux and macOS runners.

### 2. Build and test behavior was split between workflow code and project build scripts
- The repository already had `Makefile` and `Makefile.win` entry points for `build_libraries` and `test`.
- Keeping equivalent orchestration logic inside the workflow made the pipeline harder to read and easier to break.

## Changes Implemented

### 1. Delegate library builds to repository Makefiles
- Updated `.github/workflows/build-libs.yaml` so the `Build all libraries` step calls `make build_libraries` on Linux/macOS and `make -f Makefile.win build_libraries` on Windows.
- Simplified `Makefile` so `build_libraries` delegates directly to `cmake --build build` instead of scraping source `CMakeLists.txt` files.

### 2. Delegate test execution to repository Makefiles
- Updated `.github/workflows/build-libs.yaml` so the test step calls `make test` on Linux/macOS and `make -f Makefile.win test` on Windows.
- Simplified the Unix `Makefile` test target to build configured binaries, list configured CTest suites, and run registered tests directly through CTest.

### 3. Make configure guards compatible with workflow-created build trees
- Updated Makefile configure guards to check for `build/CMakeCache.txt` instead of generator-specific files such as `build/Makefile`.
- This keeps the Makefile wrappers compatible with build directories already configured by the GitHub Actions workflow.

## Primary Files Updated
- `.github/workflows/build-libs.yaml`
- `Makefile`
- `Makefile.win`

## Result
- The workflow is shorter and easier to read.
- Build and test orchestration now lives in the repository build scripts rather than duplicated CI shell snippets.
- The workflow now relies on CMake and CTest through the project Makefiles instead of target-name scraping.

## Windows Follow-up
- The initial Makefile delegation exposed a quoting problem in `Makefile.win` when it was invoked from Bash-based GitHub Actions steps.
- PowerShell variables inside `-Command "..."` were expanded by the outer shell before PowerShell executed, which broke Visual Studio detection and configure argument construction.
- The Windows configure command was updated to use Bash-safe single-quoted PowerShell script text so the embedded PowerShell variables and `&` invocation operator are preserved correctly.