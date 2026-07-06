MStudio Product

This product is a native desktop editor application under `_Product/MStudio`.

Structure:

- `CMakeLists.txt` - build entry for the native product target
- `src/main.cpp` - desktop entry point
- `src/gui_gtk_like.cpp` - native GUI shell and editor behavior

The emitted executable target is `MStudio`. On Windows, the application opens a real GUI window with native controls and uses the graphics component models for the workspace tree, inspector, and editor-state preview.
