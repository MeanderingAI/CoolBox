# Extension Build Runner Restore + D/Zig Build Support (v1.3.5)

## Summary
Restored the missing extension build runner script used by the GUI and added build-path support for D and Zig bindings.

## Why This Change Was Needed
The GUI Extensions build endpoint calls `_scripts/build_scripts/build_extensions.py`. That script was missing in the repo, causing all extension build actions to fail regardless of language.

## What Changed
1. Added `_scripts/build_scripts/build_extensions.py` with expected CLI behavior:
   - Build all bindings
   - Build a single binding
   - `--list`
   - `--target` for cmake-based builds
2. Implemented build dispatch for existing binding types (`cmake`, `emscripten`, `cargo`, `go`, `maven`, `npm`, `python`, `vlang`, `c3`, `r`, `postgres`).
3. Added `dlang` build handling via `dub build` (with compiler fallback).
4. Added `zig` build handling via `zig build`.
5. Preserved line-buffered output streaming semantics expected by the GUI build log view.

## Validation
1. `python _scripts/build_scripts/build_extensions.py --list` succeeds and reports `dlang` and `zig` build types.
2. `python _scripts/build_scripts/build_extensions.py postgres_bindings` succeeds as a safe smoke path.
3. GUI build endpoint now has a real script target at the expected path.
