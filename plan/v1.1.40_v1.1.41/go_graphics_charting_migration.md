# Go Graphics And Charting Migration

## Status
- Still declared and wrapped through the broad bridge.

## Planned Files
- `_libraries/go_bindings/abi/graphics.h`
- `_libraries/go_bindings/graphics.go`
- `_libraries/go_bindings/native/graphics.cpp`

## Planned Scope
- color
- canvas
- graph
- table
- basic rendering utilities

## Notes
- This module should come later because it mixes graphics types, rendering outputs, and UI-adjacent concepts that are less suitable for the first ABI extractions.
