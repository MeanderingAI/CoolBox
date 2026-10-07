# CoolBox v3.1.3 Release Notes

## Windows build and test-runtime fixes

This plan records the Windows fixes implemented and checked on 2026-10-05.
Successful builds are verified; a passing full test suite is not yet verified.

### Makefile dispatch and failure reporting

- The top-level [Makefile](../../../Makefile) checks `OS=Windows_NT` before
  invoking `uname`. Native Windows builds no longer report
  `process_begin: CreateProcess(NULL, uname -s, ...) failed`.
- [Makefile.win](../../../Makefile.win) propagates CMake failures from
  `build_libraries`, preventing the signing step from proceeding after a
  failed build.

### Dependency configuration

- The charts stb dependency retains its pinned commit,
  `2c980bb59875b0d32144a71867fbdebb2f77cd20`.
- Removed shallow cloning and forced disconnected updates from that
  dependency declaration. CMake can fetch the pinned commit when it is
  absent locally instead of failing during `stb-populate`.
- CMake configuration completed successfully after this change.

### MSVC compilation compatibility

- Added explicit captures of local constants to the concurrent timer test
  worker lambda and Recording Studio's microphone-row bounds lambda.
- Enabled `_USE_MATH_DEFINES` privately for MSVC on the math tests, quantum
  simulator library, quantum simulator tests, and Gabor patches tests that
  use `M_PI`. Other compilers and downstream consumers are unaffected by
  these private definitions.
- The corrected targets, including both quantum test executables, compile
  successfully in Release configuration.

### Test runtime DLL staging

- The shared `coolbox_stage_test_runtime_dependencies` helper in
  [CMakeLists.txt](../../../CMakeLists.txt) uses `TARGET_RUNTIME_DLLS` on
  CMake 3.21 and newer. Dependencies are resolved at generation time,
  including linked targets declared later during configuration.
- Older CMake versions retain the existing configure-time collection path.
- Added the missing staging calls for `xml_screen_descriptor_parser_tests`
  and `music_sequencer_tests`.
- Rebuilt Windows simulation, XML parser, and optimization factory test
  targets with their linked DLLs staged beside the executables.
- Rebuilt the music sequencer test with `note_synthesis.dll` and
  `music_theory.dll` beside its executable. The staged `note_synthesis.dll`
  hash matches the original Release build output.

## Verification

- [x] Windows Makefile dispatch checked without the `uname` error.
- [x] CMake configuration and affected Release targets built successfully.
- [x] `make build_libraries` completed with exit code 0 after the shared
  runtime-staging fix.
- [x] The subsequent music sequencer staging change passed its targeted
  Release build and DLL hash check.
- [ ] Rerun the full Release test suite after resolving execution-policy
  blocks. The earlier run reported 8 failures out of 109 tests; that run
  is not post-fix verification.

## Deferred Windows Signing Policy

Deferred at the user's request. Do not change Windows security policy as
part of these build fixes.

- `0xc0e90002` is `STATUS_SYSTEM_INTEGRITY_POLICY_VIOLATION`, not evidence
  of DLL corruption. `certutil` decoded the status, and Windows Code
  Integrity events 3033 and 3077 confirmed the blocks.
- The blocking policy is `VerifiedAndReputableDesktop`, with policy GUID
  `{0283ac0f-fff1-49ae-ada1-8a933130cad6}`.
- Recorded blocks include unsigned `json.dll`, `motion_analysis.dll`,
  `video_filters.dll`, and the music theory and note synthesis test
  executables. Release copies of `json.dll` were byte-identical.
- [The signing script](../../../_scripts/sign_windows_artifacts.ps1)
  supports `COOLBOX_WINDOWS_SIGN_CERT_PATH`, but no certificate is
  configured. Signing is currently skipped by the build.
- Follow-up requires a code-signing certificate trusted by the enforced
  policy or an administrator-approved development environment. A
  self-signed certificate is not guaranteed to satisfy that policy.
- After approved signing or environment setup, rerun the affected tests
  and the full Release suite before marking Windows test validation
  complete.