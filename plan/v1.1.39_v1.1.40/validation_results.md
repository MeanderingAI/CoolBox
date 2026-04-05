# Validation Results (v1.1.39 -> v1.1.40)

## Static Validation
- Validation was re-run after restoring the missing Go bindings wrapper declarations.
- `_libraries/go_bindings/bindings.go` reported no diagnostics after reintroducing the exported constants and wrapper/result types.
- A follow-up read confirmed the package now again declares `LinearRegression`, `HMM`, `BanditArm`, `BanditAgent`, and `SimulationResult`.

## Static Validation
- Validation was re-run after making the Go bridge header self-contained.
- `_libraries/go_bindings/bridge.h` and `_libraries/go_bindings/bindings.go` reported no diagnostics after adding the missing standard includes and direct forward-declaration include.
- A follow-up read confirmed `bridge.h` now includes `<stdint.h>`, `<stddef.h>`, and `bridge_forward.h` before exporting the bridge API.

## Static Validation
- Validation was re-run after restoring the `docs_publish` tag trigger.
- `.github/workflows/build-purchase-pipeline.yaml` reported no diagnostics after expanding the `docs_publish` job guard to allow both `workflow_dispatch` and `refs/tags/v*` events.
- A follow-up read confirmed the skip-causing `github.event_name == 'workflow_dispatch'` restriction is no longer the only path for docs publishing.

## Remaining Note
- User-provided CI logs confirm the `macos-latest` / `macos-arm64` Go build was previously failing on missing wrapper declarations in `_libraries/go_bindings/extra_stubs.go` and `_libraries/go_bindings/bindings.go`.
- User-provided CI logs also confirm the Windows Go build was failing on cgo preamble parsing in `_libraries/go_bindings/bindings.go` because `_libraries/go_bindings/bridge.h` was not self-contained.
- A local `go test ./...` attempt was started from `_libraries/go_bindings`, but terminal output returned garbled in this environment and could not be used as reliable runtime evidence.
- Final runtime validation still depends on re-running `generate_purchase_go` in GitHub Actions after both the wrapper-declaration fix and the self-contained bridge-header fix, and re-running the release pipeline to confirm `docs_publish` starts on tag events instead of being skipped.