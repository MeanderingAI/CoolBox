# Docs Publish Environment Protection Workaround

## Summary
- Adjusted the release pipeline so the reusable `docs_publish` workflow is no longer invoked on tag-triggered release runs.
- Limited docs publishing from the release pipeline to manual `workflow_dispatch` runs.

## Problem
- The reusable docs workflow deploys through the `github-pages` environment.
- Repository environment protection rules rejected tag refs such as `v1.1.38` from deploying to `github-pages`.
- This caused the `build_and_publish` job inside `docs_publish` to be rejected even though workflow permissions and Pages actions were already configured correctly.

## Files Updated
- `.github/workflows/build-purchase-pipeline.yaml`

## Result
- Tag-based release runs no longer fail because of `github-pages` environment protection rules.
- Documentation publishing remains available through manual dispatch.
- Automatic tag-based Pages deployment can be restored later by changing the `github-pages` environment rules in repository settings.
