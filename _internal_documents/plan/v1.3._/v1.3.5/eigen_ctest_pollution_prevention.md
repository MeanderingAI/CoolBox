# Eigen CTest Pollution Prevention (v1.3.5)

## Summary
Eigen dependency integration was adjusted so upstream Eigen tests are not registered in the project CTest suite.

## Why This Change Was Needed
Adding Eigen as a CMake subproject caused many upstream tests to appear as unsupported/not-run and forced non-zero CTest exits.

## What Changed
1. Switched Eigen integration to a headers-only population approach.
2. Avoided adding Eigen as a subdirectory in the project CMake graph.

## Validation
1. Project test registry is reduced to project-owned tests.
2. Full-suite `ctest --output-on-failure -j 8` passes (`36/36`).
3. `make test` passes with no CTest failures (`36/36`).
