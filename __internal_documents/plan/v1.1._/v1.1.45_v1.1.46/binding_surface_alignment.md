# Binding Surface Alignment

## Summary
- Defined a shared first-party binding surface for the existing language bindings.
- Added a small metadata client contract across the binding packages so they expose the same baseline semantics.
- Kept the existing ML, GUI, and graphics APIs intact; the new metadata client surface is additive.
- Kept this work independent from the VLang and C3 LSP deliverables.

## Scope
- `internal_documents/extensions/README.md` now defines the shared binding contract.
- `_libraries/c_bindings` now provides a client-oriented metadata API in addition to the existing flat C functions.
- `_libraries/java_bindings`, `_libraries/go_bindings`, `_libraries/rust_bindings`, `_libraries/python_bindings`, `_libraries/r_bindings/coolboxr`, `_libraries/c3_bindings`, and `_libraries/vlang_bindings` now expose the same metadata client semantics.

## Shared Metadata Client Surface
- default client construction
- endpoint-based client construction
- endpoint inspection
- version lookup
- description lookup
- readiness check
- capability enumeration
- indexed capability lookup

## Independence From LSP
- The shared binding surface is not part of the LSP runtime.
- The LSP binaries under `apps/lsp` remain separate deliverables with separate packaging and validation paths.
- The binding contract exists for language bindings and extension-facing library consumers, not for the language-server binaries themselves.

## Validation Notes
- Editor diagnostics reported no errors in the updated binding packages and binding-surface documentation at edit time.
- End-to-end runtime validation for every language binding still depends on external toolchains that were not installed in the workspace at edit time.