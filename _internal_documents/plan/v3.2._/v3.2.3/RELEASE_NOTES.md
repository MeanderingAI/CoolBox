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

