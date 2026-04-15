# Validation Results (v1.1.46 -> v1.1.47)

## Static Validation
- Reviewed the macOS Cocoa runtime path in `_libraries/backages/GRAPHICS/full_application_window/source/full_application_window.cpp`.
- Confirmed the failing `NSInteger` and `NSUInteger` references were used only as Objective-C runtime scalar arguments and could be replaced by ABI-compatible C++ integer aliases.
- Reviewed the runner feasibility question against the repository's existing Linux Docker-based local pipeline and documented the resulting platform split in `runner_platform_strategy.md`.
- Reviewed the existing `_local_build_pipeline` dispatcher, Windows PowerShell wrapper, and job-script layout to turn the runner strategy into a concrete repository implementation plan in `runner_platform_implementation.md`.
- Reviewed the existing purchase-generation workflows and local packaging scripts to identify missing V/C3 coverage and output-name drift across bindings.
- Confirmed that the missing V and C3 purchase-generation paths could be implemented by packaging the binding sources together with the existing C binding headers and native library artifacts.
- Confirmed that local purchase-package naming could be normalized safely through shared helpers in `_local_build_pipeline/scripts/jobs/common.sh` without changing the binding API surface.
- Confirmed that the GitHub purchase workflows could also be aligned to emit the same manifest/checksum metadata and platform-qualified asset names with only packaging-step changes.
- Confirmed that the repeated GitHub workflow asset-finalization logic could be factored into a composite action without changing the package staging logic for each language.

## Editor Diagnostics
- Editor diagnostics reported no errors in the updated `full_application_window.cpp` translation unit after the scalar-type fix.
- Editor diagnostics reported no errors in the new release-plan documents.
- Editor diagnostics reported no errors in the added V/C3 purchase workflows, the new local purchase job scripts, or the shared packaging-helper changes.
- Editor diagnostics reported no errors in the updated GitHub purchase workflows after the manifest/checksum and asset-name alignment changes.
- Editor diagnostics reported no errors in the new composite action or the enriched local manifest helper.

## Local Execution
- No macOS build was available in the current workspace to re-run the affected compile step end to end.
- No native Windows or macOS container-runner implementation was added in this change; this release-plan entry records the feasible strategy and platform constraints instead.
- The new V/C3 purchase-generation paths were added as code and workflow definitions, but they were not executed end to end in this workspace.
- The normalized asset contract now covers both the local `generate-purchase-*` scripts and the GitHub purchase workflows, but the updated GitHub workflows were not executed end to end in this workspace.
- The composite action and enriched manifest format were validated statically only in this workspace.
- Docker-local `build-libs.yaml` was executed successfully through `_local_build_pipeline`, and `_local_build_pipeline/out/build-libs/docker-exit.txt` recorded `EXIT=0`.
- Focused Windows-host LSP validation had already completed successfully earlier in this cycle for both `plvlang_lsp` and `plc3_lsp` through the dedicated PowerShell wrapper path.
- Native Windows `build-libs` verification exposed and then fixed two runner issues during this session:
	- the PowerShell wrapper incorrectly preferred the WSL bash launcher instead of Git Bash
	- the native Windows runner resolved the repo root incorrectly and finalized its transcript in the wrong order
- Native Windows `build-libs` verification was then hardened further by switching to an isolated per-run build directory under `_local_build_pipeline/tmp/`, which removed the previous locked-file cleanup failure against the shared repository `build/` tree.
- After that hardening pass, review of the current source tree confirmed that `quiche` was no longer required for the normal build: the in-tree `quic_transport` target is implemented entirely in C++ and top-level CMake already labels external QUIC/HTTP3 dependency support as disabled.
- The remaining legacy `quiche` wiring was then removed from the repository path entirely by deleting the top-level opt-in, the external dependency fetch block, the legacy helper script, and the submodule declaration.
- The remaining on-disk legacy `quiche` trees were then removed from both `external/quiche/` and the top-level `quiche/` directory so the source tree matches the new dependency state.
- A direct Windows configure probe after disabling that legacy path no longer reproduced the previous `quiche` population failure, which narrowed the remaining native-Windows work to whatever configure/build issues remain after the unused legacy dependency wiring is gone.
- The native Windows runner also now records failures correctly in its staged log, rather than incorrectly writing `EXIT_STATUS=0` after a configure failure.
- Native Windows `build-libs` verification then exposed a second Windows-host-only failure in MSBuild tracking output: deep generated build paths under `_local_build_pipeline/tmp/...` caused `.tlog` directory-not-found failures such as `MSB6003` and `MSB3491` for targets including `cryptocurrency_utils_tests` and `dimensionality_reduction`.
- That host-path issue was fixed by shortening the native Windows build root from the repository tree to `%TEMP%\coolbox-lbp\...`, which keeps the generated Visual Studio tracking paths short enough for MSBuild to operate normally.
- A follow-up native Windows rerun after the short-path change progressed past the previous `.tlog` failures and successfully built targets that had previously failed under the deeper path, including `cryptocurrency_utils_tests`.
- The latest short-path native Windows rerun was still actively compiling and linking additional targets at the end of this verification window, so the path-depth regression is considered fixed even though the full end-to-end job had not yet completed within this session.
- A complete all-workflows local verification sweep was attempted, but the current editor terminal integration did not provide reliable staged results for every Docker-local workflow in this session, so only the explicitly observed successes and failures are recorded here.

## Result
- The immediate macOS compile blocker in the Cocoa runtime path has a targeted source fix.
- The repository now also has an explicit platform-runner strategy note plus a concrete implementation plan for how `.github/workflows` and `_local_build_pipeline` can evolve toward Linux Docker, Windows native, and macOS native backend parity.
- The repository now also has dedicated purchase-generation coverage for V and C3 plus a normalized local release-asset naming contract across the binding purchase jobs.
- GitHub and local purchase generation now share the same manifest/checksum metadata expectation, and the CI asset names are aligned with the normalized platform-qualified naming convention across the binding languages.
- The asset metadata generation logic is now centralized for GitHub purchase workflows, and the manifest content is rich enough to trace release assets back to repository state and workflow execution context.
- The local pipeline verification work also hardened the Windows runner path by fixing bash selection and native-runner path/log-finalization bugs, even though one Windows cleanup lock issue still remains before full native parity can be claimed.
- The local pipeline verification work also hardened the Windows runner path enough to move the remaining blocker from runner mechanics to an actual Windows dependency-configure failure in `quiche`, which is a more actionable next-stage problem.
- The local pipeline verification work also established that `quiche` was a stale dependency path for the current codebase and removed the related repository wiring, which simplifies Windows-native validation and better matches the repository's current internal HTTP/3 implementation.
- The local pipeline verification work also identified and fixed a second native-Windows-only path-depth problem in MSBuild by moving the isolated build root to a short temp path, allowing the Windows-native rerun to advance through targets that had previously failed during tracked-output generation.