**C Binding Guide**

- **Purpose:** Public overview of the native C integration layer for CoolBox.
- **Location:** `_libraries/c_bindings`

## Surface

- Documentation namespace: `coolbox::c`.
- Stable C API backed by the native `metadata_management` library.
- Acts as a low-level integration layer and the native dependency base for the Java package.
- Exported symbol names remain the C-style `coolbox_c_*` functions.

## Build

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

## Public API Highlights

- `coolbox_c_version()`
- `coolbox_c_describe()`
- `coolbox_c_capability_count()`
- `coolbox_c_capability_at()`
- `coolbox_c_is_ready()`

## Documentation

Generate API docs with Doxygen:

```bash
bash _libraries/c_bindings/generate_docs.sh
```

## Packaging

- Release page: https://github.com/MeanderingAI/CoolBox/releases
- Release workflows publish `c-extension-*` artifacts.
- The Java packaging flow consumes the prebuilt C extension artifact during its own packaging stage.