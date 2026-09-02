# Fix Go Windows Build: Link C++ Standard Library

- Problem: Go Windows build fails with missing C++ symbols (e.g., operator new, std::string, __gxx_personality_seh0) due to missing linkage of the C++ standard library.
- Solution: Added conditional linkage of `stdc++` to the `coolboxbridge` target in `_libraries/go_bindings/cbridge/CMakeLists.txt` for Windows/MinGW builds.
- Impact: Ensures all C++ runtime and standard library symbols are available to Go bindings on Windows, fixing linker errors.
- Location: See `_libraries/go_bindings/cbridge/CMakeLists.txt` for the change.
- Example patch:
  ```cmake
  if(MINGW OR WIN32)
    target_link_libraries(coolboxbridge PUBLIC stdc++)
  endif()
  ```
- This step is required for any target using C++ code with Go/cgo on Windows/MinGW.
