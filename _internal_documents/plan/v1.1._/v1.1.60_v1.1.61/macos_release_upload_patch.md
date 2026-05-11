# Patch Description: macOS Release Asset Upload Robustness for R Workflow

## Summary
This patch updates the `.github/workflows/generate-purchase-r.yaml` workflow to ensure robust and platform-specific release asset uploads for macOS builds.

## Details
- **Problem:** GitHub Actions would sometimes fail with a 404 error when uploading release assets on macOS, because the release did not exist before the upload step.
- **Solution:**
  - The workflow now restricts release creation and asset upload steps to run only on macOS (`runs-on: macos-latest`).
  - Before uploading assets, the workflow explicitly creates the release using `gh release create` if it does not already exist.
  - The asset upload step uses `gh release upload` and is guarded to only run after the release is confirmed to exist.
- **Effect:**
  - Prevents 404 errors due to missing releases.
  - Ensures only macOS builds handle release asset uploads, avoiding duplicate or failed uploads from other platforms.

## Motivation
This change was made to address CI/CD failures and ensure that release assets are reliably uploaded for macOS builds, following best practices for GitHub Actions release management.

## Reference
- File: `.github/workflows/generate-purchase-r.yaml`
- Change applied: v1.1.60 → v1.1.61
