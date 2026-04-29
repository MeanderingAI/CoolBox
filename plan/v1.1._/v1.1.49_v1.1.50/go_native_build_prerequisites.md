# Go Native Build Prerequisites

## Summary
- Fixed the Go purchase-generation flow so native libraries are built before `go test` runs.
- Corrected the Go bridge CMake file to resolve repository includes from the actual repository root.
- Hardened Windows `coolbox_c_bindings` production so the MinGW linker can find a predictable import library.

## Problem
- The Go purchase-generation workflow ran `go test` before building `_libraries/c_bindings` and `_libraries/go_bindings/cbridge`.
- Ubuntu and macOS then failed to link because `coolboxbridge` or `coolbox_c_bindings` was not present.
- The Go bridge CMake file also used `CMAKE_SOURCE_DIR` as though it were always the repository root.
- On Windows, the MinGW linker also needed a stable `coolbox_c_bindings` import library output in the build directories that cgo already searches.

## Files Updated
- `.github/workflows/generate-purchase-go.yaml`
- `_libraries/go_bindings/cbridge/CMakeLists.txt`
- `_libraries/c_bindings/CMakeLists.txt`
- `_libraries/c_bindings/include/coolbox/coolbox_c.h`

## Change
- Added explicit native build steps for `_libraries/c_bindings` and `_libraries/go_bindings/cbridge` before `go test` in the GitHub Actions workflow.
- Corrected the Go bridge CMake include roots to derive from the repository root rather than `CMAKE_SOURCE_DIR`.
- Added Windows export/import annotations for the public `coolbox_c_bindings` C API.
- Pinned the `coolbox_c_bindings` archive, library, and runtime output directories so both `build` and `build/Release` contain the artifacts cgo expects.

## Result
- Go purchase-generation now has the native prerequisites available before tests run.
- Ubuntu and macOS no longer fail because the native bridge libraries were never built.
- Windows Go builds now produce `coolbox_c_bindings` artifacts in deterministic locations for the cgo linker.