# Local Pipeline CMake Build Driver

## Summary
- Updated `_local_build_pipeline/scripts/jobs/build-libs.sh` to build through `cmake --build` instead of the repo-level `make build_libraries` wrapper.
- Updated the same job to run tests through `ctest --test-dir build --output-on-failure` instead of the repo-level `make test` wrapper.

## Rationale
- The Linux local pipeline already configures the project with CMake and then immediately falls back to a second build entrypoint by calling the top-level `Makefile`.
- Using `cmake --build` keeps the job on the same configured build graph and makes the pipeline script reflect the actual generator-driven build step more directly.
- Running tests through `ctest` keeps test execution aligned with the configured build tree instead of routing through a wrapper target that rebuilds first.

## Scope
- This change is limited to `_local_build_pipeline/scripts/jobs/build-libs.sh`.
- No changes were made to the top-level repo `Makefile`, Windows build flow, or native macOS pipeline in this revision.