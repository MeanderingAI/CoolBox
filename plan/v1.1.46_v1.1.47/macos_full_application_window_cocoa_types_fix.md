# macOS Full Application Window Cocoa Types Fix

## Summary
- Fixed the macOS Cocoa-runtime build failure in `_libraries/packages/GRAPHICS/full_application_window/source/full_application_window.cpp`.
- Replaced direct use of Objective-C scalar aliases that were not available in the C++ translation unit with ABI-compatible plain C++ types.

## Problem
- The macOS build failed while compiling `full_application_window.cpp` with:
  - `unknown type name 'NSInteger'`
  - `unknown type name 'NSUInteger'`
- The Cocoa runtime path uses `objc_msgSend` directly from C++, but the file was not importing an Objective-C header set that guaranteed those aliases in that compilation mode.

## Change
- Introduced local aliases in the Cocoa code path:
  - `CocoaInteger = long`
  - `CocoaUnsignedInteger = unsigned long`
- Updated the `setActivationPolicy:` message-send cast to use `CocoaInteger`.
- Updated the `styleMask` variable to use `CocoaUnsignedInteger`.

## Reasoning
- The failing code only needs ABI-compatible signed and unsigned scalar types for the Objective-C runtime calls.
- Using plain C++ scalar types avoids depending on `NSInteger` and `NSUInteger` being visible in this C++ translation unit while preserving the expected Cocoa call signatures on macOS.

## Validation
- Editor diagnostics reported no errors in the updated source file after the change.
- Full macOS rebuild still needs to be re-run on a macOS-capable runner to confirm the repository progresses past the earlier compile stop.

## Files
- `_libraries/packages/GRAPHICS/full_application_window/source/full_application_window.cpp`