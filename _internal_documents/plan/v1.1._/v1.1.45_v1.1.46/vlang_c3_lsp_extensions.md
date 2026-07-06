# VLang And C3 LSP Extensions

## Summary
- Added new V and C3 LSP libraries under `_libraries/packages/TOOLS/`.
- Added `plvlang_lsp` and `plc3_lsp` front-end binaries under `apps/lsp`.
- Added Dockerfiles and dedicated `workflow_run` publish workflows for the new extension images.
- Extended the local build pipeline with focused runners that build, test, and package the V and C3 extensions locally.
- Kept the LSP deliverable separate from the language-binding surface work tracked in `binding_surface_alignment.md`.

## Implementation Details
- `_libraries/CMakeLists.txt` now includes `packages/LSP/lsp_vlang` and `packages/LSP/lsp_c3`.
- `apps/lsp/CMakeLists.txt` now builds and installs `plvlang_lsp` and `plc3_lsp` alongside the existing language servers.
- `.github/workflows/build-libs.yaml` now stages the new binaries into `apps/lsp/dist` and uploads them with the existing LSP artifact family.
- `.github/workflows/lsp-vlang.yaml` and `.github/workflows/lsp-c3.yaml` publish dedicated container images for the new extension binaries.
- `.github/workflows/lsp-docker.yaml` now includes both new images in the aggregate publish job.

## Testing Strategy
- Added focused executable tests for both new LSP libraries:
  - `plvlang_lsp_test`
  - `plc3_lsp_test`
- Added local pipeline jobs that:
  - configure a clean build
  - build the new LSP binary and test target
  - run the focused `ctest` selection
  - stage the built binary for downstream Docker packaging
- Added `_local_build_pipeline/scripts/validate_lsp_target.ps1` as a Windows-friendly wrapper around the Docker-backed local validation path so targeted LSP runs can leave host-visible logs and artifacts even when inline terminal capture is unreliable.

## Notes
- The initial V and C3 diagnostics are lightweight structural checks rather than full parsers. This keeps the extension pipeline functional immediately while preserving a clear future upgrade path to parser-backed diagnostics.
- The broader local `build-libs` pipeline still contains unrelated repository-wide failures outside the new V and C3 extension scope.
