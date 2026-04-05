# Docs Publish Tag Trigger Restore

## Summary
- Restored `docs_publish` so it can run during tag-triggered release executions as well as manual workflow dispatches.
- Removed the release-pipeline condition that was forcing the documentation publish job to be skipped on tag runs.

## Problem
- In `.github/workflows/build-purchase-pipeline.yaml`, the `docs_publish` job had been restricted to `github.event_name == 'workflow_dispatch'`.
- That meant version-tagged release runs could build the docs prerequisites successfully and still skip `docs_publish` entirely.
- The skip reason was not a runtime failure in the reusable docs workflow; it was caused by the caller job's explicit `if:` guard.

## Files Updated
- `.github/workflows/build-purchase-pipeline.yaml`

## Result
- `docs_publish` now runs when:
  - `build_cpp_docs` succeeds,
  - `build_tutorials` succeeds, and
  - the event is either `workflow_dispatch` or a `refs/tags/v*` release tag.
- Tag-driven release runs are no longer blocked from invoking the reusable docs publishing workflow.
