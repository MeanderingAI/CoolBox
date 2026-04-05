# Go Decision Tree Narrow ABI Extraction

## Summary

The Go decision tree bindings were extracted from the monolithic `bindings.go` file into a dedicated module file and ABI header. This is the second implementation step in the narrow C ABI migration after the linear regression extraction.

## Changes

- added `_libraries/go_bindings/abi/decision_tree.h` for the decision tree ABI surface
- added `_libraries/go_bindings/decision_tree.go` for the Go decision tree wrapper implementation
- updated `_libraries/go_bindings/bridge.h` to include the new decision tree ABI header
- removed the decision tree wrapper implementation from `_libraries/go_bindings/bindings.go`
- added shared `toCIntSlice` conversion support in `_libraries/go_bindings/cgo_helpers.go`

## Rationale

This keeps the exported Go API stable while reducing the amount of unrelated cgo wrapper code carried in the umbrella bindings file. It also establishes the same extraction pattern used by linear regression so later modules can follow a consistent structure.

## Validation

- editor diagnostics reported no issues for the touched Go and header files
- runtime validation remains limited in this environment because terminal output is currently unreliable

## Next Steps

- extract the next model module using the same pattern, likely HMM or PCA
- begin narrowing the native implementation side into dedicated source files after a few wrapper-level extractions are complete
