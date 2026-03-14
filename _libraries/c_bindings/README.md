# CoolBox C Bindings

Plain C bindings for CoolBox backed by the native `metadata_management` library.

## Features

- Stable C header for external consumers
- Small metadata-oriented API surface
- Direct link against the CoolBox `metadata_management` shared library
- CMake build with CTest coverage
- Doxygen-ready API documentation

## Public API

The current bindings expose:

- `coolbox_c_version()`
- `coolbox_c_describe()`
- `coolbox_c_capability_count()`
- `coolbox_c_capability_at()`
- `coolbox_c_is_ready()`

All metadata values are sourced from the linked native CoolBox library rather than duplicated in the C layer.

## Build

From the repository root:

- `make build_c_bindings`

Or directly with CMake:

- `cmake -S _libraries/c_bindings -B _libraries/c_bindings/build -DBUILD_TESTING=ON`
- `cmake --build _libraries/c_bindings/build`
- `ctest --test-dir _libraries/c_bindings/build --output-on-failure`

## Docs

API docs are generated with Doxygen using `_libraries/c_bindings/Doxyfile`.

From the repository root, run:

- `bash _libraries/c_bindings/generate_docs.sh`

From inside `_libraries/c_bindings`, run:

- `bash ./generate_docs.sh`
