# File Browser Product

## Summary
- Added a reusable `file_browser_lib` library under `_libraries/packages/TOOLS/file_browser`.
- Added a standalone `_Product/file_browser` desktop application backed by the shared `WorkspaceDockHost` abstraction.
- The library is independent of the host product and can be embedded into other applications, including the editor product.

## Library Design
- `tools::file_browser::FileBrowserComponent` owns a root path and a flattened, formatted directory tree.
- The component exposes browsable `FileEntry` values with path, label, directory flag, and depth metadata.
- Refreshing and selection lookup are handled in the library so embedding products do not need to reimplement filesystem traversal.

## Product Design
- The standalone `file_browser` app uses the same shared dock host used by the editor product.
- The left side exposes `Files` and `Settings` through the host navigation rail.
- Opening a workspace re-roots the browser, double-clicking a directory drills into it, and double-clicking a file loads a preview into the center pane.

## Editor Integration
- The editor product now uses `FileBrowserComponent` to populate its workspace pane instead of manually traversing the filesystem in the product source.
- This keeps file-tree behavior reusable and consistent between the editor and the standalone file browser.

## Documentation And Pipeline Follow-Up
- The docs portal should describe `file_browser` as both a reusable TOOLS library surface and a standalone product that demonstrates the `WorkspaceDockHost` flow.
- The generated dependency map should show `file_browser` as a product consumer of `file_browser_lib`, `full_application_window`, and `components` so the reuse story is visible without opening the product source.
- Product build automation should compile `file_browser` explicitly so changes in the shared host-window and components layers are validated against a real GUI product target.

## Release Packaging Follow-Up
- The standalone product now has an optional prerelease packaging presentation that renders installation notes through `OS_GENERICS/installer_abstraction` when the release workflow enables that path.
- Ordinary local builds keep the normal file preview and inspector flow because the installer abstraction stays disabled unless the release pipeline turns it on explicitly.
- Packaged release artifacts should keep presenting `file_browser` as both a runtime product and a reference host for `file_browser_lib`.