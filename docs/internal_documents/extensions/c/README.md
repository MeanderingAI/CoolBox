# C Extension

## Location

- `_libraries/c_bindings`

## Surface

- Documentation namespace: `coolbox::c`
- Stable C API backed by the native `metadata_management` library
- Primarily used as a low-level integration layer and as the native dependency base for Java packaging
- Exported symbol names remain the C-style `coolbox_c_*` functions

## Installation

Build and install from source with CMake:

```bash
cmake -S _libraries/c_bindings -B _libraries/c_bindings/build -DBUILD_TESTING=ON
cmake --build _libraries/c_bindings/build
```

## Setup And Build

Repository helper target:

```bash
make build_c_bindings
```

Direct CMake flow:

```bash
cmake -S _libraries/c_bindings -B _libraries/c_bindings/build -DBUILD_TESTING=ON
cmake --build _libraries/c_bindings/build
ctest --test-dir _libraries/c_bindings/build --output-on-failure
```

## Public API

- `coolbox_c_version()`
- `coolbox_c_describe()`
- `coolbox_c_capability_count()`
- `coolbox_c_capability_at()`
- `coolbox_c_is_ready()`

## Documentation Setup

Generate API docs with Doxygen:

```bash
bash _libraries/c_bindings/generate_docs.sh
```

## Packaging Notes

- Release page: https://github.com/MeanderingAI/CoolBox/releases
- Release workflows publish `c-extension-*` artifacts.
- The Java packaging flow consumes the prebuilt C extension artifact during its own packaging stage.