# Swift Extension Release Stub (v1.3.5)

## Goal
Add a reusable GitHub Actions workflow that packages Swift bindings and publishes a `swift-extension-*` artifact family.

## Implemented
- Added workflow: `.github/workflows/generate-purchase-swift.yaml`.
- Workflow supports `workflow_call` from the main purchase pipeline.
- Builds Swift bindings from `_deliverables/libraries/bindings/swift_bindings`.
- Stages release archives:
  - `coolbox-swift-bindings-linux-x86_64-<ref>.tar.gz`
  - `coolbox-swift-bindings-linux-x86_64-<ref>.zip`
- Runs `.github/actions/finalize-purchase-assets` to generate manifest and checksums.
- Uploads artifact with naming:
  - `swift-extension-linux-x86_64-<ref>`
- Attaches release assets on tag refs (`refs/tags/v*`).

## Pipeline Integration
- Wired into `.github/workflows/build-purchase-pipeline.yaml` as job `generate_purchase_swift`.
- Included in diagnostics summary and dependency graph where purchase jobs are tracked.
- Included as a required dependency for docs publish gating in the main release pipeline.

## Notes
- This is a stub release path focused on Linux packaging.
- Future follow-up can add macOS/Windows matrix packaging once runtime/linking expectations are finalized.
