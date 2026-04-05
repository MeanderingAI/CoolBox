# CoolBox Extension Overview

This document indexes the extension and binding packages that are maintained in this repository and packaged by the release workflows.

## Per-Extension Internal READMEs

- Python: `internal_documents/extensions/python/README.md`
- Go: `internal_documents/extensions/go/README.md`
- C: `internal_documents/extensions/c/README.md`
- Java: `internal_documents/extensions/java/README.md`
- JavaScript / Emscripten: `internal_documents/extensions/javascript/README.md`
- Rust: `internal_documents/extensions/rust/README.md`
- R: `internal_documents/extensions/r/README.md`

Each extension README contains:

- repository location
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

These names matter because the docs publishing workflow restores artifacts by those exact names before generating the unified site.