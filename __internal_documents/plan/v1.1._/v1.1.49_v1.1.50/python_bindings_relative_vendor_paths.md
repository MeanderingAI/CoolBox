# Python Bindings Relative Vendor Paths

## Summary
- Fixed the Python bindings packaging flow so Linux and macOS purchase-generation no longer write absolute vendored source paths into setuptools metadata.
- Normalized vendored fallback source entries to `setup.py`-relative POSIX paths before they are appended to the extension source list.
- Hardened the path normalizer so macOS runner path variations do not fall back to absolute vendored source paths.

## Problem
- The GitHub Actions `generate_purchase_python` workflow forces vendored source fallback on non-Windows runners.
- In that fallback path, `_libraries/python_bindings/setup.py` appended absolute filesystem paths for vendored `.cpp` files into `source_files`.
- When `python -m build` generated `ml_toolbox.egg-info/SOURCES.txt`, setuptools recorded those absolute paths.
- Linux packaging rejects absolute source entries and failed with `setup script specifies an absolute path` for `vendor_src/GRAPHICS/charts/source/graphics.cpp`.
- A follow-up macOS release run still failed because the first relative-path helper used `resolve()`, and that could diverge from the runner's lexical workspace path, causing the helper to return an absolute vendored source path again.

## Files Updated
- `_libraries/python_bindings/setup.py`
- `_libraries/python_bindings/ml_toolbox.egg-info/SOURCES.txt`

## Change
- Added a helper in `_libraries/python_bindings/setup.py` to convert vendored fallback source paths into paths relative to the Python bindings package root.
- Updated fallback source handling to append those relative paths instead of resolved absolute paths.
- Reworked the helper to normalize paths lexically and fall back to `os.path.relpath(...)` instead of relying on `Path.resolve()`, which makes the conversion robust on macOS runners.
- Removed stale absolute vendored source entries from the checked-in `ml_toolbox.egg-info/SOURCES.txt` so repository state matches the corrected packaging behavior.

## Result
- The `generate_purchase_python` Linux and macOS packaging paths now keep vendored fallback sources manifest-safe.
- The same correction also applies to the local `_local_build_pipeline/scripts/jobs/generate-purchase-python.sh` flow because it uses the same Python bindings build logic.