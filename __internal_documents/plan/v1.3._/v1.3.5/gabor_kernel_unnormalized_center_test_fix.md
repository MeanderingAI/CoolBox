# Gabor Kernel Unnormalized Center Test Fix (v1.3.5)

## Summary
The Gabor kernel center-value test was updated to explicitly exercise unnormalized mode.

## Why This Change Was Needed
The test expected a center value of 1.0, which corresponds to unnormalized behavior, while default normalization changed the observed value.

## What Changed
1. Updated the center-value test call to pass `normalize=false` for the expected unnormalized result.

## Validation
1. Targeted test `GaborPatchesTests` passes.
2. Included in full-suite validation where `ctest --output-on-failure -j 8` passes (`36/36`).
