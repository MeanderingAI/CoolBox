# Bower Shell Product

## Summary
- Added a reusable `bower_shell` library under `_libraries/backages/TOOLS/bower_shell`.
- Added a standalone `_Product/bower_shell` console application that drives the library through a REPL.
- Kept the shell engine operating-system independent by implementing commands and background jobs in standard C++ instead of delegating to platform shells.

## Library Design
- `tools::bower_shell::ShellSession` owns the current working directory, parses command lines, executes built-in commands, and tracks background jobs.
- Background work is simulated through shell-managed ticks, so hosts can call `tick()` from any event loop and embed the shell without tying it to a platform scheduler.
- The public API exposes command execution results, job snapshots, completed-job draining, and prompt generation for UI embedding.

## Commands
- Implemented `help`, `pwd`, `cd`, `ls`, `echo`, `cat`, `mkdir`, `touch`, `sleep`, `jobs`, and `wait`.
- Appending `&` launches supported commands into the background under the shell's own job queue.

## Product Integration
- Added `_Product/bower_shell/src/main.cpp` as a thin launcher around `ShellSession`.
- The standalone product prints completed background job notifications and can also execute a single command with `-c`.

## Embedding Direction
- The shell library is independent of stdin/stdout and owns no platform windowing concerns.
- Other products, including MStudio, can embed the shell by composing `ShellSession`, feeding user-entered commands into `execute()`, and polling `tick()` plus `take_completed_jobs()` from their UI loop.

## Documentation And Pipeline Follow-Up
- The docs portal should present `bower_shell` as the executable reference host for `bower_shell_lib`, with the generated dependency map showing that product-to-library relationship directly.
- The product page should keep emphasizing that the shell behavior is implemented in the reusable library, while the standalone product is only a thin launcher and REPL harness.
- Product build automation should compile `bower_shell` explicitly so library changes that preserve unit tests but break the shipping shell entry point are caught by CI.

## Release Packaging Follow-Up
- The standalone shell can now print prerelease/install guidance through `OS_GENERICS/installer_abstraction` when the release workflow compiles products with installer abstractions enabled.
- Ordinary local builds keep the normal REPL banner because the release-only packaging layer is not part of default makefile-driven builds.
- Packaged release artifacts should continue to frame `bower_shell` as a thin launcher around the reusable shell library rather than a platform-specific terminal host.