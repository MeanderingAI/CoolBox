# Tyst Framework Linux fabs Compatibility

## Summary
- Updated `_libraries/packages/TOOLS/tyst_framework/headers/tyst_framework.hpp` so Linux builds use `std::fabs` for `long double` absolute-value comparisons.
- Preserved the prior `std::fabsl` path for non-Linux builds, matching the behavior that was already working on Windows and macOS.

## Rationale
- The Linux CI toolchain reported that `std::fabsl` was not available as a member of `std` in the failing build environment.
- The same header already compiled on the other supported platforms, so the smallest safe change was to make the absolute-value call platform-specific rather than changing every platform to the Linux fallback.

## Implementation Details
- Added a small internal helper, `absolute_long_double`, in the `tyst::framework::detail` section of the header.
- On `__linux__`, the helper calls `std::fabs(value)`.
- On other platforms, the helper calls `std::fabsl(value)`.
- `compare_near` now uses that helper for both the difference and tolerance calculations.

## Scope
- This change is limited to the floating-point comparison helper used by the Tyst test framework.
- No test semantics were changed beyond selecting the platform-compatible standard-library entry point.