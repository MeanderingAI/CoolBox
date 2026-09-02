# GHCR Owner Lowercase For LSP Images (v1.1.36 -> v1.1.37)

## Summary
This document records the GitHub Container Registry tag fix applied during the v1.1.36 to v1.1.37 transition for LSP Docker image publishing.

## Issue Addressed

### LSP image push failed when the repository owner contained uppercase characters
- The `Build & push plang LSP image` step failed during Docker tag validation.
- The failing tag used `ghcr.io/${{ github.repository_owner }}/...` directly.
- Docker rejected the tag because the repository name component must be lowercase.

## Root Cause
- The workflows used `github.repository_owner` directly when constructing GHCR image tags.
- GitHub repository owners can contain uppercase characters.
- GHCR repository paths are validated as Docker image names, which require lowercase repository components.

## Changes Implemented

### 1. Normalize the GHCR owner to lowercase before tagging
- Added a `Normalize GHCR owner` step in the affected LSP image workflows.
- The step writes a lowercase owner value to `GITHUB_OUTPUT` using Bash lowercase expansion.

### 2. Update the release pipeline LSP image tags
- Updated `.github/workflows/build-purchase-pipeline.yaml` so the `generate_lsp_images` job uses the normalized lowercase owner for:
  - `plang-lsp`
  - `plrust-lsp`
  - `pljava-lsp`
  - `plpython-lsp`

### 3. Update standalone LSP image workflows
- Updated `.github/workflows/lsp-plang.yaml`.
- Updated `.github/workflows/lsp-rust.yaml`.
- Updated `.github/workflows/lsp-java.yaml`.
- Updated `.github/workflows/lsp-python.yaml`.
- Updated `.github/workflows/lsp-docker.yaml`.
- This keeps the same fix applied across both the release pipeline and the standalone LSP image workflows.

## Primary Files Updated
- `.github/workflows/build-purchase-pipeline.yaml`
- `.github/workflows/lsp-plang.yaml`
- `.github/workflows/lsp-rust.yaml`
- `.github/workflows/lsp-java.yaml`
- `.github/workflows/lsp-python.yaml`
- `.github/workflows/lsp-docker.yaml`

## Result
- LSP Docker image tags are now valid even when the repository owner includes uppercase characters.
- The `Build & push plang LSP image` failure mode is removed at the tag-construction layer.
- The same fix now protects the other LSP image publishing paths from the same GHCR naming issue.