# Purchase Pipeline Generate Purchase Rename

## Summary
- Renamed the purchase extension generator jobs from `build_ext_*` to `generate_purchase_*` in the release pipeline.
- Renamed the reusable workflow files from `build-ext-*.yaml` to `generate-purchase-*.yaml`.
- Removed redundant `if:` guards from the purchase generator caller jobs and from the reusable workflow job bodies.

## Motivation
- The old `build_ext_*` naming no longer matched the intent of the jobs.
- The redundant conditions contributed to confusing skipped-job behavior in GitHub Actions.
- The workflow-call sites, workflow filenames, and internal job ids needed to be kept in sync.

## Files Updated
- `.github/workflows/build-purchase-pipeline.yaml`
- `.github/workflows/generate-purchase-python.yaml`
- `.github/workflows/generate-purchase-js.yaml`
- `.github/workflows/generate-purchase-go.yaml`
- `.github/workflows/generate-purchase-c.yaml`
- `.github/workflows/generate-purchase-java.yaml`
- `.github/workflows/generate-purchase-r.yaml`
- `.github/workflows/generate-purchase-rust.yaml`

## Result
- The release pipeline now refers only to `generate_purchase_*` jobs.
- The reusable workflow filenames and `uses:` targets now match.
- Obsolete `build-ext-*.yaml` files were removed after the new workflow files were created.
