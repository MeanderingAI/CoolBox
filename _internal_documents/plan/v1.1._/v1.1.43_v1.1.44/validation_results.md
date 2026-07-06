# Validation Results (v1.1.43 -> v1.1.44)

## Static Validation
- Reviewed `.github/workflows/build-libs.yaml` to confirm the inlined `build-products` job was removed and the reusable library release workflow now ends after `release`.
- Reviewed `.github/workflows/build-products.yaml` to confirm the extracted workflow preserves the existing product matrix, product target list, packaging behavior, and release-attachment logic.
- Reviewed `.github/workflows/build-purchase-pipeline.yaml` to confirm `build_products` now fans out from `build_libs` alongside the `generate_purchase_*` jobs instead of sitting inside the reusable library-release workflow.
- Reviewed the `workflow_run`-based language-extension workflows to confirm they still target `Build Libraries & Release`, which now finishes earlier because product packaging is no longer inside that workflow.

## Editor Diagnostics
- Editor diagnostics reported no errors in `.github/workflows/build-libs.yaml`, `.github/workflows/build-products.yaml`, and `.github/workflows/build-purchase-pipeline.yaml` after the workflow split.

## Execution Note
- This revision was validated through workflow inspection, dependency tracing, and editor diagnostics only; no GitHub Actions run was executed from this environment.
- Real-world runtime still depends on GitHub runner availability and toolchain support for each configured product platform in the extracted workflow matrix.

## Result
- Product builds are now scheduled in parallel with purchase-generation work instead of delaying downstream language-extension automation that depends on completion of `Build Libraries & Release`.