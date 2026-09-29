# C3 Extension CI Path Remodel (v1.5.0)

## Issue

`generate-purchase-c3` used legacy `_libraries/*_bindings` paths for both C3 module files and native C dependencies.

## Root cause

The workflow still built/staged from old locations:

- `_libraries/c_bindings`
- `_libraries/c3_bindings`

## Fix

Updated `.github/workflows/generate_purchase/generate-purchase-c3.yaml` to use:

- `_deliverables/libraries/bindings/c_bindings`
- `_deliverables/libraries/bindings/c3_bindings`

## Impact

Ensures C3 packaging jobs build and stage from real repository paths and avoids path-not-found failures in CI.
