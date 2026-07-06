# Matrix Profile Tolerance Adjustment (v1.3.5)

## Summary
A self-distance assertion in matrix profile tests was adjusted to a numerically stable tolerance.

## Why This Change Was Needed
Floating-point precision noise produced tiny non-zero values around expected zeros, causing false negatives in strict checks.

## What Changed
1. Relaxed the near-zero expectation tolerance for the self-distance assertion.

## Validation
1. Targeted test `MatrixProfileTests` passes.
2. Included in full-suite validation where `ctest --output-on-failure -j 8` passes (`36/36`).
