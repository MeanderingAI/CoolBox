# CoolBox v3.2.3 Release Notes

## Windows Test Runtime Reliability

Verified the Windows graphics segmentation-fault fixes and improved runtime
dependency staging for tests that previously timed out in Release CI.

### Graphics Segmentation-Fault Verification

The three graphics tests that previously crashed now build and pass locally in
Release mode:

- `ChartsTests`
- `GraphicsFontsTests`
- `GraphicsCanvasTests`

The Windows graphics libraries continue to use static linkage where required
to avoid passing C++ standard-library-owned objects across DLL boundaries.

### Timeout Fix

The six timed-out executables previously produced no test-framework output,
which indicated a failure during process startup or shutdown rather than slow
test bodies. The affected tests completed locally in milliseconds, reinforcing
that the CI behavior was specific to the MinGW runtime environment.

The shared
[`coolbox_stage_test_runtime_dependencies`](../../../../CMakeLists.txt)
function now copies the exact MinGW runtime DLLs from the configured compiler
directory beside each Windows test executable:

- `libstdc++-6.dll`
- `libwinpthread-1.dll`
- the matching `libgcc_s_*.dll`

CMake configuration fails explicitly if a required MinGW runtime DLL is
missing. This prevents tests from silently loading an incompatible runtime
from the GitHub runner's `PATH`.

Runtime staging was added to the affected file-browser, BowerShell, graphics,
OS-dialog, and CAD test targets. `WindowsSimulationTests` and
`DeepLearningScalingTests` already used the shared staging function and inherit
the improved MinGW behavior.

## Verification

- [x] Configured the project with `BUILD_TESTING=ON`.
- [x] Built all nine previously failing Release test targets.
- [x] Ran the nine tests through
  [`run_release_tests.ps1`](../../../../_scripts/run_release_tests.ps1) with a
  20-second per-test timeout.
- [x] All nine tests passed: 9/9 in 0.53 seconds.
- [x] `git diff --check` passed.
- [ ] Confirm the runtime-specific fix with a clean MinGW Windows CI run.

## JavaScript Binding Workflow

Fixed Windows JavaScript binding configuration for Emscripten SDK 6.0.12 and
newer. The workflow previously assumed the SDK always installed
`emcmake.bat`; when that wrapper was absent, configuration failed even though
SDK installation and activation had succeeded.

The Windows workflow now:

- invokes the canonical `emcmake.py` entry point with the SDK's embedded
  Python interpreter;
- loads the activated environment and exports its exact Python and Node paths
  to subsequent workflow steps;
- validates all required Emscripten paths before configuration; and
- checks native command exit codes after cloning, installation, activation,
  and CMake configuration so a failed command cannot be reported as a
  successful setup step.

See the
[JavaScript purchase workflow](../../../../.github/workflows/generate-purchase-js.yaml).

### Emscripten Binding Compilation

Fixed the macOS Emscripten build after `hidden_markov_model.cpp` failed to find
`viterbi_rank_convergence.h`. The standalone HMM JavaScript target now includes
the rank-convergence header directory already used by the native HMM target.

A complete Emscripten build exposed and resolved two later failures as well:

- PDE/SPDE bindings now use include paths relative to the include directories
  configured on their target instead of repository-root-relative includes.
- The obsolete circuitry JavaScript target was removed from the build. Its
  binding referenced `Circuit`, `CircuitSolver`, and solution fields that the
  current circuitry library does not provide, and the same CMake file already
  documented that this binding must remain disabled until the library API is
  repaired.

The complete Emscripten bindings project configured and built successfully
after these changes.

## Python Binding Workflow

Fixed Windows Python packaging configuration after the workflow installed GSL
through vcpkg but configured CMake without the vcpkg toolchain. The same step
also ran under Git Bash, whose `pkg-config` resolved to a broken Strawberry
Perl script, and Bison was not installed.

The Windows Python workflow now:

- installs and verifies the x64 Eigen, SQLite, and GSL vcpkg packages;
- installs and verifies WinFlexBison;
- exports the resolved vcpkg root, toolchain, installed prefix, target triplet,
  and Bison executable;
- runs Windows CMake configuration through PowerShell instead of Git Bash,
  avoiding the unrelated Strawberry Perl `pkg-config`; and
- passes the same explicit dependency arguments to both the test-enabled
  native build and the native libraries used for Python packaging.

Native command failures are now reported immediately instead of continuing
with missing dependencies.

See the
[Python purchase workflow](../../../../.github/workflows/generate-purchase-python.yaml).

### Linux/macOS Build Scope

The Ubuntu Python job was also spending several minutes configuring and
building the entire CoolBox C++ tree with tests and binaries enabled. That
build was not required by the Python package: Linux and macOS explicitly set
`COOLBOX_PYTHON_FORCE_VENDOR_SOURCES=1`, and the package already falls back to
its vendored chart and wave-generator sources.

The non-Windows workflow now performs a lightweight check for the Python
binding setup and required vendored sources, then proceeds directly to the
Python extension build. This avoids the redundant top-level build and the
observed SIGTERM (`exit code 143`) after the runner terminated the long build.

### LSP Image Generation

LSP artifact and Docker-image workflows now build the required LSP executable
targets explicitly and stage them from CMake's actual output directory,
`build/_deliverables/apps/lsp`. The previous workflows searched
`build/apps/lsp` and the build root, so successful builds still produced empty
artifact directories and all image inputs were reported missing.

The local image fallbacks also disable unrelated products and tests and build
only the LSP targets required by each image job. Missing build outputs now fail
while staging instead of being hidden by permissive copy commands.
