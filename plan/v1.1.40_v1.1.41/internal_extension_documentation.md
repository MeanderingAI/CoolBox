# Internal Extension Documentation

## Summary
- Added a new `internal_documents/` folder at the repository root for maintainer-facing documentation.
- Added an internal extension catalog describing the different language bindings and extension artifact families shipped from this repository.

## Files Added
- `internal_documents/README.md`
- `internal_documents/extensions_overview.md`

## Details
- The new internal documentation summarizes the current extension inventory across:
  - Python
  - Go
  - C
  - Java
  - JavaScript / Emscripten
  - Rust
  - R
- The catalog records each extension's location, exposed surface, primary purpose, and packaging or workflow notes.
- It also captures the artifact naming used by the reusable release workflows so future CI and docs changes can refer to one internal source of truth.

## Result
- Maintainers now have a dedicated internal reference for understanding how each extension is represented in the repo and in the release pipeline.
- The new folder separates internal operational notes from user-facing package documentation.