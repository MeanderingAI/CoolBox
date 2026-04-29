# MStudio GUI Product

## Summary
- Reworked `_Product/MStudio` from a console-style scaffold into a native desktop GUI application.
- The MStudio product now opens a real desktop window instead of printing an ASCII window simulation to stdout.
- The GUI state is driven by the GRAPHICS components library models for the toolbar, file tree, property inspector, and editor preview.
- The top-level product window is now hosted by `graphics::full_application_window::FullApplicationWindow` instead of a product-local Win32 window bootstrap.

## Product Changes
- Simplified `_Product/MStudio/src/main.cpp` to a platform-neutral desktop entry path.
- Replaced the old command-loop behavior in `_Product/MStudio/src/gui_gtk_like.cpp` with a native window, menu commands, toolbar buttons, file-open/save dialogs, a multiline editor control, and status text.
- Kept the editor buffer logic inside the product class so file-open and file-save continue to work through the same product API.
- Reworked the product host window path so MStudio attaches its child controls to the native handle provided by `full_application_window` and uses render hooks to keep the component-driven inspector and status panes refreshed.

## Graphics Component Usage
- The toolbar labels come from `graphics::components::ToolbarModel`.
- The left sidebar content is built from `graphics::components::FileTreeModel`.
- The inspector pane is populated from `graphics::components::PropertyInspectorModel` and `graphics::components::LayerListModel`.
- The right-side preview also renders an `EditableTextView` component model so the native UI remains tied to the graphics component layer instead of bypassing it.

## Build Integration
- Added `_Product/CMakeLists.txt` so products can be added through the root project.
- Updated the root `CMakeLists.txt` to include `_Product` through a `BUILD_PRODUCTS` option.
- Updated `_Product/MStudio/CMakeLists.txt` to build the `MStudio` target against the GRAPHICS `components`, `full_application_window`, and reusable `file_browser_lib` libraries.
- Updated `_Product/MStudio/CMakeLists.txt` so the product path, CMake target, and executable name now all use `MStudio` consistently.
- Added Windows post-build staging in `_Product/MStudio/CMakeLists.txt` so `full_application_window.dll`, `components.dll`, and `json.dll` are copied next to `MStudio.exe` for runtime loading.

## Documentation And Pipeline Follow-Up
- The docs portal now needs to present MStudio as both a standalone product and a consumer of `components`, `full_application_window`, and `file_browser_lib` in the generated dependency map.
- Product-oriented CI should build `MStudio` explicitly instead of relying on an incidental full-tree build, so product regressions surface even when library packaging still succeeds.
- The generated product page should remain aligned with the editor architecture by keeping its build hints, run hints, and related-library links synchronized with the CMake target graph.

## Release Packaging Follow-Up
- The product now has an optional prerelease packaging path that can render install guidance through `OS_GENERICS/installer_abstraction` when the release workflow enables `BUILD_PRODUCT_INSTALLER_ABSTRACTIONS`.
- Ordinary local and makefile-driven builds keep the normal editor startup path because the installer abstraction is linked only when the release pipeline opts into it.
- Packaged release artifacts should continue to describe MStudio as a prerelease GUI product that depends on the staged runtime libraries copied beside the executable.

## Result
- The root CMake build now produces `_Product/MStudio/Debug/MStudio.exe` on Windows.
- The MStudio product opens an actual GUI, uses `full_application_window` as its host shell, and reuses `file_browser_lib` for its workspace pane.
- The MStudio runtime folder now contains the required GUI DLL dependencies, fixing the Windows startup failure caused by `full_application_window.dll` not being found beside the executable.
- The release packaging path can now swap the startup content to prerelease/install guidance without affecting local development builds.