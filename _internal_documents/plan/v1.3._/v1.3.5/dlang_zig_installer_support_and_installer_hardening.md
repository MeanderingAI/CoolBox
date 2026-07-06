# Dlang/Zig Installer Support + Installer Hardening (v1.3.5)

## Summary
Extended the master tool installer to support D and Zig and hardened the installer script so `--list`, `--check`, and install flows execute reliably.

## Why This Change Was Needed
The GUI Install Tools API accepted `dlang` and `zig` after backend updates, but installer-side support was incomplete and the installer script itself had structural/runtime issues that could block execution.

## What Changed
1. Added `install_dlang()` and `install_zig()` handlers in `_scripts/install_scripts/master_installer.py`.
2. Registered `dlang` and `zig` in the installer tool registry and status checks.
3. Repaired installer preamble/runtime reliability issues so the script executes cleanly:
   - Added missing imports and helper functions.
   - Fixed argument parsing behavior for `--list` and `--check`.
   - Fixed Python 3.9 compatibility issues and safety checks.
4. Kept platform-specific install paths aligned with existing installer behavior (brew/apt/dnf/pacman/winget where available).

## Validation
1. `python _scripts/install_scripts/master_installer.py --list` shows `dlang` and `zig`.
2. `python _scripts/install_scripts/master_installer.py --check` runs successfully and reports status for all tools.
3. Diagnostics on `master_installer.py` report no errors.
