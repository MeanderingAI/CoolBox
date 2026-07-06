# Python HMM Bindings Repair (v1.3.5)

## Summary
Repaired HMM Python binding implementation/header issues and restored expected matrix API behavior.

## Why This Change Was Needed
Corrupted or inconsistent binding definitions caused API mismatch behavior and test instability.

## What Changed
1. Corrected HMM binding implementation and header alignment.
2. Restored expected matrix argument handling used by tests.

## Validation
1. `hmm_python_bindings_test` passes.
2. `test_bindings.py` passes (`9/9`).
