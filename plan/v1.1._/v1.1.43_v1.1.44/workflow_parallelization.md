# Workflow Parallelization

## Summary
- Moved product building and product packaging out of the reusable `Build Libraries & Release` workflow into a separate reusable workflow defined in `.github/workflows/build-products.yaml`.
- Updated `.github/workflows/build-purchase-pipeline.yaml` so `build_products` now runs as a sibling job beside the `generate_purchase_*` jobs after `build_libs` completes.

## Reason For The Change
- The language-extension workflows listen for completion of `Build Libraries & Release` via `workflow_run`.
- Keeping product packaging inside that workflow meant product release packaging delayed downstream extension publishing even though those extension jobs only depend on the library and LSP artifacts.
- Splitting product packaging into a separate branch preserves the product release flow without making it a blocker for the language-extension pipeline.

## Workflow Changes
- `.github/workflows/build-libs.yaml` now ends after the `release` job creates or updates the GitHub release with library assets.
- `.github/workflows/build-products.yaml` now owns the product matrix build, product archive packaging, workflow artifact upload, and release attachment upload for `MStudio`, `file_browser`, and `bower_shell`.
- `.github/workflows/build-purchase-pipeline.yaml` now invokes `build_products` with `needs: build_libs`, allowing it to run in parallel with `generate_purchase_python`, `generate_purchase_js`, `generate_purchase_go`, `generate_purchase_c`, `generate_purchase_java`, `generate_purchase_r`, and `generate_purchase_rust`.

## Result
- Product packaging is still part of the broader release pipeline, but it no longer lengthens the completion time of `Build Libraries & Release`.
- The language-extension workflows can start as soon as the library release workflow completes, while product packaging continues independently in the purchase pipeline.