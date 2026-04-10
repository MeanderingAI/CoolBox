**Rust Binding Guide**

- **Purpose:** Public overview of the Rust crate and its native wrapper build flow.
- **Location:** `_libraries/rust_bindings`

## Surface

- Documentation namespace: `coolbox::rust`.
- Rust crate `coolbox-rs`.
- Current documented scope focuses on a linear regression wrapper backed by CoolBox native code.

## Build

```bash
cd _libraries/rust_bindings
cargo build --release
cargo test
```

## Notes

- The crate metadata identifies the library as `coolbox_rs`.
- The namespace label used in the docs maps onto the crate and library identifiers rather than replacing them.
- The native wrapper is compiled through `build.rs` using `cxx-build`.

## Packaging

- Release page: https://github.com/MeanderingAI/CoolBox/releases
- Release workflows publish `rust-extension-*` artifacts.