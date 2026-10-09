# CoolBox v3.2.4 Release Notes

## Python Packaging CI

Linux and macOS Python packaging no longer configures and builds the entire
CoolBox C++ tree with tests and binaries enabled. The Python package already
uses vendored sources on non-Windows platforms, so the previous native build
was redundant and could be terminated by the runner with exit code 143.

The non-Windows workflow now verifies the Python binding setup and required
vendored chart and wave-generator sources, then proceeds directly to building
the Python extension. The Windows workflow retains its explicit native
dependency and library build because it uses the MSVC-built libraries.

See the
[Python purchase workflow](../../../../.github/workflows/generate-purchase-python.yaml).

## LSP Artifact and Image Generation

LSP workflows now build the required executable targets explicitly and stage
the results from CMake's actual output directory:

```text
build/_deliverables/apps/lsp
```

The previous workflows searched `build/apps/lsp` and the build root, which
left the distribution directory empty even when compilation succeeded.
Fallback builds now disable unrelated products and tests, build only the
required LSP targets, and fail immediately when an expected executable is
missing.

See the
[LSP artifact workflow](../../../../.github/workflows/build-lsp.yaml) and
[LSP image workflow](../../../../.github/workflows/build-purchase-pipeline.yaml).

## Verification

- [x] Non-Windows Python packaging no longer starts the broad top-level C++
  build.
- [x] LSP workflows use the CMake output directory consistently.
- [x] Modified workflow YAML passed linting.
- [x] Required LSP targets built successfully in a focused local build.
