# Python Deep Learning Wrapper Include Fix (v1.3.5)

## Summary
Resolved deep-learning wrapper include/signature mismatches that blocked Python extension builds.

## Why This Change Was Needed
Build-time header/signature inconsistencies prevented successful local in-place extension compilation.

## What Changed
1. Aligned deep-learning wrapper include usage and signatures to match the actual implementation.

## Validation
1. `test_bindings.py` passes (`9/9`).
2. Local extension build flow is unblocked in the bindings directory.
