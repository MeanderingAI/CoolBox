# Windows Go CBridge uint8 Header

## Summary
- Fixed the Windows Go cbridge build by adding the missing fixed-width integer type include to the public bridge header.
- Removed the implicit dependency on downstream translation units to provide `uint8_t` before including `bridge.h`.
- Kept the fix scoped to the header contract rather than adding compiler-specific workarounds.

## Problem
- The Windows Go purchase pipeline builds `_libraries/go_bindings/cbridge` after the C bindings step succeeds.
- `_libraries/go_bindings/cbridge/bridge.h` declares `coolbox_color_create(uint8_t r, uint8_t g, uint8_t b, uint8_t a)`.
- The header did not include any standard integer type header, so MinGW C++ compilation failed with `uint8_t was not declared in this scope`.
- `bridge.cpp` included `<cstdint>`, but that does not repair the public header contract for any translation unit that includes `bridge.h` first.

## Files Updated
- `_libraries/go_bindings/cbridge/bridge.h`

## Change
- Added `#include <stdint.h>` at the top of `bridge.h` before the `extern "C"` block.
- Used the C-compatible fixed-width integer header so the public bridge declarations remain valid for both C and C++ consumers.

## Result
- The Windows Go cbridge target can now resolve `uint8_t` directly from its public header during MinGW compilation.
- The build no longer fails on the `coolbox_color_create` declaration solely because fixed-width integer types were undeclared.
- The header is now self-contained with respect to the integer types used in its public API.