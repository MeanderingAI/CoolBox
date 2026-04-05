# Go Graphics And Charting Narrow ABI Extraction

## Summary

The Go graphics and charting bindings were extracted from the monolithic bindings file into a dedicated module file and ABI header. This covers the rendering-oriented wrappers and leaves the GUI/component wrappers as the main remaining broad slice.

## Changes

- added `_libraries/go_bindings/abi/graphics.h`
- added `_libraries/go_bindings/graphics.go`
- updated `_libraries/go_bindings/bridge.h` to include the new graphics ABI header
- removed inline graphics and charting declarations from `_libraries/go_bindings/bridge.h`
- removed graphics and charting wrapper implementations from `_libraries/go_bindings/bindings.go`

## Scope

- color
- canvas
- graph
- table
- font face
- fractal
- function plot
- parametric plot
- polar plot
- histogram plot

## Rationale

This extraction moves the rendering-oriented wrapper surface out of the umbrella Go file and consolidates it behind a dedicated ABI header. That keeps the Go API stable while making the remaining monolithic area much smaller and more clearly focused on GUI/component primitives.

## Validation

- editor diagnostics reported no issues in the touched Go and header files
- runtime validation remains limited in this environment because terminal output is still unreliable

## Next Steps

- extract the GUI/component wrappers next
- or begin native-side file splits if the current milestone only requires wrapper and ABI modularization
