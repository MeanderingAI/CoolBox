# Bower Shell Const-Correctness Fix

## Summary
- Fixed the `bower_shell` build break caused by `ShellSession::build_background_job` being declared `const` while calling non-const command execution logic.

## Root Cause
- `build_background_job(const ParsedCommand&) const` delegated to `run_parsed_command(const ParsedCommand&)`.
- `run_parsed_command` is intentionally non-const because some commands can mutate shell-session state.
- That mismatch caused the compiler to reject the call in `bower_shell.cpp` with a discarded-qualifiers error.

## Code Changes
- Removed the `const` qualifier from `build_background_job` in:
  - `_libraries/backages/TOOLS/bower_shell/headers/bower_shell.hpp`
  - `_libraries/backages/TOOLS/bower_shell/source/bower_shell.cpp`

## Result
- The function contracts now reflect the real mutability of background job construction, and the reported CI compiler error path is resolved.