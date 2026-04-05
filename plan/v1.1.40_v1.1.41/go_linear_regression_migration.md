# Go Linear Regression Migration

## Status
- First interface-level extraction already completed.

## Current Files
- `_libraries/go_bindings/abi/linear_regression.h`
- `_libraries/go_bindings/linear_regression.go`
- compatibility include through `_libraries/go_bindings/bridge.h`

## Remaining Work
- Move the native implementation into `_libraries/go_bindings/native/linear_regression.cpp`.
- Switch error handling to the shared status-code model.
- Add runtime verification after extraction from the broad bridge implementation.

## Scope
- fit
- predict
- feature count
- intercept
- weight inspection
- method-name inspection
- free

## Notes
- This is the reference module for the remaining migrations.
- Future module extractions should follow the same ABI, Go wrapper, and native implementation pattern.
