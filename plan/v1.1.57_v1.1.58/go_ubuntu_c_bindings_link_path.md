# Go Ubuntu C Bindings Link Path

## Summary
- Fixed the Ubuntu Go purchase workflow so the Go bindings can find and link the native `coolbox_c_bindings` library during `go test`.
- Expanded the Go cgo link search paths to include both the root C bindings build directory and the config-specific `build/Release` output directory.
- Added runtime loader environment setup for Go tests so Linux and macOS can also load `coolbox_c_bindings` after linking succeeds.

## Problem
- The Ubuntu Go purchase workflow failed during `go test` link with:

```text
/usr/bin/ld: cannot find -lcoolbox_c_bindings: No such file or directory
```

- The Go package linked `coolbox_c_bindings` directly from `_libraries/go_bindings/client.go`.
- Its Linux and macOS cgo link flags only searched `_libraries/c_bindings/build`.
- The C bindings CMake target can emit artifacts into both the root build directory and the config-specific `build/Release` directory, so the Go linker could miss the shared library even when the native build step succeeded.

## Files Updated
- `_libraries/go_bindings/client.go`
- `.github/workflows/generate-purchase-go.yaml`
- `_local_build_pipeline/scripts/jobs/generate-purchase-go.sh`

## Change
- Updated the Go cgo `LDFLAGS` in `client.go` so Linux and macOS search both `_libraries/c_bindings/build` and `_libraries/c_bindings/build/Release` for `coolbox_c_bindings`.
- Exported `COOLBOX_C_BINDINGS_BUILD_DIR` and `COOLBOX_C_BINDINGS_BUILD_CONFIG_DIR` in the Go purchase workflow.
- Prepended those directories to `LD_LIBRARY_PATH` and `DYLD_LIBRARY_PATH` before `go test` so the shared library can be loaded at runtime after the link step.
- Mirrored the same runtime environment setup in the local `generate-purchase-go.sh` script.

## Result
- Ubuntu Go purchase builds should no longer fail immediately with `cannot find -lcoolbox_c_bindings` when the native C bindings were built into the config-specific output directory.
- The Go purchase workflow now matches the runtime loader handling already required by the Rust purchase workflow for the same native library.
- Any remaining Go purchase failures after this point are more likely to be real test or packaging issues rather than missing native library search paths.