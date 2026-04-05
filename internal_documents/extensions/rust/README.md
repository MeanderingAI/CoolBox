# Rust Extension

## Location

- `_libraries/rust_bindings`

## Surface

- Rust crate `coolbox-rs`
- Current documented scope focuses on a linear regression wrapper backed by CoolBox native code

## Installation

From the repository root:

```bash
cd _libraries/rust_bindings
cargo build --release
```

## Setup And Build

Build and test:

```bash
cd _libraries/rust_bindings
cargo build --release
cargo test
```

## Setup Notes

- The crate metadata identifies the library as `coolbox_rs`.
- The native wrapper is compiled through `build.rs` using `cxx-build`.

## Packaging Notes

- Release page: https://github.com/MeanderingAI/CoolBox/releases
- Release workflows publish `rust-extension-*` artifacts.