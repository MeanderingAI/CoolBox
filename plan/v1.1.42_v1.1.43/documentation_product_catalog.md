# Documentation Product Catalog Recovery

## Summary
- Restored the missing narrative docs generator by adding `_scripts/build_product_catalog.py` as the catalog-generation entrypoint.
- Updated `_scripts/build_documentation.sh` so the documentation build now calls `build_product_catalog.py` directly.

## Reason For The Change
- CI failed because the documentation build was trying to execute a catalog generator path that was not available on the runner checkout.
- Reintroducing the generator under a fresh, explicit name reduces ambiguity and gives the docs pipeline a new stable path to call.

## Script Behavior
- `build_product_catalog.py` regenerates the `solutions` area of the site, including:
  - narrative library catalog pages
  - category pages
  - reverse dependency mapping
  - the `catalog.json` manifest used by `_scripts/build_product_assets.py`
- The fallback logic in `build_documentation.sh` makes the docs build more resilient while the repository transitions from the old script name.

## Result
- The docs build now has a named generator entrypoint again, and the documentation pipeline is no longer coupled to a single missing script path.