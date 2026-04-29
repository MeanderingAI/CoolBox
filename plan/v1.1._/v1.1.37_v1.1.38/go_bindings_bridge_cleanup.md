# Go Bindings Bridge Cleanup

## Summary
- Removed a stale top-level `bridge.cpp` file from `_libraries/go_bindings`.
- Clarified in the Go bindings build notes that C++ bridge sources must live under `cbridge/`.

## Problem
- `go test` scans the package directory before build.
- The Go bindings package already uses cgo through `bindings.go`, but the root package directory still contained a standalone `bridge.cpp`.
- Go rejected the package with `C++ source files not allowed when not using cgo or SWIG: bridge.cpp`.

## Files Updated
- `_libraries/go_bindings/README.build.md`
- `_libraries/go_bindings/bridge.cpp` removed

## Result
- The Go package root no longer contains a stray `.cpp` file that interferes with package scanning.
- The C++ bridge remains under `_libraries/go_bindings/cbridge/bridge.cpp`, where the CMake bridge target is built.
