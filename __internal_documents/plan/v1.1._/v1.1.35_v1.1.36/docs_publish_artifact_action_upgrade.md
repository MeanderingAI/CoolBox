# Docs Publish Artifact Action Upgrade (v1.1.35 -> v1.1.36)

## Summary
This document records the GitHub Pages workflow fix applied during the v1.1.35 to v1.1.36 transition for the `docs_publish / build_and_publish` failure caused by deprecated artifact action usage.

## Issue Addressed

### GitHub Pages publish path failed due to deprecated artifact action usage
- The `docs_publish / build_and_publish` job failed automatically with a deprecation error referencing `actions/upload-artifact: v3`.
- The direct workflow YAML did not contain an explicit `actions/upload-artifact@v3` reference in the affected path.
- The failing dependency came from the older GitHub Pages artifact/deploy action chain used by the documentation workflow.

## Root Cause
- `.github/workflows/docs-publish.yaml` still used `actions/upload-pages-artifact@v1`.
- The workflow also used `actions/deploy-pages@v1`.
- That older Pages publish path was outdated relative to the current artifact and Pages deployment stack enforced by GitHub Actions.

## Changes Implemented

### 1. Upgrade the Pages artifact upload action
- Updated `.github/workflows/docs-publish.yaml` from `actions/upload-pages-artifact@v1` to `actions/upload-pages-artifact@v3`.

### 2. Upgrade the Pages deploy action
- Updated `.github/workflows/docs-publish.yaml` from `actions/deploy-pages@v1` to `actions/deploy-pages@v4`.

### 3. Add the required Pages deployment permissions and environment
- Added `pages: write` and `id-token: write` to the workflow permissions block.
- Added the `github-pages` environment to the `build_and_publish` job.
- This aligns the workflow with the current GitHub Pages deployment requirements for OIDC-based deployments.

## Primary File Updated
- `.github/workflows/docs-publish.yaml`

## Result
- The documentation publish workflow no longer relies on the deprecated Pages action path.
- The Pages upload and deploy steps now use supported action majors.
- The workflow permissions and environment configuration now match the current Pages deployment model.