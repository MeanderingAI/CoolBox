# CoolBox Extension Overview

This document indexes the extension and binding packages that are maintained in this repository and packaged by the release workflows.

## Per-Extension Internal READMEs

- Python: `docs/internal_documents/extensions/python/README.md`
- Go: `docs/internal_documents/extensions/go/README.md`
- C: `docs/internal_documents/extensions/c/README.md`
- Java: `docs/internal_documents/extensions/java/README.md`
- JavaScript / Emscripten: `docs/internal_documents/extensions/javascript/README.md`
- Rust: `docs/internal_documents/extensions/rust/README.md`
- R: `docs/internal_documents/extensions/r/README.md`
- Swift: `docs/internal_documents/extensions/swift/README.md`
- Postgres: `docs/internal_documents/extensions/postgres/README.md`

Each extension README contains:

- repository location
- Rust-style documentation namespace aliases alongside the real package identifiers
- exposed surface and purpose
- installation instructions
- setup and local build instructions
- release page reference
- packaging or workflow notes

## Release Workflow Artifact Names

The reusable release workflows publish these extension artifact families:

- `python-extension-*`
- `go-extension-*`
- `r-extension-*`
- `rust-extension-*`
- `c-extension-*`
- `javascript-extension-*`
- `java-extension-*`
- `swift-extension-*`
- `postgres-extension-*`

These names matter because the docs publishing workflow restores artifacts by those exact names before generating the unified site.