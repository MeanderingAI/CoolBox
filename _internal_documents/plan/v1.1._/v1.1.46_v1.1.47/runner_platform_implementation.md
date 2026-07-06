# Runner Platform Implementation Plan

## Goal
- Turn the platform-runner strategy into an actionable repository plan for `.github/workflows` and `_local_build_pipeline`.
- Preserve Linux Docker as the existing clean-room baseline.
- Add native Windows and native macOS validation paths that mirror the Linux logical jobs where practical.

## Current Repository State
- `.github/workflows/build-libs.yaml` already runs on Linux, macOS, and Windows through GitHub-hosted runners.
- `_local_build_pipeline` currently approximates GitHub Actions primarily through Docker-backed Linux runners.
- `_local_build_pipeline/run_workflow.ps1` is already available as a Windows PowerShell entrypoint, but it delegates to the Linux Docker workflow runner.
- Focused PowerShell wrapper support already exists for targeted Windows-host validation in `_local_build_pipeline/scripts/validate_lsp_target.ps1`.

## Execution Model

### Linux
- Keep Docker as the primary local clean-room execution backend.
- Keep `_local_build_pipeline/scripts/run_workflow.sh` as the Linux/Docker dispatcher.
- Keep `_local_build_pipeline/docker/linux-ci.Dockerfile` as the reproducible CI image baseline.

### Windows
- Add a native Windows local execution backend for selected jobs.
- Use PowerShell wrappers plus host tools instead of trying to mirror Linux container behavior exactly.
- Restrict any future Windows-container support to narrow CLI/service packaging cases only.

### macOS
- Add a native macOS local execution backend for selected jobs.
- Use shell scripts running directly on the host or macOS VM, not Docker.
- Treat macOS validation as a first-class native backend, not a container backend.

## Planned Repository Changes

## 1. Add Backend-Aware Local Runner Dispatch
- Extend `_local_build_pipeline/scripts/run_workflow.sh` with an explicit backend selector.
- Proposed interface:
  - `--backend docker-linux`
  - `--backend native-windows`
  - `--backend native-macos`
- Keep the current Linux Docker path as the default when no backend is specified.
- Update `_local_build_pipeline/run_workflow.ps1` to pass an explicit backend for Windows-native runs.

## 2. Add Native Windows Job Scripts
- Add Windows-native equivalents for the highest-value existing Linux jobs:
  - `build-libs`
  - `build-tests`
  - `build-products`
  - focused LSP validation jobs
- Proposed file additions under `_local_build_pipeline/scripts/jobs/`:
  - `build-libs.windows.ps1`
  - `build-tests.windows.ps1`
  - `build-products.windows.ps1`
  - `build-lsp-target.windows.ps1`
- Reuse the existing MSYS2/MinGW and PowerShell patterns already visible in `.github/workflows/build-libs.yaml`.
- Standardize outputs under `_local_build_pipeline/out/<job-name>/` to match the Docker-backed path shape.

### Windows Verification Notes
- Local verification on a Windows host exposed two concrete runner bugs in the first native `build-libs` implementation:
  - `_local_build_pipeline/run_workflow.ps1` preferred the Windows WSL launcher at `C:\Windows\system32\bash.exe` instead of Git Bash, which prevented reliable local bash-backed workflow execution.
  - `_local_build_pipeline/scripts/jobs/build-libs.windows.ps1` originally resolved the repository root one directory too shallow and then appended to `command.log` before `Stop-Transcript`, which caused a file-lock failure.
- Both issues were corrected during verification:
  - the PowerShell wrapper now prefers Git for Windows bash over the WSL launcher
  - the native Windows runner now resolves the repository root correctly and finalizes the transcript before appending exit status
- The native Windows runner was then hardened further to use an isolated per-run build directory under `_local_build_pipeline/tmp/` instead of deleting and reusing the repository-wide `build/` tree.
- That change removed the previous `build/eigen-src` cleanup lock failure and allowed the native Windows path to reach the actual CMake configure stage reliably.
- Review of the current source tree showed that the in-tree `quic_transport` component is now a pure internal C++ implementation and the top-level CMake already documents external QUIC/HTTP3 dependency support as disabled.
- Based on that verified usage state, the remaining legacy `quiche` wiring was removed from the default repository path entirely:
  - the top-level legacy option was removed
  - the external dependency fetch block was removed
  - the legacy helper script was removed
  - the `external/quiche` submodule declaration was removed
