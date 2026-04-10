# Tyst Framework

## Summary
- Added a new backage at `_libraries/backages/TOOLS/tyst_framework`.
- Implemented `tyst_framework` as a thin GTest-compatible wrapper rather than a separate test runtime.
- Added a companion `tyst_framework_main` target so tests can opt into the wrapper with a single link dependency.

## API Surface
- Header entry point: `tyst_framework.hpp`
- Namespace helpers: `tyst::framework::Test`, `tyst::framework::Environment`, `tyst::framework::init(...)`
- Macro aliases for GTest-style authoring:
  - `TYST_TEST`, `TYST_TEST_F`
  - `TYST_EXPECT_*` assertions
  - `TYST_ASSERT_*` assertions
  - `TYST_SKIP`, `TYST_SUCCEED`, `TYST_FAIL`

## Build Integration
- Added `_libraries/backages/TOOLS/tyst_framework/CMakeLists.txt`.
- Registered the new backage in `_libraries/CMakeLists.txt` independent of the parser and SQL build toggle.
- Kept the wrapper targets out of the install/export set so production package exports remain library-focused.

## Adoption Example
- Converted `_libraries/backages/SP/fourier_tranforms/tests/test_fourier_tranforms.cpp` from direct `gtest/gtest.h` usage to `tyst_framework.hpp`.
- Updated `_libraries/backages/SP/fourier_tranforms/CMakeLists.txt` so the Fourier test executable links `tyst_framework_main` instead of `GTest::gtest_main` directly.

## Result
- The repository now has a reusable testing backage that preserves GTest behavior while giving the project a local testing entry point and naming convention.