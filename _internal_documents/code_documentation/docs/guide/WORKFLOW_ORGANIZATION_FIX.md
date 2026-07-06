# Workflow File Organization Fix

## Issue
Act was failing with error:
```
Error: CreateFile C:\KEYS\CoolBox\.github\workflows\build-tutorials.yaml: The system cannot find the file specified.
```

## Root Cause
GitHub Actions requires **reusable workflows** (those called with `uses:` at the job level) to be at the top level of `.github/workflows/`. Subdirectories are not supported for reusable workflows.

## Files Moved

Moved from `documentation/` subdirectory to top level:
- ✅ `build-cpp-docs.yaml`
- ✅ `build-tutorials.yaml`  
- ✅ `docs-publish.yaml`

Previously moved from `generate_purchase/` subdirectory:
- ✅ `generate-purchase-python.yaml`
- ✅ `generate-purchase-js.yaml`
- ✅ `generate-purchase-go.yaml`
- ✅ `generate-purchase-c.yaml`
- ✅ `generate-purchase-java.yaml`
- ✅ `generate-purchase-r.yaml`
- ✅ `generate-purchase-rust.yaml`

## Remaining Subdirectories

### `.github/workflows/fragments/`
Contains workflow fragments (steps/composite actions). These files:
- Are **not** being used correctly (they should be proper composite actions with `action.yml`)
- Are being referenced with `- uses:` syntax (step-level, not job-level)
- Should be refactored into proper composite actions in `.github/actions/` directory
- **For now:** Can stay in subdirectories but won't work with act

### `.github/workflows/lsp/`
Contains LSP-specific workflows:
- May need to be moved to top level if they are reusable workflows
- Or converted to composite actions

### `.github/workflows/_disabled/`
Disabled workflows - can stay as-is

## Testing with Act

After moving files, act now works but requires:
1. **Docker Desktop must be running**
2. Workflow files must be at top level (fixed)

Updated script to check for Docker before running.

## Recommendations

1. **Short term:** Files moved, act should work with Docker running
2. **Long term:** Refactor `fragments/` to proper composite actions in `.github/actions/`
3. **Best practice:** Keep all reusable workflows at `.github/workflows/` top level
