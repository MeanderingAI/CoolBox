**CoolBox Language Guides**

- **Purpose:** Public overview of the language-specific integration guides available in this repository.

## Core Language Specs

- `MATLAB.md`: Supported MATLAB subset for the repository parser and LSP work.
- `VHDL.md`: Supported VHDL subset for the repository parser and LSP work.

## Binding And Integration Guides

- `C.md`: Native C API and build flow.
- `Go.md`: Go bindings backed by the cgo bridge.
- `Java.md`: Java package backed by the C bindings.
- `JavaScript.md`: JavaScript and WebAssembly outputs built with Emscripten.
- `Python.md`: Python bindings, packaging, and native library resolution notes.
- `R.md`: R package layouts and build paths.
- `Rust.md`: Rust crate and native wrapper build notes.

## Notes

- The binding guides summarize the current checked-in repository structure and build flows.
- The binding guides now pair each binding with a Rust-style documentation namespace such as `coolbox::python` while retaining the real package-manager identifiers used by each ecosystem.
- Release artifacts for each language are published through the repository release workflows.