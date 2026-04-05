# Go Multi-Arm Bandit Narrow ABI Extraction

## Summary

The Go multi-arm bandit bindings were extracted from the monolithic bindings file into a dedicated module file and ABI header. This continues the narrow C ABI migration across the remaining model-focused wrappers.

## Changes

- added `_libraries/go_bindings/abi/multi_arm_bandit.h`
- added `_libraries/go_bindings/multi_arm_bandit.go`
- updated `_libraries/go_bindings/bridge.h` to include the new bandit ABI header
- removed inline bandit arm and bandit agent declarations from `_libraries/go_bindings/bridge.h`
- removed the bandit arm and bandit agent wrapper implementations from `_libraries/go_bindings/bindings.go`

## Rationale

The bandit module still has a manageable C ABI surface even though it includes both arm and agent handles. Extracting it now further reduces the monolithic wrapper file and keeps the per-module migration pattern consistent before the graphics and GUI slices.

## Validation

- editor diagnostics reported no issues in the touched Go and header files
- runtime validation remains limited in this environment because terminal output is still unreliable

## Next Steps

- extract the graphics/charting wrappers next
- follow that by extracting the GUI/component wrappers, or begin matching native-side splits if wrapper-level extraction is sufficient for the current milestone
