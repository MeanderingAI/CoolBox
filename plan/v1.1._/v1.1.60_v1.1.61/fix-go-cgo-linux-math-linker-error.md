# Fix Go/cgo Linux linker error: add -lm to LDFLAGS

## Problem
On Linux, Go bindings using cgo failed to link math functions (e.g., exp, log) with errors like:

    undefined reference to symbol 'exp@@GLIBC_2.29'

This is because the math library (-lm) was not explicitly linked in the Go cgo LDFLAGS, even though CMake linked it for the bridge library.

## Solution
Explicitly add `-lm` to the `#cgo linux LDFLAGS` in all Go binding source files that use the C++ bridge:

- _libraries/go_bindings/bindings.go
- _libraries/go_bindings/decision_tree.go
- _libraries/go_bindings/bayesian_network.go
- _libraries/go_bindings/gui.go

### Example change:

    -#cgo linux LDFLAGS: -lstdc++ -L${SRCDIR}/cbridge/build -lcoolboxbridge
    +#cgo linux LDFLAGS: -lstdc++ -lm -L${SRCDIR}/cbridge/build -lcoolboxbridge

## Rationale
Go/cgo does not automatically propagate static library dependencies from CMake. The math library must be explicitly listed in the Go source for the linker to resolve math symbols.

## Status
- [x] Patch applied to all relevant Go binding files.
- [x] Linux build should now succeed without math linker errors.

---

**Documented by GitHub Copilot, 2026-04-18**
