# Go HMM And PCA Narrow ABI Extraction

## Summary

The Go HMM and PCA bindings were extracted from the monolithic bindings file into dedicated module files and ABI headers. This continues the narrow C ABI migration after the linear regression and decision tree slices.

## Changes

- added `_libraries/go_bindings/abi/hmm.h`
- added `_libraries/go_bindings/hmm.go`
- added `_libraries/go_bindings/abi/pca.h`
- added `_libraries/go_bindings/pca.go`
- updated `_libraries/go_bindings/bridge.h` to include the new HMM and PCA ABI headers
- removed inline HMM and PCA declarations from `_libraries/go_bindings/bridge.h`
- removed HMM and PCA wrapper implementations from `_libraries/go_bindings/bindings.go`
- corrected `_libraries/go_bindings/decision_tree.go` to use the package name and cgo include layout already established by the earlier extracted modules

## Rationale

HMM and PCA are both model-oriented modules with well-contained wrapper surfaces. Extracting them now reduces the size of the broad bindings file and reinforces a consistent per-module migration pattern before the remaining model, graphics, and GUI slices are tackled.

## Validation

- editor diagnostics reported no issues in the touched Go and header files
- runtime validation remains limited in this environment because terminal output is still unreliable

## Next Steps

- extract SVM or multi-arm bandit next using the same wrapper/header split
- start introducing dedicated native implementation units after a few more module extractions so the C++ side matches the wrapper layout
