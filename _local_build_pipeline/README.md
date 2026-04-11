# Local Build Pipeline

This folder provides a local approximation of the GitHub Actions workflows using Docker-based Linux runners where practical, plus an `act` wrapper for workflows that are still GitHub-centric.

## Goals
- Reproduce the Linux portions of the main CI workflows before pushing.
- Catch build and test failures locally in a clean container.
- Provide a consistent entrypoint for workflows that can be approximated without GitHub-hosted services.

## What This Supports
- Docker-local runners:
  - `build-libs.yaml`
  - `build-products.yaml`
  - `build-cpp-docs.yaml`
  - `build-tutorials.yaml`
  - `build-purchase-pipeline.yaml` as a local Linux-only subset runner
  - `generate-purchase-c.yaml` as a Linux-local runner
  - `generate-purchase-python.yaml` as a Linux-local runner
  - `generate-purchase-go.yaml` as a Linux-local runner
  - `generate-purchase-js.yaml` as a Linux-local runner
  - `generate-purchase-rust.yaml` as a Linux-local runner
  - `generate-purchase-r.yaml` as a Linux-local runner
  - `generate-purchase-java.yaml` as a Linux-local runner
  - `docs-publish.yaml` as a dry-run only
  - `lsp-vlang.yaml` as a Docker-local LSP image build
  - `lsp-c3.yaml` as a Docker-local LSP image build
- `act` fallback:
  - workflows with `workflow_dispatch` that do not have a dedicated Docker runner here

## Workflow Support Matrix
- `build-libs.yaml`: Docker-local runner
- `build-products.yaml`: Docker-local runner
- `build-cpp-docs.yaml`: Docker-local runner
- `build-tutorials.yaml`: Docker-local runner
- `build-purchase-pipeline.yaml`: Docker-local subset runner
- `generate-purchase-c.yaml`: Docker-local Linux runner
- `generate-purchase-go.yaml`: Docker-local Linux runner
- `generate-purchase-java.yaml`: Docker-local Linux runner
- `generate-purchase-js.yaml`: Docker-local Linux runner
- `generate-purchase-python.yaml`: Docker-local Linux runner
- `generate-purchase-r.yaml`: Docker-local Linux runner
- `generate-purchase-rust.yaml`: Docker-local Linux runner
- `docs-publish.yaml`: Docker dry-run helper or `act`
- `lsp-vlang.yaml`: Docker-local runner
- `lsp-c3.yaml`: Docker-local runner
- `ci.yaml`: no direct local runner; use targeted workflow scripts instead
- `lsp-docker.yaml`: no dedicated local runner; depends on `workflow_run`, artifacts, and registry push
- `lsp-java.yaml`: no dedicated local runner; depends on `workflow_run`, artifacts, and registry push
- `lsp-plang.yaml`: no dedicated local runner; depends on `workflow_run`, artifacts, and registry push
- `lsp-python.yaml`: no dedicated local runner; depends on `workflow_run`, artifacts, and registry push
- `lsp-rust.yaml`: no dedicated local runner; depends on `workflow_run`, artifacts, and registry push

## What This Does Not Fully Reproduce
- `generate-purchase-*.yaml`
  - these depend on downloaded workflow artifacts, per-language packaging toolchains, and GitHub artifact behavior
- `lsp-*.yaml`
  - these depend on `workflow_run`, downloaded artifacts, registry publishing, or Docker image pushes
- release upload and Pages publish
  - local scripts stop short of `gh release upload`, GHCR pushes, and GitHub Pages deployment

## Requirements
- `docker`
- `bash`
- optional: `act` for workflows without a direct Docker runner

## First Run
Build the local Linux image:

```bash
_local_build_pipeline/scripts/run_workflow.sh build-libs.yaml --rebuild-image
```

## Common Commands
Run the Linux build-and-test workflow locally:

```bash
_local_build_pipeline/scripts/run_workflow.sh build-libs.yaml
```

Run just the configured tests against a local CI-style build:

```bash
_local_build_pipeline/scripts/run_workflow.sh build-tests
```

Run the Linux product build locally:

```bash
_local_build_pipeline/scripts/run_workflow.sh build-products.yaml
```

Run the local subset of the purchase pipeline:

```bash
_local_build_pipeline/scripts/run_workflow.sh build-purchase-pipeline.yaml
```

Run one Linux-local purchase generator directly:

```bash
_local_build_pipeline/scripts/run_workflow.sh generate-purchase-python.yaml
```

Run a local LSP image build for the V or C3 extensions:

```bash
_local_build_pipeline/scripts/run_workflow.sh lsp-vlang.yaml
_local_build_pipeline/scripts/run_workflow.sh lsp-c3.yaml
```

Run all locally supported Docker workflows:

```bash
_local_build_pipeline/scripts/run_all_supported.sh
```

Attempt a workflow with `act` instead:

```bash
_local_build_pipeline/scripts/run_workflow.sh docs-publish.yaml --act
```

Windows PowerShell wrapper:

```powershell
./_local_build_pipeline/run_workflow.ps1 -Workflow build-libs.yaml -RebuildImage
```

## Notes
- The Docker runner copies the repository into a container-local `/workspace` before running builds. This avoids Windows-mounted filesystem issues with tools like `FetchContent` that delete and recreate directories during configure.
- Logs and selected outputs are synced back to `_local_build_pipeline/out/<job-name>/` on the host after each Docker run.
- The Docker scripts intentionally remove `build/` before configuring inside the container-local workspace so they behave more like CI.
- `build_libraries` in the repo Makefile currently builds the default CMake graph, so local CI reproduction will also compile test executables when `BUILD_TESTING=ON`.
- The `docs-publish.yaml` local runner is a dry-run helper only. It prepares prebuilt docs inputs but does not publish to GitHub Pages.
- The `generate-purchase-*` local runners are Linux approximations. They skip GitHub artifact download/upload and release attachment steps, but they execute the core build/package logic locally.
- `build-tests` is a convenience local target, not a GitHub workflow file. It runs `make test` inside the local Linux CI container, configuring first if needed.
- `lsp-vlang.yaml` and `lsp-c3.yaml` local runners build the corresponding LSP binary, run its focused test target, stage the binary into `apps/lsp/dist`, and build the matching Docker image locally.
