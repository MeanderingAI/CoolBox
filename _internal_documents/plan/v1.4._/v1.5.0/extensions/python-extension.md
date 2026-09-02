# Python Extension Path Remodel (v1.5.0)

## Issue

Python packaging and purchase workflows mixed old `_libraries/python_bindings` paths with current deliverables layout.

## Root cause

Multiple entrypoints and docs still pointed to obsolete locations.

## Fix

Aligned to `_deliverables/libraries/bindings/python_bindings` in:

- `.github/workflows/generate-purchase-python.yaml`
- `.github/workflows/docs-publish.yaml` (python build artifact path)
- `setup.py`
- `pyproject.toml`
- `MANIFEST.in`
- `_local_build_pipeline/scripts/jobs/generate-purchase-python.sh`
- `_scripts/generate_docs_hub.sh`
- user-facing tutorials and site install snippets
- python bindings package metadata/readme

## Impact

Stabilizes Python purchase/docs pipelines and keeps install/build documentation consistent with the actual repository structure.
