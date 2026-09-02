# Python PDE SPDE Binding Source Inclusion Fix (v1.3.5)

## Summary
Added the missing PDE/SPDE binding source into Python extension build inputs.

## Why This Change Was Needed
The missing source file prevented expected modules from being compiled and imported correctly.

## What Changed
1. Included PDE/SPDE binding source in the Python extension build list.

## Validation
1. `test_bindings.py` passes (`9/9`).
2. Imports resolve for the intended PDE/SPDE Python binding surface.
