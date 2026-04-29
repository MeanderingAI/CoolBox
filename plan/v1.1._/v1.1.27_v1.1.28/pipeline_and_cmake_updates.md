# Pipeline and CMake Updates (v1.1.27 -> v1.1.28)

## Summary
This document records the CI and build-pipeline changes completed during the v1.1.27 to v1.1.28 transition.

## Issues Addressed

### 1. Preserve package CMake requirements at 4.3.1
- Kept the package-level `cmake_minimum_required(VERSION 4.3.1)` declarations in the source-controlled subpackages.
- Avoided weakening source requirements just to match the runner default CMake version.
- Restored the affected package CMake files after the temporary compatibility rollback.

### 2. Fix pipeline compatibility in the workflow layer
- Moved the compatibility fix into GitHub Actions workflows instead of changing package requirements.
- Added explicit CMake setup steps using `jwlawson/actions-setup-cmake@v2`.
- Pinned the workflow CMake version to `4.3.1`.
- Added version verification steps so the workflow fails if another CMake version is on `PATH`.

### 3. Removed runner/package-manager ambiguity
- Stopped relying on package-manager-installed CMake where the workflow should use the pinned GitHub Action version.
- Removed package-manager CMake installs from the workflows that configure the root project.
- Simplified artifact flow in the top-level release pipeline by removing unnecessary `run_id` wiring for same-run artifact downloads.

## Primary Files Updated
- `.github/workflows/build-libs.yaml`
- `.github/workflows/build-libs-main.yaml`
- `.github/workflows/build-purchase-pipeline.yaml`
- `.github/workflows/ci.yaml`

## Result
- Source-controlled package CMake requirements remain at `4.3.1`.
- The CI pipeline is configured to install and verify CMake `4.3.1` explicitly.
- The top-level pipeline no longer depends on reusable-workflow output plumbing for same-run artifact fetches.
