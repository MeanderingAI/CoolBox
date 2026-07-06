# macOS Run and Debug Terminal Profiles (v1.3.5)

## Summary
Added debugger-extension-free Run and Debug profiles for macOS/Linux so secret generator CLI and GUI dashboard can run from VS Code without C++ debug adapters.

## Why This Change Was Needed
Existing launch entries were primarily Windows-oriented (`cppvsdbg`, `.exe` paths, PowerShell tasks). On macOS this caused Run and Debug failures before runtime output was visible.

## What Changed
1. Added `node-terminal` launch profiles for `secret_gen_cli` fixed and prompt argument modes.
2. Added `node-terminal` launch profiles for GUI dashboard start and stop (`make dashboard`, `make dashboard-kill`).
3. Added macOS build task for `secret_gen_cli` (`build-secret-gen-cli-macos`) to avoid PowerShell-based prelaunch failures.
4. Converted macOS-specific `secret_gen_cli` profiles from `cppdbg` to terminal launch commands.

## Validation
1. `build-secret-gen-cli-macos` task completes successfully.
2. Terminal launch profile `Run secret_gen_cli (Prompt Args, macOS/Linux)` runs successfully with different versions (`v7`, `v4`) and exits cleanly.
