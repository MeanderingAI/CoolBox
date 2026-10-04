# V Extension CI Path Remodel (v1.5.0)

## Issue

`generate-purchase-v` referenced missing `_libraries/vlang_bindings` and `_libraries/c_bindings` paths.

## Root cause

Legacy path assumptions remained in build and staging steps.

## Fix

Updated `.github/workflows/generate_purchase/generate-purchase-v.yaml` to use:

- `_deliverables/libraries/bindings/vlang_bindings`
- `_deliverables/libraries/bindings/c_bindings`

## Impact

Prevents CI failures during native dependency build and package staging for V bindings.
