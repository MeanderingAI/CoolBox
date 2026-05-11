# macOS Release Signing And Notarization Guide

## Summary
- Added a `to_do` guide for setting up trusted macOS release signing and notarization.
- Documented the recommended Developer ID plus notarization workflow for CoolBox macOS products.
- Identified the existing macOS build workflows where signing and notarization should be integrated later.

## Problem
- The repository had Windows signing documentation, but no equivalent guidance for macOS releases.
- There was no repo-local checklist for Developer ID certificate export, CI keychain setup, notarization credentials, or Gatekeeper validation.

## Files Updated
- `to_do/macos-release-signing-and-notarization.md`
- `to_do/windows-trusted-code-signing-certificate.md`

## Change
- Added a new macOS guide that covers:
  - `Developer ID Application` and optional `Developer ID Installer` certificates
  - App Store Connect API key setup for `xcrun notarytool`
  - suggested GitHub Actions secrets
  - temporary keychain setup in CI
  - `codesign`, `notarytool`, `stapler`, and `spctl` validation steps
  - recommended integration points in existing macOS release workflows
- Added a related-guide reference from the Windows signing note to the new macOS document.

## Result
- CoolBox now has an explicit repository guide for how to add trusted macOS release signing and notarization without inventing a separate process from scratch.