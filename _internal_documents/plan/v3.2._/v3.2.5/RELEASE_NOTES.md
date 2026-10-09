# CoolBox v3.2.5 Release Notes

## Cross-Platform Packaging and LSP Follow-Up

v3.2.5 continues the CI stabilization work from v3.2.4 and makes the
packaging paths fail clearly instead of producing success-shaped empty
artifacts.

### Python Bindings on Linux and macOS

The Python purchase workflow avoids the unrelated full-project C++ build on
Linux and macOS. These platforms set
`COOLBOX_PYTHON_FORCE_VENDOR_SOURCES=1`, so the workflow verifies the vendored
binding sources and builds the extension directly. This prevents long,
resource-intensive jobs from stalling while compiling unrelated test and
library targets.

### LSP Executable Staging

The LSP artifact and Docker-image workflows explicitly build the executable
targets they package and copy them from:

```text
build/_deliverables/apps/lsp
```

Windows multi-configuration builds are also handled from their `Release`
subdirectory. Missing outputs now stop the staging step with a diagnostic
instead of being hidden by permissive copy fallbacks.

### Validation

- [x] Verified the corrected LSP output location with local Release builds.
- [x] Built `plang_lsp`, `plrust_lsp`, `pljava_lsp`, `plscala_lsp`, and
  `plpython_lsp` successfully.
- [x] Validated the modified workflow YAML.
- [x] Confirmed whitespace validation with `git diff --check`.
