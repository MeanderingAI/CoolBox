# Go Bayesian Network And GUI Narrow ABI Extraction

## Summary

The remaining broad Go wrappers were extracted into dedicated module files and ABI headers. This completes the wrapper-level narrow C ABI migration for the current Go bindings surface.

## Changes

- added `_libraries/go_bindings/abi/bayesian_network.h`
- added `_libraries/go_bindings/bayesian_network.go`
- added `_libraries/go_bindings/abi/gui.h`
- added `_libraries/go_bindings/gui.go`
- updated `_libraries/go_bindings/bridge.h` to include the new Bayesian network and GUI ABI headers
- removed inline GUI declarations from `_libraries/go_bindings/bridge.h`
- removed the remaining Bayesian network and GUI wrapper implementations from `_libraries/go_bindings/bindings.go`

## Result

After this extraction, `_libraries/go_bindings/bindings.go` now primarily contains:

- shared cgo preamble and imports for shared helpers and type definitions
- shared matrix and slice helpers
- shared constants
- shared handle-bearing Go type definitions used by the extracted modules

## Notes

A few GUI constructors were given input guards during extraction to avoid invalid empty-slice pointer use in the cgo calls.

## Validation

- editor diagnostics reported no issues for the full Go bindings directory
- runtime validation remains limited in this environment because terminal output is still unreliable

## Next Steps

- begin matching native-side splits under `_libraries/go_bindings/native/` and related bridge implementation files
- optionally add focused tests per extracted module once runtime validation tooling is reliable in this environment
