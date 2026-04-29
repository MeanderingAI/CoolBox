# Go Bindings CGO Preamble Fix

## Summary
- Fixed the Go bindings cgo preamble so the `#cgo` directives are parsed correctly by the Go toolchain.
- Removed the nested block comment that invalidated the cgo comment block immediately before `import "C"`.

## Problem
- `_libraries/go_bindings/bindings.go` contains cgo compiler and linker directives in the comment block above `import "C"`.
- That block also contained a nested `/* ... */` comment.
- Nested block comments are not valid inside the cgo preamble and caused Go to stop treating the block as cgo metadata.
- As a result, the Go build failed with:
  - `invalid character U+0023 '#'`

## Files Updated
- `_libraries/go_bindings/bindings.go`

## Result
- The cgo preamble is now a single valid comment block.
- The `#cgo` directives for C++ flags, include paths, and linker flags are parsed correctly again.
