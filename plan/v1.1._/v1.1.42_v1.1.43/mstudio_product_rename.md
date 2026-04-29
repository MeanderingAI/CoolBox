# MStudio Product Rename

## Summary
- Renamed the active desktop editor product identity from the earlier notepad and `worksplace_editor` naming mix into a consistent `MStudio` product path, target, and executable name.
- Moved the active product sources under `_Product/MStudio` and updated the build, packaging, docs, and VS Code task surfaces to use the new product name.

## Product Changes
- `_Product/CMakeLists.txt` now includes `_Product/MStudio` instead of the old `_Product/notepad` path.
- `_Product/MStudio/CMakeLists.txt` now defines the executable target as `MStudio` instead of `worksplace_editor`.
- `_Product/MStudio/src/main.cpp` and the GUI implementation now present the product title as `CoolBox MStudio`.
- The installer-abstraction integration for prerelease packaging now renders `MStudio` as both the product name and executable identifier.

## VS Code Integration
- `.vscode/tasks.json` now builds and launches the product through `MStudio`-named debug tasks.
- The debug launch path now points at `build/_Product/MStudio/Debug/MStudio.exe` instead of the older executable name.

## Documentation And Asset Generation
- `_scripts/build_product_assets.py` now publishes the product metadata under the slug `mstudio` with build and run hints that reference `MStudio`.
- The current plan and validation notes were updated so the docs trail reflects the renamed target and runtime path.

## Result
- The product identity is now internally consistent across product source path, target name, executable name, release packaging labels, and VS Code task labels.