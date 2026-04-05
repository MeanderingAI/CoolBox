# Go Bindings Missing Wrapper Types Fix

## Summary
- Restored the exported Go wrapper constants and model/result types required by the Go bindings package.
- Fixed `generate_purchase_go` failures caused by methods referencing public types that were no longer declared in the package.

## Problem
- `_libraries/go_bindings/bindings.go` still contained methods that used `LinearRegression`, `HMM`, `BanditArm`, `BanditAgent`, and `SimulationResult`.
- The package no longer declared those exported types and related constants such as `FitMethodClosedForm` and `SplitCriterionGini`.
- As a result, the Go compiler failed during `generate_purchase_go`, including on `macos-latest` / `macos-arm64`, with errors such as:
  - `undefined: LinearRegression`
  - `undefined: SimulationResult`
  - `undefined: BanditAgent`
  - `undefined: BanditArm`
  - `undefined: HMM`
- The failing macOS CI log specifically showed the missing wrapper declarations breaking both `_libraries/go_bindings/extra_stubs.go` and `_libraries/go_bindings/bindings.go`, including:
  - `./extra_stubs.go:9:10: undefined: HMM`
  - `./extra_stubs.go:13:10: undefined: BanditArm`
  - `./extra_stubs.go:25:10: undefined: BanditAgent`
  - `./extra_stubs.go:25:37: undefined: SimulationResult`
  - `./bindings.go:377:112: undefined: LinearRegression`

## Files Updated
- `_libraries/go_bindings/bindings.go`

## Result
- The Go bindings package once again declares the wrapper surface expected by its existing methods and tests.
- Exported constants for fit methods, split criteria, and SVM kernels are available again.
- The package is aligned with the current bridge handle types used by the native bindings layer.

