# CP Decomposition ALS Khatri-Rao Fix (v1.3.5)

## Summary
ALS factor updates in CP decomposition were corrected to use Khatri-Rao products instead of full Kronecker products.

## Why This Change Was Needed
The previous update path produced rank-dependent dimension issues during matrix multiplications and could trigger assertions in Eigen-backed tests.

## What Changed
1. Added and used Khatri-Rao product computation in CP ALS update steps.
2. Replaced the previous Kronecker-based update multipliers in the factor update path.

## Validation
1. Targeted test `CandecompParafacTests` passes.
2. Included in full-suite validation where `ctest --output-on-failure -j 8` passes (`36/36`).
