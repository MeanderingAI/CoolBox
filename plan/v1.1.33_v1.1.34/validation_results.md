# Validation Results (v1.1.33 -> v1.1.34)

## Static Validation
- Validation was re-run after the LSP artifact naming and download-flow updates.
- The edited workflow files reported no diagnostics after the final patch set.

## Specific Checks Completed
- Confirmed `.github/workflows/build-libs.yaml` now uses release-scoped LSP artifact names.
- Confirmed `.github/workflows/build-libs.yaml` no longer auto-triggers on pushes to `main`.
- Confirmed `.github/workflows/build-purchase-pipeline.yaml` now downloads the updated LSP artifact names and no longer expects stale per-language artifacts.
- Confirmed `.github/workflows/docs-publish.yaml` no longer auto-triggers on pushes or pull requests against `main`.

## Final Status
- The repository now documents the LSP artifact conflict fix under the v1.1.33 to v1.1.34 release cycle.
- The workflow artifact flow is internally consistent with the updated LSP upload names.

## Remaining Note
- No live GitHub Actions run was executed from this environment after these edits.
- Final runtime validation still depends on re-running the affected workflow jobs in GitHub Actions.