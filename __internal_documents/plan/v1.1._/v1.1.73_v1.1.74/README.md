# CoolBox v1.1.73 → v1.1.74 Migration Notes

## Summary of Changes

### LSP Modularization
- All LSP-related files and packages were moved from `TOOLS` to a dedicated `LSP` folder under `_libraries/packages/LSP`.
- All CMake references, include paths, and build scripts were updated to use the new `LSP` location.
- Documentation and plan files referencing LSP in `TOOLS` were updated to reference `LSP`.

### Deep Learning Package
- The deep learning package was moved from `ML/deep_learning` to `DL/deep_learning`.
- All CMake and emscripten bindings were updated to reference the new location.
- Duplicate or legacy references to `deep_learning` were removed from the build system.

### Build System
- The build and test system was updated to reflect the new modular structure.
- All LSP and DL modules build and link successfully.
- The only remaining build blocker is a code error in `gabor_patches.cpp` (see below).

### Outstanding Issue
- The Gabor patches module fails to build due to a C++ error:
  - `error C2064: term does not evaluate to a function taking 2 arguments` at `gabor_patches.cpp(298,9)`
  - This is caused by an invalid call to `.at(r, c)` on a matrix element in the mean response calculation.

---

## Migration Checklist
- [x] Move all LSP files from TOOLS to LSP
- [x] Update all CMake and script references for LSP
- [x] Move deep_learning from ML to DL
- [x] Update all CMake and script references for DL
- [x] Remove duplicate/legacy deep_learning references
- [x] Validate build and test (except for Gabor patches error)
- [ ] Fix Gabor patches code error

---

## Next Steps
- Fix the `.at(r, c)` usage in `gabor_patches.cpp` to resolve the build error.
- Re-run build and test to confirm all modules pass.
