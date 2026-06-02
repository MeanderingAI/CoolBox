# Python LinearRegression fit Segfault Fix (v1.3.5)

## Summary
Fixed a segmentation fault in Python bindings when calling `LinearRegression.fit(...)`.

## Why This Change Was Needed
Model parameter storage could be used before initialization during SGD updates, which could lead to out-of-bounds access and process crashes.

## What Changed
1. Ensured model parameters are initialized before SGD update logic in the binding-backed implementation path.

## Validation
1. `test_bindings.py` passes (`9/9`).
2. `hmm_python_bindings_test` passes in CTest.
