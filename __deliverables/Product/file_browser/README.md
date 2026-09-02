# File Browser Product

This product is a native file browser application under `_Product/file_browser`.

Structure:

- `CMakeLists.txt` — build entry for the native product target
- `src/main.cpp` — product entry point
- `src/file_browser_app.cpp` — workspace dock host integration and browser behavior

The application reuses `file_browser_lib` for filesystem traversal and `graphics::full_application_window::WorkspaceDockHost` for the docked product shell.