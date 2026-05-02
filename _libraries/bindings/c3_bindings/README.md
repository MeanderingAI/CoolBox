# CoolBox C3 Bindings

This package provides C3 bindings for CoolBox through the native C bindings in `_libraries/c_bindings`.

## Current scope

- Thin C3 wrapper over `coolbox_c_bindings`
- `Client` entry point similar to the Java bindings
- version, description, readiness, and capability metadata delegated to the linked CoolBox library
- example program and smoke test scaffold

## Build

Build the native C bindings first:

```bash
cmake -S _libraries/c_bindings -B _libraries/c_bindings/build -DBUILD_TESTING=ON
cmake --build _libraries/c_bindings/build --config Release
```

Compile the example from the repository root:

```bash
c3c compile _libraries/c3_bindings/src/coolbox.c3 _libraries/c3_bindings/examples/metadata.c3 -I _libraries/c_bindings/include -L _libraries/c_bindings/build -l coolbox_c_bindings
```

Run the smoke tests:

```bash
c3c compile-test _libraries/c3_bindings/src/coolbox.c3 _libraries/c3_bindings/tests/coolbox_test.c3 -I _libraries/c_bindings/include -L _libraries/c_bindings/build -l coolbox_c_bindings
```

On Windows, if the linker searches the configuration subdirectory, add an extra library path such as `-L _libraries/c_bindings/build/Release`.

## Use

```c3
module example;
import coolbox;
import std::io;

fn void main()
{
    coolbox::Client client = coolbox::create_default();
    io::printfn("%s", client.version());
    io::printfn("%s", client.describe());
}
```