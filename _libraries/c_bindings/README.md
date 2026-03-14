# CoolBox C Bindings

Plain C bindings scaffold for CoolBox.

## Features

- Stable C header for external consumers
- Small metadata-oriented API surface
- CMake build with CTest coverage
- Doxygen-ready API documentation

## Public API

The current scaffold exposes:

- `coolbox_c_version()`
- `coolbox_c_describe()`
- `coolbox_c_capability_count()`
- `coolbox_c_capability_at()`
- `coolbox_c_is_ready()`

## Build

From the repository root:

- `make build_c_bindings`

Or directly with CMake:

- `cmake -S _libraries/c_bindings -B _libraries/c_bindings/build -DBUILD_TESTING=ON`
- `cmake --build _libraries/c_bindings/build`
- `ctest --test-dir _libraries/c_bindings/build --output-on-failure`

## Docs

API docs are generated with Doxygen using `_libraries/c_bindings/Doxyfile`.
