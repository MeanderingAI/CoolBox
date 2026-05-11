# Workflow Consolidation: build-libs

## Summary
This note records the consolidation of the previous split between `.github/workflows/build-libs.yaml` and `.github/workflows/build-libs-main.yaml`.
The duplicate `build-libs-main.yaml` workflow was removed so `build-libs.yaml` is now the single workflow for both reusable release builds and direct pushes to `main`.

## What `build-libs.yaml` Does Now

### Role
- `build-libs.yaml` is the reusable primary library build workflow.
- It is designed to be called from other workflows through `workflow_call`.
- It can also be started manually with `workflow_dispatch`.

### Behavior
- Runs a multi-platform matrix build.
- Covers Linux x86_64, Linux arm64, macOS arm64, macOS x86_64, Windows x86_64, and Windows arm64.
- Installs platform-specific dependencies.
- Configures CMake with testing enabled.
- Delegates the actual build and test execution to `Makefile` or `Makefile.win`.
- Uploads test results.
- Packages and uploads LSP artifacts.
- Packages release assets by category.

### Why It Exists
- This is the workflow the release pipeline depends on.
- In `.github/workflows/build-purchase-pipeline.yaml`, the `build_libs` job calls `./.github/workflows/build-libs.yaml`.
- That makes `build-libs.yaml` the single shared build entry point for release-oriented automation.

## What Changed

### Removed Workflow
- `.github/workflows/build-libs-main.yaml` was removed.

### Trigger Change
- `.github/workflows/build-libs.yaml` remains callable through `workflow_call` and `workflow_dispatch`, but no longer auto-triggers from `main` branch pushes.

### Operational Result
- There is no longer a separate Linux-only workflow that duplicates part of the build path.
- All main-branch build execution now goes through the same workflow file that release orchestration already uses.

## Consequences
- Main-branch pushes no longer start this workflow automatically.
- Artifact naming, build steps, and test orchestration now come from one source of truth.
- Future build changes only need to be made in one workflow file.

## Rationale
- This removes workflow duplication.
- This removes naming confusion.
- This reduces the risk that `main` builds and release builds drift apart again.