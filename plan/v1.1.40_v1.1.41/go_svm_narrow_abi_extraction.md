# Go SVM Narrow ABI Extraction

## Summary

The Go SVM bindings were extracted from the monolithic bindings file into a dedicated module file and ABI header. This continues the narrow C ABI migration after the linear regression, decision tree, HMM, and PCA slices.

## Changes

- added `_libraries/go_bindings/abi/svm.h`
- added `_libraries/go_bindings/svm.go`
- updated `_libraries/go_bindings/bridge.h` to include the new SVM ABI header
- removed inline SVM declarations from `_libraries/go_bindings/bridge.h`
- removed the SVM wrapper implementation from `_libraries/go_bindings/bindings.go`

## Rationale

SVM is still a relatively contained model wrapper even though it carries kernel configuration parameters. Extracting it now keeps the exported Go API stable while continuing to shrink the monolithic bindings surface and standardize the per-module ABI/header split.

## Validation

- editor diagnostics reported no issues in the touched Go and header files
- runtime validation remains limited in this environment because terminal output is still unreliable

## Next Steps

- extract multi-arm bandit next
- begin native implementation splits after the remaining model wrappers are migrated
