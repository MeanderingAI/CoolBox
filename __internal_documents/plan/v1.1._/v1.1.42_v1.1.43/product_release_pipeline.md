# Product Release Pipeline Split

## Summary
- Pulled product building and product packaging out of the main library build job in `.github/workflows/build-libs.yaml`.
- Added a dedicated `build-products` job that runs after the `release` job and handles product builds and product archive uploads separately from library release creation.

## Workflow Changes
- The main `build` job now stays focused on library builds, tests, and library asset packaging.
- The `release` job still creates or updates the GitHub release from the aggregated library assets.
- The new `build-products` job:
  - depends on `release`
  - uses a per-platform matrix
  - configures the repository with `BUILD_PRODUCTS=ON` and `BUILD_PRODUCT_INSTALLER_ABSTRACTIONS=ON`
  - builds `MStudio`, `file_browser`, and `bower_shell`
  - packages prerelease product archives and uploads them to the created release

## Target Matrix
- The dedicated product job now attempts all configured product platforms:
  - `linux-x86_64`
  - `linux-arm64`
  - `macos-arm64`
  - `macos-x86_64`
  - `windows-x86_64`
  - `windows-arm64`

## Packaging Notes
- Product archives still include `PRERELEASE.txt` and `INSTALL.txt` generated from the shared installer abstraction notes tool.
- Product release assets are uploaded both as workflow artifacts and as release attachments.

## Result
- Product packaging is now a dedicated post-release concern rather than being interleaved with the core library release build.