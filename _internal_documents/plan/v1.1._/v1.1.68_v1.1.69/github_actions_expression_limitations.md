
# GitHub Actions Expression Limitations: `exists()` Error

## Problem
GitHub Actions workflow expressions do **not** support arbitrary functions like `exists()`. Only a limited set of [context and expression functions](https://docs.github.com/en/actions/learn-github-actions/expressions) are allowed. Using `exists('D:\a\_temp\msys64')` in an `if:` condition causes a YAML parsing error:

```
Unrecognized function: 'exists'. Located at position ...
```

## Solution
- **You cannot check for file existence in workflow `if:` expressions.**
- Instead, always run the MSYS2 setup step, or use a shell step to check and skip logic at runtime.
- The recommended pattern is to:
	1. Remove the `exists()` logic from the `if:` expressions.
	2. Use a shell script step to check for the directory and skip or echo as needed.

## Patch Plan
- Remove the `exists()` and `!exists()` logic from workflow `if:` conditions.
- Always run the MSYS2 setup step for ARM64, or use a shell script to check and skip at runtime.
- Document this limitation and the fix in the plan.

---
**Date:** 2026-04-19
**Author:** GitHub Copilot
