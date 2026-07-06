# Root Python Repo Install Support

## Summary
- Added root-level Python packaging entry points so the repository can be installed directly with `pip install git+https://github.com/MeanderingAI/CoolBox.git`.
- Reused the existing `_libraries/python_bindings` build instead of duplicating the extension configuration in a second setup implementation.

## Problem
- The Python bindings already supported installation from the bindings subdirectory, but the repository root had no `pyproject.toml` or `setup.py` for Python packaging.
- That meant direct GitHub installs from the repository root could not work with plain pip commands.

## Files Added
- `pyproject.toml`
- `setup.py`
- `MANIFEST.in`

## Files Updated
- `README.md`
- `_libraries/python_bindings/README.md`

## Result
- Root-level pip installs now use the same metadata and package layout as the existing bindings package.
- The root `setup.py` delegates into `_libraries/python_bindings/setup.py`, so the extension build, vendored fallback logic, and package compatibility behavior remain centralized in one place.
- Users can install from GitHub with either:
  - `pip install git+https://github.com/MeanderingAI/CoolBox.git`
  - `pip install git+https://github.com/MeanderingAI/CoolBox.git#subdirectory=_libraries/python_bindings`