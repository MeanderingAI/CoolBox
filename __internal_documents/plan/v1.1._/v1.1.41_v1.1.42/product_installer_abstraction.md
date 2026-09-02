# Product Installer Abstraction

## Summary
- Added `_libraries/packages/OS_GENERICS/installer_abstraction` as a release-oriented packaging helper for products.
- Modeled prerelease packaging around three installation experiences: Windows `Install Genie`, macOS `Drag And Drop Bundle`, and Linux or other platforms as a `Portable Binary`.
- Kept the abstraction focused on packaged products and prerelease guidance rather than forcing installer behavior into ordinary development builds.

## Library Design
- `InstallerExperience` selects the expected install surface for the current operating system.
- `ReleaseChannel` distinguishes prerelease and stable output paths so packaging text can evolve without changing product code shape.
- `PackageInstallerPlan` captures product name, executable name, installer mode, and install/package/launch hints.
- `PreReleaseScreen` captures the richer product-facing prerelease summary shown inside packaged products.
- Rendering helpers produce both installation notes and prerelease text so the same semantics can be reused by products and release automation.

## Product Integration
- `MStudio`, `file_browser`, and `bower_shell` now compile two paths:
  - a normal local-build startup path with no installer abstraction dependency
  - a release-packaged prerelease path enabled only when installer abstractions are linked
- Product CMake files only link `installer_abstraction` when the target exists and set `COOLBOX_PRODUCT_INSTALLER_ABSTRACTIONS=1` in that case.
- This keeps local development and makefile-based builds free from packaging-only behavior while still allowing packaged previews to present installation guidance.

## Workflow Integration
- Added a release-only top-level CMake option, `BUILD_PRODUCT_INSTALLER_ABSTRACTIONS`, so `OS_GENERICS` packaging helpers are opt-in.
- Enabled that option in `.github/workflows/build-libs.yaml` and left it disabled for ordinary local builds.
- Added `installer_abstraction_notes`, a small generator executable that renders `PRERELEASE.txt` and `INSTALL.txt` from the shared abstraction instead of duplicating those strings in workflow YAML.
- The release packaging step now stages product binaries, copies adjacent runtime libraries, and writes package notes through the generator before archiving each product.

## Current Scope
- The current implementation provides a generic prerelease packaging interface and shared messaging layer.
- It does not yet ship a full native Windows installer executable, a finalized macOS drag-and-drop app bundle pipeline, or richer Linux package formats.
- The main goal of this revision is to make packaged products explain their expected install path clearly while preserving a clean local-development path.

## Result
- Product prerelease/install messaging now comes from one source of truth shared between product code and release packaging.
- The release workflow can package products with OS-aware guidance without leaking that behavior into makefile-driven builds.
- The active plan folder now records the installer abstraction, workflow integration, and release-only gating work explicitly.