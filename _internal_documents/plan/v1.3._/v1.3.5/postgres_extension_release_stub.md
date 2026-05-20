# Postgres Extension Release Stub (v1.3.5)

## Goal
Add a reusable GitHub Actions workflow that validates and packages Postgres extension scripts and publishes a `postgres-extension-*` artifact family.

## Implemented
- Added workflow: `.github/workflows/generate-purchase-postgres.yaml`.
- Workflow supports `workflow_call` from the main purchase pipeline.
- Validates binding scripts via:
  - `python _scripts/build_scripts/build_extensions.py postgres_bindings`
- Stages release archives from `_deliverables/libraries/bindings/postgres_bindings`:
  - `coolbox-postgres-bindings-linux-x86_64-<ref>.tar.gz`
  - `coolbox-postgres-bindings-linux-x86_64-<ref>.zip`
- Runs `.github/actions/finalize-purchase-assets` to generate manifest and checksums.
- Uploads artifact with naming:
  - `postgres-extension-linux-x86_64-<ref>`
- Attaches release assets on tag refs (`refs/tags/v*`).

## Pipeline Integration
- Wired into `.github/workflows/build-purchase-pipeline.yaml` as job `generate_purchase_postgres`.
- Included in diagnostics summary and dependency graph where purchase jobs are tracked.
- Included as a required dependency for docs publish gating in the main release pipeline.

## Notes
- This is a stub release path focused on packaging SQL extension scripts.
- Live Postgres validation can be expanded later by injecting `POSTGRES_DSN` and provisioning a temporary service container.
