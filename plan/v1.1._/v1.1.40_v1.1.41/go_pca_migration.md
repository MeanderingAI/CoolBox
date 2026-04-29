# Go PCA Migration

## Status
- Still declared and wrapped through the broad bridge.

## Planned Files
- `_libraries/go_bindings/abi/pca.h`
- `_libraries/go_bindings/pca.go`
- `_libraries/go_bindings/native/pca.cpp`

## Planned Scope
- create
- fit
- transform
- fit-transform
- inverse-transform
- inspect components, variance, singular values, mean, and scale

## Notes
- This module has more matrix-heavy buffer contracts.
- It should follow the earlier ML modules once the helper patterns are stable.
