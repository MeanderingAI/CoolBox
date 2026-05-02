# CoolBox V Bindings

This package provides V bindings for CoolBox through the native C bindings in `_libraries/c_bindings`.

## Current scope

- Thin V wrapper over `coolbox_c_bindings`
- `Client` entry point similar to the Java bindings
- version, description, readiness, and capability metadata delegated to the linked CoolBox library
- V module metadata, example program, and smoke test

## Build

Build the native C bindings first:

```bash
cmake -S _libraries/c_bindings -B _libraries/c_bindings/build -DBUILD_TESTING=ON
cmake --build _libraries/c_bindings/build --config Release
```

Run the V smoke tests from the package directory:

```bash
cd _libraries/vlang_bindings
v test .
```

Run the example:

```bash
cd _libraries/vlang_bindings
v run examples/metadata.v
```

The module links against `_libraries/c_bindings/build` by default and also adds `_libraries/c_bindings/build/Release` on Windows for Visual Studio builds.

## Use

```v
import coolbox

fn main() {
    client := coolbox.create_default()
    println(client.version())
    println(client.describe())
}
```