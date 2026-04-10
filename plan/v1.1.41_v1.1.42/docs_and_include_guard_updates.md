# Docs And Include Guard Updates

## Summary
- Moved the public language guides into `docs/core-lib/languages/` and relocated maintainer-facing docs into `docs/internal_documents/`.
- Updated repository and plan references to point at the new docs layout.
- Normalized the binding package labels in the docs to Rust-style namespace aliases such as `coolbox::python` and `coolbox::java` while preserving the real package-manager identifiers.
- Replaced `#pragma once` with `#ifndef` include guards for the currently patched header set across Go bindings, Rust bindings, Python bindings, and selected `_libraries/backages` and `_Product` headers.
- Added `_scripts/tmp_replace_pragma_once.ps1` as a repository helper for include-guard conversion attempts.

## Docs Changes
- Added language guide pages under `docs/core-lib/languages/` for C, Go, Java, JavaScript, Python, R, Rust, MATLAB, and VHDL, along with a folder index.
- Moved the maintainer-facing extension docs from the old root-level `internal_documents/` layout into `docs/internal_documents/`.
- Updated the binding guides and internal extension READMEs so their package labels follow a Rust-style namespace convention in the documentation.
- Updated `README.md` and the plan notes that referenced the old docs paths.

## Documentation Portal Follow-Up
- Added an internal narrative documentation layer so the portal is no longer limited to raw Doxygen output for library discovery.
- Added the catalog generator that now lives at `_scripts/build_product_catalog.py` to generate a solutions catalog, category pages, per-library summaries, and a dependency map from repository structure, README content, metadata macros, product definitions, and CMake link relationships.
- Extended the documentation hub so it links to `Solutions Catalog`, `Dependency Map`, and the generated product pages alongside the existing API-oriented docs.
- The dependency map now shows which libraries are consumed by products and apps, which closes a gap that was not visible in the C++ API docs alone.
- The portal direction for upcoming revisions is to keep pairing API docs with narrative descriptions, build/run hints, architecture notes, and consumer maps so products and reusable libraries can be understood from the same entry point.

## Include Guard Changes
- Converted the currently patched headers from `#pragma once` to path-derived include guards using the `COOLBOX_...` macro naming pattern.
- Cleaned duplicate `#pragma once` directives in headers such as `_libraries/backages/CHEMISTRY/include/chemistry/periodic_table.h` and `_libraries/backages/GRAPHICS/charts/headers/graphics.h` while adding a single guard pair.
- The patched header set includes:
  - Go binding ABI and bridge headers
  - Rust binding bridge header
  - Python binding distributed and graphics-related headers plus vendored chart and wave-generator headers
  - Selected `_libraries/backages` headers under `TOOLS`, `IO`, `GRAPHICS`, `MISC`, `ELECTRONICS`, and `CHEMISTRY`
  - `_Product/MStudio/src/gui_gtk_like.hpp`
  - `_Product/MStudio/src/text_box_editor.hpp`

## Supporting Files
- `_scripts/tmp_replace_pragma_once.ps1`
- `README.md`
- `docs/core-lib/languages/README.md`
- `docs/internal_documents/README.md`
- `docs/internal_documents/extensions_overview.md`

## Result
- The documentation tree is consolidated under `docs/`.
- Binding documentation now uses a consistent Rust-style namespace convention without changing the actual published package names.
- The currently modified headers now use conventional include guards instead of `#pragma once`.
- The version folder `plan/v1.1.41_v1.1.42/` now records this docs, dependency-map, and header-guard work explicitly.