# LSP Artifact Name Alignment

## Summary
- Updated LSP publishing workflows to download the current artifact names produced by `build-libs`.
- Replaced stale per-binary artifact names like `plang_lsp` with the current `pl-lsp-binaries-*` artifact pattern.
- Updated the release pipeline LSP packaging job to consume the combined Linux LSP binaries artifact and re-stage per-language packages locally.

## Problem
- `build-libs` now uploads `pl-lsp-dist-${platform}-${ref}` and `pl-lsp-binaries-${platform}-${ref}` artifacts.
- Several LSP workflows still tried to download old artifact names such as `plang_lsp`, `plrust_lsp`, `pljava_lsp`, and `plpython_lsp`.
- GitHub Actions failed with artifact-not-found errors when those stale names were requested.

## Files Updated
- `.github/workflows/build-purchase-pipeline.yaml`
- `.github/workflows/lsp-plang.yaml`
- `.github/workflows/lsp-java.yaml`
- `.github/workflows/lsp-python.yaml`
- `.github/workflows/lsp-rust.yaml`
- `.github/workflows/lsp-docker.yaml`

## Result
- The standalone LSP image workflows now download the combined Linux LSP binaries artifact from the triggering workflow run.
- The main release pipeline also consumes the current combined artifact instead of expecting obsolete single-binary uploads.
