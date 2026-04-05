# Go Bindings Bridge Header Self-Contained Fix

## Summary
- Made the Go bridge header self-contained for cgo parsing.
- Added the standard integer and size headers plus the forward declarations include required by the bridge API.

## Problem
- `_libraries/go_bindings/bridge.h` declares functions and opaque handles used by the cgo preamble in `_libraries/go_bindings/bindings.go`.
- That header used `uint8_t`, `uint32_t`, and `size_t` without directly including the standard headers that define them.
- Because cgo parses the header from the Go preamble, the missing includes caused symbol resolution failures such as:
  - `could not determine what C.coolbox_color_create refers to`
  - `unknown type name 'uint8_t'`
- The header also depended on opaque forward declarations that were safer to include directly rather than only via the Go preamble ordering.
- User-provided Windows CI logs showed the same failure mode in `generate_purchase_go`, including:
  - `./bindings.go:28:7: could not determine what C.coolbox_color_create refers to`
  - `./bridge.h:72:36: error: unknown type name 'uint8_t'`
  - repeated `uint8_t` parse failures for the `coolbox_color_create` parameters in the cgo preamble.

## Files Updated
- `_libraries/go_bindings/bridge.h`

## Result
- `bridge.h` now includes `<stdint.h>`, `<stddef.h>`, and `bridge_forward.h` directly.
- The header can be parsed independently by cgo and other C/C++ consumers without relying on surrounding include order.
- The `coolbox_color_create` declaration and related bridge symbols are now visible through a self-contained header interface.
- The same header-level fix addresses both the Windows and macOS cgo parsing failures because both runners parse the identical Go preamble and bridge header.
