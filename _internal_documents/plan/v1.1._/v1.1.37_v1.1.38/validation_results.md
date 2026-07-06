# Validation Results (v1.1.37 -> v1.1.38)

## Static Validation
- Validation was re-run after the purchase pipeline job rename and reusable workflow file rename.
- `.github/workflows/build-purchase-pipeline.yaml` and the new `generate-purchase-*.yaml` workflow files reported no diagnostics after the update.

## Static Validation
- Validation was re-run after the reusable docs publish permission fix.
- `.github/workflows/build-purchase-pipeline.yaml` reported no diagnostics after adding the `docs_publish` job permissions.

## Static Validation
- Validation was re-run after the Go bindings bridge cleanup.
- `_libraries/go_bindings/README.build.md` and `_libraries/go_bindings/bindings.go` reported no diagnostics after the change.
- A follow-up scan confirmed no `.cpp` files remained in the `_libraries/go_bindings` package root.

## Static Validation
- Validation was re-run after the R bindings archive filename fix.
- `.github/workflows/generate-purchase-r.yaml` reported no diagnostics after the filename correction.

## Static Validation
- Validation was re-run after the LSP artifact consumer updates.
- `.github/workflows/build-purchase-pipeline.yaml`, `.github/workflows/lsp-plang.yaml`, `.github/workflows/lsp-java.yaml`, `.github/workflows/lsp-python.yaml`, `.github/workflows/lsp-rust.yaml`, and `.github/workflows/lsp-docker.yaml` reported no diagnostics after the updates.
- A follow-up workflow search confirmed there were no remaining artifact download steps using the stale `plang_lsp`, `plrust_lsp`, `pljava_lsp`, or `plpython_lsp` names.

## Remaining Note
- No live GitHub Actions run was executed from this environment after these updates.
- Final runtime validation still depends on re-running the affected purchase, docs publish, R bindings, Go bindings, and LSP publishing jobs in GitHub Actions.
