# v1.4.7 Actions Build Scripts Path Update

## Context

Build helper scripts were moved from:

- `_scripts/build_*.py`
- `_scripts/build_*.sh`

into:

- `_scripts/build_scripts/`

## CI impact

GitHub Actions workflows still referenced the old script paths, which can cause runtime failures such as:

- `No such file or directory` for documentation build commands.

## Fixes applied

### Workflow path updates

Updated:

- `.github/workflows/build-tutorials.yaml`
  - `bash ./_scripts/build_documentation.sh "$(pwd)"`
  - to `bash ./_scripts/build_scripts/build_documentation.sh "$(pwd)"`

- `.github/workflows/docs-publish.yaml`
  - `python ./_scripts/build_tags.py ...`
  - `python ./_scripts/build_publications.py ...`
  - `python ./_scripts/build_tutorials.py ...`
  - now all point to `./_scripts/build_scripts/...`

### Script internal path update

Updated:

- `_scripts/build_scripts/build_documentation.sh`

So it now calls:

- `${PROJECT_ROOT}/_scripts/build_scripts/build_tutorials.py`

instead of the old location.

### Git tracking / ignore update

Because `.gitignore` includes `build_*`, moved script names were still being ignored by basename pattern.

Added explicit unignore rules for:

- `_scripts/build_scripts/`
- `_scripts/build_scripts/**`

so moved build scripts can be tracked and included in CI checkouts.

## Validation

- Local invocation succeeded:
  - `bash ./_scripts/build_scripts/build_documentation.sh "$(pwd)"`
- Workflow references for docs scripts now point to the moved directory.
