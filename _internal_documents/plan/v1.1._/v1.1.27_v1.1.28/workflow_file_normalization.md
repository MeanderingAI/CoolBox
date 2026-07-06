# Workflow File Normalization (v1.1.27 -> v1.1.28)

## Summary
This document records the GitHub Actions workflow-file normalization completed during the v1.1.27 to v1.1.28 transition.

## Changes Made

### 1. Replaced `.yml` workflow files with `.yaml`
- Removed the old `.yml` workflow files from `.github/workflows`.
- Recreated the workflow set using `.yaml` extensions only.
- Updated workflow references so local reusable-workflow `uses:` paths point to the new `.yaml` files.

### 2. Replaced underscore-heavy workflow filenames with hyphenated names
- Normalized workflow filenames to use hyphens instead of underscores.
- Updated README references and internal workflow-call references accordingly.

### 3. Split the two build-library workflows into distinct final names
- Kept the reusable matrix/release workflow as:
  - `.github/workflows/build-libs.yaml`
- Kept the direct main-branch workflow as:
  - `.github/workflows/build-libs-main.yaml`
- This avoids filename collisions after normalization while preserving both workflows.

## Final Workflow Set
- `.github/workflows/build-cpp-docs.yaml`
- `.github/workflows/build-ext-c.yaml`
- `.github/workflows/build-ext-go.yaml`
- `.github/workflows/build-ext-java.yaml`
- `.github/workflows/build-ext-js.yaml`
- `.github/workflows/build-ext-python.yaml`
- `.github/workflows/build-ext-r.yaml`
- `.github/workflows/build-ext-rust.yaml`
- `.github/workflows/build-libs.yaml`
- `.github/workflows/build-libs-main.yaml`
- `.github/workflows/build-purchase-pipeline.yaml`
- `.github/workflows/build-tutorials.yaml`
- `.github/workflows/ci.yaml`
- `.github/workflows/docs-publish.yaml`
- `.github/workflows/lsp-docker.yaml`
- `.github/workflows/lsp-java.yaml`
- `.github/workflows/lsp-plang.yaml`
- `.github/workflows/lsp-python.yaml`
- `.github/workflows/lsp-rust.yaml`

## Result
- No `.yml` workflow files remain in `.github/workflows`.
- Workflow-call paths and documentation references are aligned to the normalized `.yaml` names.
