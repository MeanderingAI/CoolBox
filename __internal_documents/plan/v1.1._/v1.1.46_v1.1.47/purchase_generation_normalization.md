# Purchase Generation And Output Normalization

## Goal
- Add the missing V and C3 purchase-generation paths so they are first-class alongside C, Go, Java, JS, Python, R, and Rust.
- Normalize the local purchase-package outputs so every binding package follows the same naming and staged-asset contract.

## Problem
- The repository had no dedicated purchase-generation workflows for V or C3.
- The local pipeline also had no matching `generate-purchase-v` or `generate-purchase-c3` job scripts.
- Existing purchase jobs had drifted output names:
  - some included a platform segment
  - some omitted it
  - helper logic for package names, zip/tar paths, and asset metadata was duplicated across scripts

## Implemented Changes

### 1. Add Missing Workflow Coverage
- Added `.github/workflows/generate-purchase-v.yaml`.
- Added `.github/workflows/generate-purchase-c3.yaml`.
- Both workflows package the binding sources together with the C binding headers and native library artifacts required by the metadata-client surface.

### 2. Add Matching Local Pipeline Jobs
- Added `_local_build_pipeline/scripts/jobs/generate-purchase-v.sh`.
- Added `_local_build_pipeline/scripts/jobs/generate-purchase-c3.sh`.
- Added dispatcher support in `_local_build_pipeline/scripts/run_workflow.sh`.
- Updated `_local_build_pipeline/README.md` so both workflows appear in the supported local-runner matrix.

### 3. Normalize Local Purchase Outputs
- Added shared helpers in `_local_build_pipeline/scripts/jobs/common.sh` for:
  - ref naming
  - platform tagging
  - package stem generation
  - tarball path generation
  - zip path generation
  - shared zip creation
  - release-asset finalization
- Updated the existing local purchase jobs to use the same helper contract.

### 4. Factor Shared CI Asset Finalization
- Added a composite GitHub Action at `.github/actions/finalize-purchase-assets/action.yml`.
- Replaced the repeated workflow-local manifest/checksum shell snippets with one shared finalization step.
- This removes repeated asset-metadata logic from the language-specific workflow files while preserving their package-specific staging steps.

## What "Normalize" Means Here
- Every local purchase job now emits release assets with the same naming shape:
  - `coolbox-<language>-bindings-<platform>-<ref>.tar.gz`
  - `coolbox-<language>-bindings-<platform>-<ref>.zip`
- Every local purchase job now stages those files in the same folder:
  - `release-assets/`
- Every local purchase job now emits the same supporting metadata when available:
  - `manifest.txt`
  - `SHA256SUMS`
- Every local purchase job now relies on the same helper functions instead of hand-rolling package names per language.
- Every GitHub purchase workflow now relies on the same composite action to finalize metadata assets rather than duplicating the same shell block per workflow.

## Richer Manifest Content
- The manifest now records more than ref and platform.
- The normalized manifest contract now includes:
  - schema version
  - repository
  - workflow name
  - run id
  - run attempt
  - ref name
  - commit SHA
  - platform
  - asset count
  - one `asset=` line per packaged artifact
- Local pipeline manifests include the same core fields where GitHub context is available and fall back to local sentinel values when it is not.

## Immediate Repository Impact
- V and C3 are no longer omitted from the purchase-generation workflow set.
- Java and Rust local purchase packages now include the same platform-qualified naming convention already used by the other bindings.
- The local release-asset layout is easier to compare, automate, and document because the output contract is consistent across languages.

## Scope Notes
- This change normalizes the local pipeline output contract first.
- The GitHub Actions purchase workflows were then aligned to the same asset contract, including manifest and checksum emission for every language-specific purchase package.
- The new V and C3 purchase paths package the sources and native dependency bundle even when the V or C3 compiler is not present in the Linux CI image.
- During local verification on Windows, the Docker-local `build-libs` runner produced a clean staged result with `EXIT=0` under `_local_build_pipeline/out/build-libs/`, confirming that the normalized local asset helper changes do not block the base Docker-backed workflow.
- The newly added V and C3 local purchase runners were included in the verification attempt set, but this workspace session did not produce a complete staged result for every Docker-local workflow, so their runtime status remains only partially verified in this plan cycle.

## Success Criteria
- Contributors can run local purchase-generation jobs for V and C3.
- All local `generate-purchase-*` jobs emit predictable package names.
- Release assets produced locally follow a backend-independent naming contract that is easier to consume from validation and release notes.