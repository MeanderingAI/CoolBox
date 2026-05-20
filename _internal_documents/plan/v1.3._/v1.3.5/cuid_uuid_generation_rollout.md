# CUID UUID Generation Rollout (v1.3.5)

## Goal
Expose CUID generation across UUID delivery surfaces and add smoke-level validation for newly exported entry points.

## Implemented
- Added CUID generation to trekker UUID core:
  - `_deliverables/libraries/groups/trekker/MISC/uuid_generation/headers/uuid_generation.hpp`
  - `_deliverables/libraries/groups/trekker/MISC/uuid_generation/source/uuid_generation.cpp`
- Added UUID core tests for CUID format and non-equality:
  - `_deliverables/libraries/groups/trekker/MISC/uuid_generation/tests/test_uuid_generation.cpp`

## Surface Integration
- Added `cuid` option to secret generator CLI:
  - `_deliverables/apps/secret_gen_cli/src/main.cpp`
- Added C API for CUID:
  - Declaration: `_deliverables/libraries/bindings/c_bindings/include/coolbox/coolbox_c.h`
  - Bridge implementation: `_deliverables/libraries/bindings/c_bindings/src/uuid_generation_c_bridge.cpp`
- Added emscripten export for CUID:
  - `_deliverables/libraries/bindings/emscripten_bindings/uuid_generation_bindings.cpp`

## Smoke Validation Added
- C bindings smoke assertion for `coolbox_c_cuid()` output shape:
  - `_deliverables/libraries/bindings/c_bindings/tests/test_coolbox_c_bindings.c`
- Emscripten smoke check for CUID export wiring:
  - CTest registration in `_deliverables/libraries/bindings/emscripten_bindings/CMakeLists.txt`
  - Verification script `_deliverables/libraries/bindings/emscripten_bindings/tests/verify_uuid_bindings_cuid.cmake`

## Notes
- The emscripten smoke check validates source export wiring (`js_cuid`, `function("cuid", &js_cuid)`, and `generate_cuid()` usage) to ensure CI catches accidental removal/regression of the CUID binding entry point.
