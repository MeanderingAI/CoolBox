# Go Narrow C ABI Migration Overview

## Goal
- Move the Go bindings away from a broad native bridge surface and toward a small, stable C ABI that Go can consume through cgo.
- Make the C-facing API the supported compatibility boundary.
- Keep native C++ implementation details behind that boundary.
- Split the bridge implementation by feature module instead of growing one monolithic bridge file.

## Current State
- `bindings.go` still contains the majority of the broad Go wrapper surface.
- `bridge.h` still acts as a compatibility umbrella across unrelated domains.
- `cbridge/bridge.cpp` still behaves like a broad native bridge compile unit.
- The first code-level extraction step has already been applied for the linear regression feature module.

## Current Implementation Status
- Dedicated ABI headers now exist under `abi/common.h` and `abi/linear_regression.h`.
- The Go linear regression wrapper now lives in `linear_regression.go`.
- Shared cgo pointer and error helpers now live in `cgo_helpers.go`.
- `bridge.h` now includes the dedicated linear-regression ABI header instead of declaring that feature module inline.

## Problems To Solve
- `bindings.go` still carries a large number of include paths and platform linker details.
- `bridge.h` still mixes graphics, GUI, and ML model declarations in one header.
- `cbridge/bridge.cpp` still pulls broad native source and header dependencies into one native build target.
- The build still depends on an externally built `coolboxbridge` archive rather than a clearly scoped native layer.

## Target Design Rules
1. One feature module per ABI header when practical.
2. Opaque handles only.
3. Flat C functions for create, fit, predict, inspect, and free operations.
4. Consistent error handling and explicit ownership rules.
5. Native implementation files grouped by domain instead of one monolithic bridge translation unit.

## Recommended Future Layout
- `abi/common.h`
- `abi/linear_regression.h`
- `abi/decision_tree.h`
- `abi/hmm.h`
- `abi/multi_arm_bandit.h`
- `abi/pca.h`
- `abi/svm.h`
- `abi/graphics.h`
- `abi/gui.h`
- `native/common.cpp`
- `native/linear_regression.cpp`
- `native/decision_tree.cpp`
- `native/hmm.cpp`
- `native/multi_arm_bandit.cpp`
- `native/pca.cpp`
- `native/svm.cpp`
- `native/graphics.cpp`
- `native/gui.cpp`

## Recommended Migration Order
1. Linear regression
2. Decision tree
3. HMM
4. Multi-arm bandit
5. PCA
6. SVM
7. Graphics and charting
8. GUI components

## Build Implications
- Fewer headers per module.
- Smaller native compilation units.
- Clearer dependency boundaries.
- Lower risk of unrelated bridge changes breaking Go builds.
- A later phase can evaluate whether cgo can compile narrow native units directly instead of linking a separately built `coolboxbridge` archive.
