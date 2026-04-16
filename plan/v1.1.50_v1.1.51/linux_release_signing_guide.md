# Linux Release Signing Guide

## Summary
- Added a `to_do` guide for signing Linux release artifacts.
- Documented the recommended GPG-based signing model for Linux archives and checksum manifests.
- Cross-linked the Linux guide from the Windows and macOS signing notes so the platform documentation stays discoverable together.

## Problem
- The repository had no Linux-specific release-signing guide.
- Unlike Windows and macOS, Linux does not have a single universal platform trust mechanism, so maintainers needed explicit guidance on what signing model makes sense for this repository.

## Files Updated
- `to_do/linux-release-signing.md`
- `to_do/windows-trusted-code-signing-certificate.md`
- `to_do/macos-release-signing-and-notarization.md`

## Change
- Added a Linux signing guide that covers:
  - why Linux signing differs from Windows Authenticode and macOS notarization
  - the recommended dedicated GPG release-key approach
  - GitHub Actions secrets for GPG-based CI signing
  - example key creation, export, CI import, and detached-signature commands
  - checksum and artifact publication strategy
  - user verification examples
  - future package-specific notes for `.deb`, `.rpm`, AppImage, and container images
- Added related-guide references from the Windows and macOS signing documents.

## Result
- CoolBox now has a repository-local guide for Linux release signing that matches the Windows and macOS documentation style and clarifies the most practical trust model for current archive-based releases.