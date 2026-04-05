# Go GUI Components Migration

## Status
- Still declared and wrapped through the broad bridge.

## Planned Files
- `_libraries/go_bindings/abi/gui.h`
- `_libraries/go_bindings/gui.go`
- `_libraries/go_bindings/native/gui.cpp`

## Planned Scope
- toolbar
- dock panel
- layer list
- property inspector
- file tree
- radio selector
- checkbox group

## Notes
- This should be one of the last modules to migrate because it is closer to toolkit glue than core ML functionality.
