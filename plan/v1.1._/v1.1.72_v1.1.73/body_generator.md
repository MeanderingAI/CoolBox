# body_generator Migration Log (v1.1.73-v1.1.74)

## 2026-04-22: Fixed graphics.lib Linker Error

- Removed `graphics` from `target_link_libraries` for `body_generator_ui` in `_Product/body_generator/CMakeLists.txt`.
- Rationale: The `graphics` target is INTERFACE only and does not produce a `graphics.lib`. Linking to it caused a fatal linker error (LNK1181: cannot open input file 'graphics.lib').
- Only `full_application_window` (and other real libraries) should be linked directly.
- This resolves the linker error and allows the build to proceed.
