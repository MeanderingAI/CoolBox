# Fix Go macOS Build: Link Graphics/Charts Library

- Problem: Go macOS build fails with missing C++ symbols (e.g., Fractal, Canvas, decision tree, etc.) due to missing linkage of the graphics/charts implementation.
- Solution: Added `charts` library to the `target_link_libraries` for `coolboxbridge` in `_libraries/go_bindings/cbridge/CMakeLists.txt`.
- Impact: Ensures all graphics and plotting primitives (Fractal, Canvas, etc.) are available to Go bindings on all platforms, fixing linker errors on macOS (and Linux if present).
- Location: See `_libraries/go_bindings/cbridge/CMakeLists.txt` for the change.
- Example patch:
  ```cmake
  target_link_libraries(coolboxbridge PUBLIC m charts)
  ```
- This step is required for any target using graphics/chart primitives implemented in the charts library.
