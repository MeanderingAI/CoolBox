# Fix Go bindings Windows build: link C++ standard library

## Problem
- Go bindings build fails on Windows/MinGW with linker errors:
  - `undefined reference to std::__cxx11::basic_string<...>`
  - `undefined reference to operator new(unsigned long long)`
  - `undefined reference to __gxx_personality_seh0`
- These errors indicate that the C++ standard library is not being linked, so C++ symbols are missing.

## Root Cause
- The C++ bridge library (libcoolboxbridge.a) is not being linked with the C++ standard library (`stdc++`) and GCC support library (`gcc`) on Windows/MinGW.
- These libraries are required for C++11/14/17 features and exception handling.

## Solution
- Update the Go bindings CMake configuration to explicitly link both `stdc++` and `gcc` for the bridge target on Windows/MinGW:

```cmake
if(MINGW OR WIN32)
  # Link both stdc++ and gcc for full C++11/14/17 support on MinGW/Windows
  target_link_libraries(coolboxbridge PUBLIC stdc++ gcc)
endif()
```
- This ensures all C++ symbols are resolved during linking.
- Rebuild the bridge and Go bindings.

## Status
- Fixed in v1.1.59_v1.1.60.
- This resolves the C++ symbol linker errors on Windows/MinGW for Go bindings.

## References
- See also: [Stack Overflow: "undefined reference to `std::__cxx11::basic_string`"](https://stackoverflow.com/questions/32088140/undefined-reference-to-stdcxx11basic-string)
- Related fixes: always link the C++ standard library and GCC support library when using C++ code with Go on Windows/MinGW.
