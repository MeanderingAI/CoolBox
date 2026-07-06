# GitHub Actions Artifact Quota Cleanup Addendum (v1.3.5)

## Summary
This addendum captures the CI reliability changes made to address GitHub artifact upload failures caused by storage quota exhaustion and to harden Windows dependency setup fragment parsing.

## Changes Added
- Fixed composite action YAML structure in `.github/workflows/fragments/deps-windows/action.yaml`.
- Corrected step indentation and key placement for `if`, `shell`, `run`, `uses`, and `with`.
- Resolved manifest parsing failures reported by GitHub Actions for the Windows dependency fragment.
- Added operational cleanup script at `_script/gh_scripts/delete_non_success_artifacts.ps1`.
- Executed artifact cleanup script against repository `MeanderingAI/CoolBox`.
- Added new binding packages:
	- `_deliverables/libraries/bindings/swift_bindings`
	- `_deliverables/libraries/bindings/postgres_bindings`
- Updated extension builder backend/frontend wiring to detect and build `swift_bindings` and `postgres_bindings`.
- Added internal extension docs for Swift and Postgres under `_internal_documents/code_documentation/docs/internal_documents/extensions/`.

## Implementation Notes
- The cleanup script discovers the repository owner/name from `remote.origin.url`.
- The script uses `gh api` to enumerate completed workflow runs and filters to non-success conclusions.
- For each non-success run, associated artifacts are deleted via Actions artifact delete API.
- Script supports `-DryRun` mode for verification without deletion.

## Operational Outcome
- Bulk artifact deletions were initiated and performed for artifacts attached to failed/cancelled runs.
- This directly mitigates `Failed to CreateArtifact: Artifact storage quota has been hit` incidents.
- GitHub storage usage recalculation still follows platform timing (typically 6-12 hours).

## Cleanup Run Snapshot
- Completed runs scanned: 267
- Non-success runs scanned: 202
- Artifacts matched: 1763
- Deleted: 1762
- Failed deletions: 1

Notes:
- The single failed deletion is typically caused by a race condition (already deleted between list/delete) or transient API refusal.
- A follow-up run can retry the remaining artifact after quota metrics refresh.

## Follow-Up
- Add retention controls (`retention-days`) to artifact upload steps where appropriate.
- Consider scheduling this cleanup script as a periodic maintenance workflow.
