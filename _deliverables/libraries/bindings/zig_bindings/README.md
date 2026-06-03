# CoolBox Zig Bindings

This package provides Zig bindings for CoolBox through the native C bindings in `_deliverables/libraries/bindings/c_bindings`.

## Current scope

- Thin Zig wrapper over `coolbox_c_bindings`
- `Client` entry point similar to the Java, C3, and V bindings
- version, description, readiness, and capability metadata delegated to the linked CoolBox library
- example program and smoke test scaffold

## Build

Build the native C bindings first:

```bash
cmake -S _deliverables/libraries/bindings/c_bindings -B _deliverables/libraries/bindings/c_bindings/build -DBUILD_TESTING=ON
cmake --build _deliverables/libraries/bindings/c_bindings/build --config Release
```

Build the Zig example:

```bash
cd _deliverables/libraries/bindings/zig_bindings
zig build
```

Run tests:

```bash
cd _deliverables/libraries/bindings/zig_bindings
zig build test
```

On Windows, if the linker searches the configuration subdirectory, include `_deliverables/libraries/bindings/c_bindings/build/Release` in your library path.
