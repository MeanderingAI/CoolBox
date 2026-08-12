# C Extension CI Path Remodel (v1.5.0)

## Issue

`generate-purchase-c` failed because workflows referenced `_libraries/c_bindings`, which does not exist in the current repository layout.

## Root cause

Legacy hardcoded paths were used for:

- CTest working directory
- docs generation path
- staged headers/libs copy path

## Fix

Updated `.github/workflows/generate-purchase-c.yaml` to use:

- `_deliverables/libraries/bindings/c_bindings/build`
- `_deliverables/libraries/bindings/c_bindings/target`
- `_deliverables/libraries/bindings/c_bindings/include`

## Impact

Prevents `ctest --test-dir` failures caused by non-existent `_libraries/c_bindings/build` and aligns C extension packaging with the active tree.
