# Go Multi-Arm Bandit Migration

## Status
- Still declared and wrapped through the broad bridge.

## Planned Files
- `_libraries/go_bindings/abi/multi_arm_bandit.h`
- `_libraries/go_bindings/multi_arm_bandit.go`
- `_libraries/go_bindings/native/multi_arm_bandit.cpp`

## Planned Scope
- create epsilon-greedy, UCB, Thompson-sampling, and decaying-epsilon agents
- run simulations
- fetch arm results
- free

## Notes
- This module is a good candidate after HMM because the data exchange is still manageable and mostly scalar or flat-array based.
