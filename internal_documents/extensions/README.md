# CoolBox Binding Surface

This document defines the minimum metadata surface that every first-party CoolBox language binding must provide.

## Goal

Every binding package should expose the same metadata-oriented client contract even if it also exposes larger language-specific ML or GUI APIs.

This binding contract is independent from the LSP binaries under `apps/lsp`. Language-server deliverables and binding/extension library deliverables are related by language support, but they are not the same runtime surface and should not be treated as the same product boundary.

The shared surface is intentionally small:

- construct a default client
- construct a client for an explicit endpoint string
- inspect the client endpoint
- query version
- query description
- check readiness
- list capabilities
- fetch a capability at an index

## Canonical Semantics

The canonical endpoint value is:

- `local://coolbox`

The canonical metadata source is the native C bindings in `_libraries/c_bindings`.

Bindings may implement the client in one of two ways:

1. Wrap the C client object directly.
2. Store the endpoint locally and delegate metadata calls to the C metadata functions.

Either implementation is acceptable as long as the public surface matches the semantics below.

## Required Surface

Each binding must expose the equivalent of the following operations in idiomatic naming for its language:

- `create_default()` or equivalent constructor
- `for_endpoint(endpoint)` or equivalent constructor
- `endpoint`
- `version()`
- `describe()`
- `is_ready()`
- `capabilities()`
- `capability_at(index)`

If the language has a natural list-length or iterator abstraction, `capability_count()` is optional but recommended.

## C Baseline

The C bindings are the compatibility baseline and expose both the legacy flat functions and a client-oriented API.

Legacy functions remain supported:

- `coolbox_c_version()`
- `coolbox_c_describe()`
- `coolbox_c_capability_count()`
- `coolbox_c_capability_at()`
- `coolbox_c_is_ready()`

Client functions provide the shared surface for low-level consumers:

- `coolbox_c_create_default_client()`
- `coolbox_c_create_client()`
- `coolbox_c_destroy_client()`
- `coolbox_c_client_endpoint()`
- `coolbox_c_client_version()`
- `coolbox_c_client_describe()`
- `coolbox_c_client_capability_count()`
- `coolbox_c_client_capability_at()`
- `coolbox_c_client_is_ready()`

## Compatibility Rule

Existing language-specific ML, graphics, and GUI APIs are not removed by this contract. The shared client surface is additive and must coexist with the richer binding APIs already present in Go, Rust, Python, and R.