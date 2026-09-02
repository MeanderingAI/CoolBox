# D and Zig Bindings Scaffolds + GUI Detection (v1.3.5)

## Summary
Added first-party D and Zig binding package scaffolds and integrated them into the GUI Extensions inventory and toolchain status checks.

## Why This Change Was Needed
The bindings catalog did not include D or Zig, so these ecosystems were missing from extension discovery, build metadata, and toolchain readiness indicators in the GUI.

## What Changed
1. Added D binding package scaffold under `_deliverables/libraries/bindings/d_bindings`.
2. Added Zig binding package scaffold under `_deliverables/libraries/bindings/zig_bindings`.
3. Updated GUI extensions backend to detect `d_bindings` and `zig_bindings` and classify them as `dlang` and `zig` build types.
4. Added backend toolchain probes (`has_d_exec`, `has_zig_exec`) for card readiness/warning logic.
5. Updated Extensions frontend language icons, build labels, and warning hints for D and Zig.

## Validation
1. `GET /extensions` inventory includes both `d_bindings` and `zig_bindings`.
2. Toolchain status fields for D and Zig are present and used by the frontend warning/disable behavior.
3. Edited backend/frontend files report no diagnostics.
