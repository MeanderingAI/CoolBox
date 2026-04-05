# Go HMM Migration

## Status
- Still declared and wrapped through the broad bridge.

## Planned Files
- `_libraries/go_bindings/abi/hmm.h`
- `_libraries/go_bindings/hmm.go`
- `_libraries/go_bindings/native/hmm.cpp`

## Planned Scope
- create
- set and get initial probabilities
- set and get transition matrix
- set and get emission matrix
- log likelihood
- most-likely-states
- free

## Notes
- Matrix ownership and buffer sizing make this a moderate-complexity module.
- It should follow decision tree after the helper patterns are stable.
