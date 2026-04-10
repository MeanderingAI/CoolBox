**Go Binding Guide**

- **Purpose:** Public overview of the Go bindings and their native bridge requirements.
- **Location:** `_libraries/go_bindings`

## Surface

- Documentation namespace: `coolbox::go`.
- cgo package backed by the native bridge library under `_libraries/go_bindings/cbridge`.
- Provides Go-facing wrappers over CoolBox native machine learning functionality.
- The underlying Go module path remains `github.com/MeanderingAI/CoolBox/_libraries/go_bindings`.

## Build

Build the native bridge:

```bash
cd _libraries/go_bindings/cbridge
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target coolboxbridge -- -j
```

Then run the Go package:

```bash
cd ..
export CGO_ENABLED=1
go test ./...
```

Windows build:

```powershell
cmake -S _libraries/go_bindings/cbridge -B _libraries/go_bindings/cbridge/build -G "Visual Studio 17 2022" -A x64
cmake --build _libraries/go_bindings/cbridge/build --config Release --target coolboxbridge
```

## Notes

- The cgo layer expects the bridge archive produced from `cbridge/`.
- The package root should not carry standalone bridge `.cpp` files.
- Current refactor work is narrowing the Go layer onto a smaller stable C ABI.

## Packaging

- Release page: https://github.com/MeanderingAI/CoolBox/releases
- Release workflows publish `go-extension-*` artifacts for Linux, macOS, and Windows.