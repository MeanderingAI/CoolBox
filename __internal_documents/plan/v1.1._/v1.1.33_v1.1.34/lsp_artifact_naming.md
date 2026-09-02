# LSP Artifact Naming (v1.1.33 -> v1.1.34)

## Summary
This document records the LSP artifact naming fix applied during the v1.1.33 to v1.1.34 transition.

## Issue Addressed

### LSP artifact uploads collided within the same workflow run
- The `build-libs` matrix uploaded LSP artifacts using fixed artifact names such as `pl_lsp_dist`.
- Multiple matrix jobs attempted to create artifacts with the same name in the same workflow run.
- GitHub Actions rejected the later uploads with a non-retryable `409 Conflict` because the artifact name already existed.

## Changes Implemented

### 1. Make LSP artifact names unique per platform and release
- Updated `.github/workflows/build-libs.yaml` so LSP artifact names include `${{ matrix.platform }}` and `${{ github.ref_name }}`.
- Consolidated the main-branch build path onto `.github/workflows/build-libs.yaml` so the release-scoped artifact naming logic now lives in one workflow.

### 2. Align downstream downloads with the new names
- Updated `.github/workflows/build-purchase-pipeline.yaml` to download the Linux x86_64 LSP dist archive using the new release-scoped artifact name.
- Updated the fallback binary download to use the new combined LSP binary artifact name.
- Updated diagnostics artifact collection to match the new wildcard patterns.

### 3. Remove stale per-language artifact download steps
- The release pipeline already relied on one combined fallback LSP binary artifact rather than separate per-language artifacts.
- Removed the stale per-language LSP artifact download steps from the release pipeline so the download flow matches the artifacts that are actually produced.

## Primary Files Updated
- `.github/workflows/build-libs.yaml`
- `.github/workflows/build-purchase-pipeline.yaml`

## Result
- LSP artifact uploads no longer collide across matrix jobs in the same workflow run.
- Release and diagnostics jobs now consume the new release-scoped artifact names consistently.
- The LSP artifact flow is simpler and better aligned with what the build workflow actually uploads.