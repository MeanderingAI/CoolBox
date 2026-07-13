# JavaScript Extension Eigen Include Fix (v1.5.0)

## Issue

Emscripten JavaScript builds (for example `decision_tree_js`) failed with:

- `fatal error: 'Eigen/Dense' file not found`

## Root cause

`_deliverables/libraries/bindings/emscripten_bindings/CMakeLists.txt` used a single hardcoded Eigen path under `external/eigen/eigen-3.4.0`, which is not always present in CI jobs.

## Fix

Added Eigen include resolution fallback logic in emscripten bindings CMake:

1. `${COOLBOX_ROOT}/external/eigen/eigen-3.4.0`
2. `${COOLBOX_ROOT}/build/_deps/eigen-src`
3. `/usr/include/eigen3`
4. `/opt/homebrew/include/eigen3`
5. `/usr/local/include/eigen3`
6. `find_path(... Eigen/Dense ...)` fallback

If no candidate exists, configuration now fails with a clear fatal message.

## Impact

Allows JS bindings to resolve Eigen headers across vendored, fetched, and system-installed environments, removing the CI compile blocker.

## Follow-up workflow hardening

`generate-purchase-js` could still fail on fresh runners where Eigen was not preinstalled.

### Additional fix

Updated `.github/workflows/generate-purchase-js.yaml`:

- Linux dependency install now includes `libeigen3-dev`
- macOS dependency install now includes `eigen`
- updated stale prebuilt-artifact check from `build/_libraries` to accept current layout (`build/_deliverables`) with legacy fallback

### Result

Reduces environment sensitivity for Emscripten configure in JS purchase jobs and aligns the prebuilt artifact detection message with the current repository structure.

## Emscripten PDE/SPDE and Circuitry binding compile fixes

### Issue

`generate-purchase-js` Emscripten builds failed with:

- `fatal error: '_deliverables/libraries/groups/cool_car/MATH/pde_solver.hpp' file not found` in `pde_spde_bindings.cpp`
- `unknown type name 'Circuit'` / `unknown type name 'CircuitSolver'` in `circuitry_bindings.cpp`

### Root cause

- `pde_spde_bindings.cpp` used a repository-root-prefixed include path string that does not resolve from the Emscripten target include graph.
- Circuitry target was enabled while current circuitry public headers (`circuit.h`, `circuit_solver.h`) do not expose the API surface required by the binding implementation in this branch.

### Fix

Updated:

- `_deliverables/libraries/bindings/emscripten_bindings/pde_spde_bindings.cpp`
- `_deliverables/libraries/bindings/emscripten_bindings/CMakeLists.txt`

Changes made:

- Normalized PDE/SPDE includes to include-dir based headers:
	- `#include <pde_solver.hpp>`
	- `#include <spde.hpp>`
	- `#include <matrix_dense.h>`
- Temporarily disabled `circuitry_js` target declaration with an explicit note so JS matrix builds do not fail on non-public circuitry API mismatch.

### Impact

- Unblocks `pde_spde_js` compilation in Emscripten jobs.
- Removes deterministic `circuitry_js` compile failures until circuitry public headers are restored.

### Follow-up TODO: re-enable `circuitry_js`

1. Restore public API declarations in `circuitry/include/circuit.h` and `circuitry/include/circuit_solver.h` so `Circuit`, `CircuitSolver`, and `CircuitSolution` are fully available to bindings.
2. Verify `CircuitSolution` fields consumed by `_deliverables/libraries/bindings/emscripten_bindings/circuitry_bindings.cpp` (including `node_voltages` and any aggregate totals) match the current solver contract.
3. Re-enable the `add_emscripten_module(circuitry_js ...)` block in `_deliverables/libraries/bindings/emscripten_bindings/CMakeLists.txt`.
4. Validate `generate-purchase-js` Emscripten CI on Linux passes with both `pde_spde_js` and `circuitry_js` targets enabled.
