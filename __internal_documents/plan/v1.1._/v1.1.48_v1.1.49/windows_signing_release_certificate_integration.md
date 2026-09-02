## Summary

Promote Windows signing from a CI-only self-signed mechanism to the chosen release path: a repository secret-backed exportable PFX, while preserving a self-signed fallback for non-release validation runs.

## Why

- Self-signed certificates are useful for validating the signing flow, but they do not establish trusted publisher identity for downstream users.
- GitHub Actions cannot safely reference a local certificate path from the repository, so release signing needs a secret-backed certificate transport.
- The build workflows already know how to sign once `COOLBOX_WINDOWS_SIGN_CERT_PATH` and related variables are present; the missing piece is secure CI provisioning.

## Changes

- Update Windows workflow signing preparation steps to prefer `COOLBOX_WINDOWS_SIGN_CERT_BASE64` and `COOLBOX_WINDOWS_SIGN_CERT_PASSWORD` GitHub secrets.
- Materialize the PFX into the runner temp directory and export the existing signing environment variables consumed by `_scripts/sign_windows_artifacts.ps1`.
- Preserve a self-signed fallback path when the release certificate secrets are not configured.
- Keep signtool acquisition tied to Windows SDK discovery with an install fallback.

## Expected Operator Setup

- Add a base64-encoded PFX to the repository secret `COOLBOX_WINDOWS_SIGN_CERT_BASE64`.
- Add the PFX password to the repository secret `COOLBOX_WINDOWS_SIGN_CERT_PASSWORD`.
- Optionally override the timestamp service with `COOLBOX_WINDOWS_SIGN_TIMESTAMP_URL`.

## Release Certificate Decision

- The selected production signing model is an exportable CA-issued PFX stored in GitHub Actions repository secrets.
- Managed signing services and hardware-token-only delivery are out of scope for the current workflow.

## Validation

- Workflow YAML validates in-editor after the secret-backed signing changes.
- Existing Makefile-driven signing behavior remains the final signing entry point.
- When no secrets are present, the workflows still generate a transient self-signed certificate so non-release CI continues to exercise the signing path.