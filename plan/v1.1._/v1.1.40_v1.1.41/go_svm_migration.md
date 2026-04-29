# Go SVM Migration

## Status
- Still declared and wrapped through the broad bridge.

## Planned Files
- `_libraries/go_bindings/abi/svm.h`
- `_libraries/go_bindings/svm.go`
- `_libraries/go_bindings/native/svm.cpp`

## Planned Scope
- create
- fit
- predict
- inspect kernel configuration where needed
- free

## Notes
- Kernel configuration and training parameter surfaces make this a later module than linear regression or decision tree.
