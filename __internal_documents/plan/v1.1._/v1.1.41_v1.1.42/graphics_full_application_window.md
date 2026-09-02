# Graphics Full Application Window

## Summary
- Added a new GRAPHICS library named `full_application_window`.
- The library creates a real native application window using the operating-system-appropriate backend.
- The implementation keeps the API in the GRAPHICS layer while selecting the platform backend at compile time.
- Added a shared polymorphic base contract in `graphics::GraphicsObject` so window hosts, component containers, and public GRAPHICS models can now be treated uniformly.
- Added a lightweight `GraphicsObjectRegistry` factory layer so callers can register and create GRAPHICS objects by key through the shared polymorphic contract.

## Backends
- Windows: Win32 window creation through `CreateWindowEx` and the standard message pump
- macOS: Cocoa window creation through the Objective-C runtime bridge
- Linux: X11 window creation when X11 development libraries are available
- Fallback: a headless backend when no native windowing backend is available at build time

## API Surface
- `graphics::GraphicsObject`
- `graphics::GraphicsObjectRegistry`
- `graphics::full_application_window::WindowConfig`
- `graphics::full_application_window::FullApplicationWindow`
- `graphics::full_application_window::Backend`
- `graphics::full_application_window::native_backend()`
- `graphics::full_application_window::RenderEvent`
- `graphics::full_application_window::RenderHooks`

## Shared Contract Coverage
- The shared `GraphicsObject` base now covers the major public GRAPHICS runtime classes, including `Component`, `ComponentHolder`, `Panel`, `WindowSimulator`, `FullApplicationWindow`, and `WorkspaceDockHost`.
- The same base was extended to public model/config structs used by those APIs, including component models, menu models, file-tree nodes, CAD viewport models, `WindowConfig`, `RenderEvent`, `WorkspacePanelLayout`, `WorkspaceNavItem`, and `WorkspaceDockModels`.
- Each participating type now exposes a stable `graphics_object_kind()` and `graphics_object_name()` so generic tooling can inspect them through a uniform interface.

## Test Coverage
- Extended the existing GRAPHICS component, window, and full-application-window tests to assert compile-time inheritance from `GraphicsObject`.
- Added runtime assertions that validate the shared `graphics_object_kind()` and `graphics_object_name()` contract through `GraphicsObject&` references.
- Added registry-based tests that create component, window, and full-application-window objects polymorphically through `GraphicsObjectRegistry`.

## Build Integration
- Added `_libraries/packages/GRAPHICS/full_application_window/CMakeLists.txt`.
- Updated `_libraries/packages/GRAPHICS/CMakeLists.txt` so standalone GRAPHICS builds include the new library.
- Updated `_libraries/CMakeLists.txt` so the root library build includes the new GRAPHICS target.

## Result
- The GRAPHICS package now includes a native-window abstraction instead of only simulated window rendering.
- The native-window abstraction now exposes rendering and lifecycle hooks for create, render, resize, tick, and close events.
- The GRAPHICS package now has one shared polymorphic contract across its public windowing and component layers, plus a simple registry/factory path for generic object creation.