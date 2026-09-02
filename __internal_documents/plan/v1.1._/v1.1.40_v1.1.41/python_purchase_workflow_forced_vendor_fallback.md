# Python Purchase Workflow Forced Vendor Fallback

## Summary
- Hardened the Python purchase workflow so non-Windows CI builds deterministically use vendored source fallbacks for `charts` and `wave_generator_utils`.
- Removed the remaining path where Linux or macOS wheel builds could still reach the final link step with unresolved `-lcharts` or `-lwave_generator_utils` flags.

## Problem
- The purchase workflow was already passing `COOLBOX_LIB_DIR`, and `setup.py` already had recursive native library discovery plus vendored fallback logic.
- Even with that logic present, the Linux CI failure still showed the extension build invoking the linker with:
  - `/usr/bin/ld: cannot find -lcharts`
  - `/usr/bin/ld: cannot find -lwave_generator_utils`
- That meant the fallback path was not being activated deterministically for the purchase job when staged native artifacts were incomplete or absent.

## Files Updated
- `.github/workflows/generate-purchase-python.yaml`
- `_libraries/python_bindings/setup.py`

## Result
- The purchase workflow now sets `COOLBOX_PYTHON_FORCE_VENDOR_SOURCES=1` on non-Windows runners.
- `setup.py` now recognizes that flag and skips staged-library resolution for that mode, allowing vendored `graphics.cpp` and `wave_generator.cpp` sources to satisfy the extension build directly.
- When forced-vendor mode is active, the extension no longer keeps unresolved `charts` or `wave_generator_utils` names in the final `libraries=` list, which is the root cause of the reported linker failure.