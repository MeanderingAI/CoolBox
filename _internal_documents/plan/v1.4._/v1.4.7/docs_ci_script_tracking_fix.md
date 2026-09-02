# v1.4.7 Docs CI Script Tracking Fix

## Issue

GitHub Actions `build_tutorials` job failed with:

- `bash: ./_scripts/build_documentation.sh: No such file or directory`
- exit code `127`

Even though the script existed locally, it was not present in the runner checkout because it was ignored by git.

## Root cause

The repository ignore pattern in `.gitignore`:

- `build_*`

also matched script filenames under `_scripts/`:

- `_scripts/build_documentation.sh`
- `_scripts/build_tutorials.py`
- `_scripts/build_tags.py`

As a result, these files were not tracked and were missing in CI checkouts.

## Fix

Updated `.gitignore` to explicitly unignore these script files:

- `!_scripts/build_documentation.sh`
- `!_scripts/build_tutorials.py`
- `!_scripts/build_tags.py`

## Result

The scripts are now addable/trackable in git and will be available to GitHub Actions after commit/push.

## Follow-up required

1. Commit `.gitignore` plus the three script files.
2. Push the branch/tag used by the workflow.
3. Re-run `build_tutorials` workflow.
