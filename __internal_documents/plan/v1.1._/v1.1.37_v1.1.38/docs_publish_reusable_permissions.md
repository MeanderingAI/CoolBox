# Docs Publish Reusable Workflow Permissions

## Summary
- Added explicit caller-side permissions for the `docs_publish` reusable workflow invocation.
- Granted `contents: write`, `pages: write`, and `id-token: write` on the `docs_publish` job in the release pipeline.

## Problem
- The reusable workflow `.github/workflows/docs-publish.yaml` requests `pages: write` and `id-token: write`.
- The caller workflow only granted `contents: write` at the top level.
- GitHub Actions rejected the reusable workflow call because the caller only allowed `pages: none` and `id-token: none`.

## Files Updated
- `.github/workflows/build-purchase-pipeline.yaml`

## Result
- The `docs_publish` reusable workflow call now grants the scopes required by the called workflow.
- The rest of the release pipeline still keeps narrower default permissions.
