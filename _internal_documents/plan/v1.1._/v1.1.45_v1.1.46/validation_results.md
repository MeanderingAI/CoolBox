# Validation Results (v1.1.45 -> v1.1.46)

## Static Validation
- Reviewed the new V and C3 LSP library targets to confirm they are included from `_libraries/CMakeLists.txt` and exposed through `apps/lsp/CMakeLists.txt`.
- Reviewed the LSP packaging step in `.github/workflows/build-libs.yaml` to confirm the new binaries are staged into `apps/lsp/dist` and uploaded with the existing LSP artifact family.
- Reviewed `.github/workflows/lsp-vlang.yaml`, `.github/workflows/lsp-c3.yaml`, and `.github/workflows/lsp-docker.yaml` to confirm the new extension images follow the same publish pattern as the existing language LSP workflows.

## Editor Diagnostics
- Editor diagnostics reported no errors in the newly added V and C3 LSP source, header, CMake, workflow, and local pipeline files at edit time.

## Local Execution
- Re-ran the local Docker-backed `build-libs` pipeline after the earlier header and linker fixes; the global build still exits non-zero because of unrelated repo-wide blockers outside the new V/C3 extension targets.
- Added and ran a focused local `build-battery-tests` job to confirm the earlier battery include-guard fix succeeds independently.
- Added `_local_build_pipeline/scripts/validate_lsp_target.ps1` to make targeted LSP validation reproducible on the Windows host without depending on fragile inline PowerShell quoting.
- Confirmed VLang targeted validation with the earlier manual Docker-backed run under `_local_build_pipeline/out/build-lsp-vlang-manual/`, which produced `plvlang_lsp` and `EXIT=0`.
- Confirmed C3 targeted validation with the wrapper-driven Docker-backed run under `_local_build_pipeline/out/build-lsp-c3-final/`, which produced `plc3_lsp`, recorded `EXIT=0`, and captured a complete `command.log` showing `plc3_lsp_test` passed.

## Result
- The repository now contains first-class V and C3 language-extension targets aligned with the existing LSP packaging model, plus focused local validation paths that were exercised successfully for both extensions independently of unrelated full-build failures.
- The shared binding-surface work is tracked separately in `binding_surface_alignment.md` so the LSP deliverable remains documented as an independent scope.