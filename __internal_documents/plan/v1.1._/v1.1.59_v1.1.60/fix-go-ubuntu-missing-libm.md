# Fix Go bindings Ubuntu build: missing libm linkage

## Problem
- Go bindings build fails on Ubuntu with linker error:
  - `/usr/bin/ld: ... undefined reference to symbol 'exp@@GLIBC_2.29'`
  - `/usr/bin/ld: /lib/x86_64-linux-gnu/libm.so.6: error adding symbols: DSO missing from command line`
  - `collect2: error: ld returned 1 exit status`
- This is caused by missing linkage to the math library (`-lm`).

## Root Cause
- The Go bindings' C/C++ bridge (libcoolboxbridge.a) uses math functions (e.g., `exp`), but the build does not explicitly link against `libm` (`-lm`).
- On some platforms (notably Ubuntu), the linker does not automatically add `-lm` for static libraries, causing unresolved symbol errors.

## Solution
- Update the Go bindings build system to explicitly link against `-lm` when building/linking the C/C++ bridge:
  - For CMake, add `m` to the target_link_libraries for the bridge target:
    ```cmake
    target_link_libraries(coolboxbridge ... m)
    ```
  - For manual gcc/g++/ld commands, add `-lm` at the end of the link line.
- Rebuild the bridge and Go bindings.

## Status
- Fixed in v1.1.59_v1.1.60 (pending application if not already present).
- This resolves the `undefined reference to symbol 'exp@@GLIBC_2.29'` error on Ubuntu.

## References
- See also: [Stack Overflow: "undefined reference to symbol 'exp@@GLIBC_2.29'"](https://stackoverflow.com/questions/34767316/undefined-reference-to-symbol-expglibc-2-29)
- Related fixes: ensure all math functions used in C/C++ code are linked with `-lm`.
