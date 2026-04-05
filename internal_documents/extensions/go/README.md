# Go Extension

## Location

- `_libraries/go_bindings`

## Surface

- cgo package backed by the native bridge library under `_libraries/go_bindings/cbridge`
- Provides Go-facing wrappers over CoolBox native ML functionality

## Installation

From the repository root:

```bash
cd _libraries/go_bindings
go test ./...
```

For consumers using a local checkout, ensure the bridge library has been built first.

## Setup And Build

Build the bridge archive:

```bash
cd _libraries/go_bindings/cbridge
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target coolboxbridge -- -j
```

Then run the Go package from the parent directory:

```bash
cd ..
export CGO_ENABLED=1
go test ./...
```

Windows setup:

```powershell
cmake -S _libraries/go_bindings/cbridge -B _libraries/go_bindings/cbridge/build -G "Visual Studio 17 2022" -A x64
cmake --build _libraries/go_bindings/cbridge/build --config Release --target coolboxbridge
```

## Setup Notes

- The Go package root should not contain standalone bridge `.cpp` files.
- The cgo layer expects the bridge archive from `cbridge/`.
- Recent CI fixes restored missing wrapper declarations and made `bridge.h` self-contained for cgo parsing.

## Packaging Notes

- Release page: https://github.com/MeanderingAI/CoolBox/releases
- Release workflows publish `go-extension-*` artifacts for Linux, macOS, and Windows.

## Narrow C ABI Refactor

- Refactor overview: `plan/v1.1.40_v1.1.41/go_narrow_c_abi_migration_overview.md`
- First implemented code slice: `abi/linear_regression.h`, `abi/common.h`, `linear_regression.go`, and `cgo_helpers.go`
- Module-by-module migration roadmap:
	- `plan/v1.1.40_v1.1.41/go_linear_regression_migration.md`
	- `plan/v1.1.40_v1.1.41/go_decision_tree_migration.md`
	- `plan/v1.1.40_v1.1.41/go_hmm_migration.md`
	- `plan/v1.1.40_v1.1.41/go_multi_arm_bandit_migration.md`
	- `plan/v1.1.40_v1.1.41/go_pca_migration.md`
	- `plan/v1.1.40_v1.1.41/go_svm_migration.md`
	- `plan/v1.1.40_v1.1.41/go_graphics_charting_migration.md`
	- `plan/v1.1.40_v1.1.41/go_gui_components_migration.md`