- The leftover `external/quiche/` and top-level `quiche/` source directories were then removed from the working tree so native validation no longer carries stale legacy checkout content.
- A subsequent native Windows rerun exposed another host-specific failure mode: MSBuild tracking output for deep project paths under `_local_build_pipeline/tmp/...` hit directory-not-found errors (`MSB6003`, `MSB3491`) while generating `.tlog` state for long target names.
- The native Windows runner was then hardened again to default its isolated build root to a short temp path under `%TEMP%\coolbox-lbp\...`, with an override available through `COOLBOX_NATIVE_TMP_ROOT` when needed.
- That short-path rerun advanced past the previously failing tracked-output targets, which verified that the next Windows-native work is no longer about path-depth failures from the isolated build root.
- Result:
  - the Windows-native path is materially closer to usable
  - the next Windows-native follow-up should focus on whatever remaining full-build or test blockers still exist after the `quiche` removal and path-depth fixes, rather than runner bootstrap, legacy QUIC dependency fetching, or deep-path MSBuild tracking failures

## 3. Add Native macOS Job Scripts
- Add native macOS equivalents for the same highest-value jobs:
  - `build-libs`
  - `build-tests`
  - `build-products`
  - focused LSP validation jobs
- Proposed file additions under `_local_build_pipeline/scripts/jobs/`:
  - `build-libs.macos.sh`
  - `build-tests.macos.sh`
  - `build-products.macos.sh`
  - `build-lsp-target.macos.sh`
- Reuse the brew dependency setup already represented in `.github/workflows/build-libs.yaml`.
- Keep the output structure aligned with `_local_build_pipeline/out/<job-name>/` for parity with Linux and Windows.

## 4. Normalize Job Inputs And Outputs Across Backends
- Define a small backend-independent contract for local jobs:
  - input repository root
  - output folder under `_local_build_pipeline/out/<job-name>/`
  - `command.log`
  - exit code file
  - optional staged artifact directory
- Ensure Linux Docker, Windows native, and macOS native jobs all emit the same observable artifacts.
- This allows plan notes and validation wrappers to stay backend-agnostic.

## 5. Separate Platform Setup From Job Logic
- Factor common job intent from platform setup.
- Proposed split:
  - platform setup scripts per backend
  - job scripts that call platform setup then execute shared configure/build/test steps
- Suggested helper additions:
  - `_local_build_pipeline/scripts/common_windows.ps1`
  - `_local_build_pipeline/scripts/common_macos.sh`
- Keep `common.sh` for Linux Docker shared behavior.

## 6. Keep GitHub Workflow Logic Aligned With Local Backends
- Do not create separate workflow intent for local-only jobs.
- Instead, mirror the same logical stages already present in `.github/workflows/build-libs.yaml`:
  - dependency install
  - configure
  - build
  - test
  - artifact staging
- Use the local backend scripts to approximate those stages per platform without requiring exact command identity.

## 7. Focus LSP Validation On Native Platform Feasibility
- Keep Linux Docker LSP validation as-is.
- Add native Windows and macOS focused LSP validation wrappers rather than trying to build native Windows/macOS Docker-image workflows locally.
- For local LSP validation, the important contract is:
  - configure the target
  - build the target and focused test target
  - run focused tests
  - stage binary under `apps/lsp/dist`
- Docker image build steps remain Linux-appropriate packaging tasks, not required for every local platform backend.

## 8. Update Documentation
- Update `_local_build_pipeline/README.md` to include a backend support matrix:
  - Linux Docker-supported jobs
  - Windows native-supported jobs
  - macOS native-supported jobs
  - workflows that still require GitHub-hosted services or artifact choreography
- Document example commands per platform.

## Suggested Delivery Order
1. Add backend selector support to the local runner dispatcher.
2. Add native Windows support for `build-libs` and focused LSP validation.
3. Add native macOS support for `build-libs` and focused LSP validation.
4. Normalize shared output artifacts across all backends.
5. Add native `build-tests` and `build-products` support.
6. Update the local pipeline README and support matrix.

## Non-Goals
- Do not attempt native macOS Docker container support.
- Do not attempt to force Windows and macOS local runs into the Linux container abstraction.
- Do not try to fully reproduce `workflow_run`, artifact download chains, registry publish steps, or release upload flows locally on every platform.

## Success Criteria
- A contributor can run a Linux clean-room build with Docker.
- A contributor on Windows can run a native local build/test path without relying on Linux containers.
- A contributor on macOS can run a native local build/test path without relying on Docker.
- Focused LSP validation can be executed on all three host platforms through backend-appropriate scripts.
- The output layout remains consistent enough that validation logs and staged artifacts are easy to compare across platforms.