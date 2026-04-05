# Go Decision Tree Migration

## Status
- Still declared and wrapped through the broad bridge.

## Planned Files
- `_libraries/go_bindings/abi/decision_tree.h`
- `_libraries/go_bindings/decision_tree.go`
- `_libraries/go_bindings/native/decision_tree.cpp`

## Planned Scope
- create
- fit
- predict
- free

## Notes
- This is the next recommended module because it already has a bounded Go wrapper and simpler ownership than HMM or graphics APIs.
- It should follow the same extraction pattern used for linear regression.
