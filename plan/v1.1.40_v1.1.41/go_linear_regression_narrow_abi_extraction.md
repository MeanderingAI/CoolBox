# Go Linear Regression Narrow ABI Extraction

## Summary
- Applied the first code-level narrow C ABI extraction for the Go bindings by moving the linear regression slice into dedicated ABI and Go wrapper files.
- Added shared cgo helper functions so pointer and error handling are no longer implicit inside the monolithic bindings file.

## Files Added
- `_libraries/go_bindings/abi/common.h`
- `_libraries/go_bindings/abi/linear_regression.h`
- `_libraries/go_bindings/cgo_helpers.go`

## Files Updated
- `_libraries/go_bindings/bridge.h`
- `_libraries/go_bindings/bindings.go`
- `_libraries/go_bindings/linear_regression.go`
- `docs/internal_documents/extensions/go/README.md`
- `docs/internal_documents/extensions/go/narrow_c_abi_migration.md`

## Details
- The linear regression declarations were removed from the umbrella `bridge.h` body and moved into a dedicated ABI header that `bridge.h` now includes.
- The Go linear regression wrapper implementation was moved out of the monolithic `bindings.go` file into `linear_regression.go` with its own narrow cgo include set.
- Shared cgo helper functions were added in `cgo_helpers.go` for:
  - C error conversion
  - float slice pointer conversion
  - `C.int` slice pointer conversion
  - converting `[]C.int` back to Go `[]int`
- This is an extraction and narrowing step only; it does not yet replace the underlying native implementation or remove the existing native bridge build dependency.

## Result
- The first feature slice now follows the documented narrow C ABI direction in code rather than existing only as a design note.
- Future slices such as decision tree and HMM can follow the same extraction pattern with less risk and less duplication.