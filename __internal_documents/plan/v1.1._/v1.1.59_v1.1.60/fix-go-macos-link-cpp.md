# Fix Go bindings macOS build: missing symbols for C++ and C bridge

## Problem
- Go bindings build fails on macOS (arm64) with linker errors:
  - `Undefined symbols for architecture arm64: ... graphics::Fractal ... _coolbox_* ...`
- The linker cannot find implementations for many C++ and C bridge functions.

## Root Cause
- The C++ bridge library (libcoolboxbridge.a) is not being linked with all required object files or dependencies.
- The C++ symbols (e.g., graphics::Fractal, _coolbox_canvas_create, etc.) are not present in the linked static/dynamic libraries.
- On macOS, you must ensure all C++ source files and dependencies are included and that the correct libraries are linked (including the C++ standard library and any other required libraries).

## Solution
1. **Ensure all required object files are included in libcoolboxbridge.a.**
   - Check that the CMakeLists.txt for the bridge target includes all relevant source files (especially those implementing the missing symbols).
2. **Link all required libraries.**
   - In CMake, for the bridge target, add all dependencies to target_link_libraries, e.g.:
     ```cmake
     target_link_libraries(coolboxbridge PUBLIC c++ charts coolbox_c_bindings ...)
     ```
   - On macOS, use `c++` for the C++ standard library (not `stdc++`).
3. **Order matters:** On macOS, the order of libraries in the link line can matter. Make sure dependent libraries come after the objects that need them.
4. **Rebuild everything cleanly:** Sometimes stale object files or libraries can cause missing symbol errors.

## Status
- Pending in v1.1.59_v1.1.60.
- Apply these CMake changes and ensure all bridge sources and dependencies are included for Go bindings on macOS.

## References
- See also: [Stack Overflow: "Undefined symbols for architecture arm64"](https://stackoverflow.com/questions/12573816/undefined-symbols-for-architecture-arm64)
- Related fixes: always link the C++ standard library and all required bridge sources for Go bindings on macOS.
