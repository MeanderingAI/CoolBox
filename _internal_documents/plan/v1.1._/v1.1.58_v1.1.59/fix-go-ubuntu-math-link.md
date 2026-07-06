# Fix Go Ubuntu Build: Link Math Library

- Problem: Go Ubuntu build fails with `undefined reference to symbol 'exp@@GLIBC_2.29'` and `DSO missing from command line` due to missing math library linkage.
- Solution: Added `target_link_libraries(... m)` to both `coolboxbridge` and `coolbox_c_bindings` CMake targets to ensure the math library is linked.
- Impact: Fixes linker errors for math functions (exp, log, pow, etc.) in C/C++ code used by Go bindings. Go tests and builds should now succeed on Linux/Ubuntu.
- Location: See `_libraries/go_bindings/cbridge/CMakeLists.txt` and `_libraries/c_bindings/CMakeLists.txt` for the changes.
- Example patch:
  ```cmake
  target_link_libraries(coolboxbridge PUBLIC m)
  target_link_libraries(coolbox_c_bindings PUBLIC m)
  ```
- This step is required for any target using math functions from `<math.h>`/`<cmath>` on Linux.
