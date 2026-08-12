# v1.4.7 Windows FullApplicationWindow Scope Fix

## Issue

Windows CI failed while compiling `full_application_window.cpp` with a cascade of parser errors, including:

- `expected constructor, destructor, or type conversion before '(' token`
- `expected unqualified-id before 'if'`
- `'impl_' was not declared in this scope`
- non-member function errors on methods like `fill_rect`, `draw_arc`, `present_canvas`

These are typical downstream errors from one malformed method body earlier in the file.

## Root cause

In `FullApplicationWindow::clear_background(...)`, the Win32 guard body was malformed. The early return existed without its surrounding condition:

- missing `if (!impl_->hwnd || !IsWindow(impl_->hwnd)) { ... }`

This corrupted function scope and caused later method definitions to be parsed incorrectly.

## Fix

Restored the Win32 validity guard in:

- `_deliverables/libraries/groups/app_builder/OS_GENERICS/full_application_window/source/full_application_window.cpp`

Patched block:

```cpp
#if defined(_WIN32)
    if (!impl_->hwnd || !IsWindow(impl_->hwnd)) {
        return;
    }
```

## Validation

Built target successfully after the fix:

- `cmake --build build --target full_application_window -j4`
- `BUILD_EXIT_CODE:0`

## Impact

- Fixes Windows parser/build failure in `full_application_window.cpp`.
- No functional behavior change beyond restoring intended Win32 safety guard.
