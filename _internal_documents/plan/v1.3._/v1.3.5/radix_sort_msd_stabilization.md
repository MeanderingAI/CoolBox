# Radix Sort MSD Stabilization (v1.3.5)

## Summary
`radix_sort_msd` behavior was stabilized for large randomized `uint64_t` datasets.

## Why This Change Was Needed
The previous MSD path could produce unsorted output on large random inputs in stress scenarios.

## What Changed
1. Updated `radix_sort_msd` to delegate to the stable LSD implementation path.

## Validation
1. Targeted test `SortsTests` passes.
2. Included in full-suite validation where `ctest --output-on-failure -j 8` passes (`36/36`).
