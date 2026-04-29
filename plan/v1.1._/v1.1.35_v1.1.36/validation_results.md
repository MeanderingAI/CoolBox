# Validation Results (v1.1.35 -> v1.1.36)

## Static Validation
- Validation was re-run after the documentation workflow Pages action upgrade.
- `.github/workflows/docs-publish.yaml` reported no diagnostics after the change.

## Static Validation
- Validation was re-run after the GHCR owner normalization updates.
- The edited LSP image workflow files reported no diagnostics after the change.

## Specific Checks Completed
- Confirmed the release pipeline `Build & push plang LSP image` path now uses a lowercase-normalized owner output when constructing GHCR tags.
- Confirmed the same lowercase owner normalization was applied to the other LSP image tags in the release pipeline.
- Confirmed the standalone LSP image workflows now use the same lowercase owner normalization pattern.

## Remaining Note
- No live GitHub Actions run was executed from this environment after the update.
- Final runtime validation still depends on re-running the affected LSP image jobs in GitHub Actions.

## Specific Checks Completed
- Confirmed `actions/upload-pages-artifact@v1` was replaced with `actions/upload-pages-artifact@v3`.
- Confirmed `actions/deploy-pages@v1` was replaced with `actions/deploy-pages@v4`.
- Confirmed `pages: write` and `id-token: write` were added to the workflow permissions.
- Confirmed the `build_and_publish` job now targets the `github-pages` environment.

## Remaining Note
- No live GitHub Actions run was executed from this environment after the update.
- Final runtime validation still depends on re-running the `docs_publish / build_and_publish` job in GitHub Actions.