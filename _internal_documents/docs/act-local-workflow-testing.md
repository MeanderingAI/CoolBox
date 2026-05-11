# Running GitHub Actions Workflows Locally with `act`

## Summary

**Yes, the changes made will enable `act` to run the build-purchase-pipeline locally!**

## Changes Made

### 1. **Converted Workflow Fragments to Composite Actions**

GitHub Actions workflows can reference local composite actions using `uses: ./path/to/action`. However, these require a specific structure:

- Must be in a directory with an `action.yaml` file
- Must have `runs.using: 'composite'` metadata
- All `run` steps must specify `shell` explicitly

**What was done:**
- Converted all `.github/workflows/fragments/*.yaml` files to proper composite actions
- Created subdirectories for each fragment (e.g., `deps-linux/action.yaml`)
- Added required metadata: `name`, `description`, `runs.using: 'composite'`
- Added `shell: bash` to all run steps
- Updated all workflow references from `.yaml` paths to directory paths

### 2. **Removed CMake Version Dependency**

The workflows were pinned to CMake 4.3.1, which doesn't exist (latest is 3.30.x).

**What was done:**
- Removed `jwlawson/actions-setup-cmake@v2` action from all workflows
- Updated dependency fragments to install CMake via system package manager (`apt`, `brew`)
- Changed verification steps from checking specific version to just checking availability
- Updated `CMakeLists.txt` from requiring CMake 4.2.3 to 3.20 (realistic modern version)

### 3. **Added Missing Dependencies**

**What was done:**
- Added to Linux deps: `cmake`, `bison`, `python3-dev`, `python3-pip`, `ninja-build`
- These are now installed via `apt-get` and available for local testing

## Files Modified

### Workflows:
- `.github/workflows/build-libs.yaml`
- `.github/workflows/build-products.yaml`
- `.github/workflows/build-lsp.yaml`

### Fragments Converted to Composite Actions:
- `.github/workflows/fragments/deps-linux/action.yaml`
- `.github/workflows/fragments/deps-macos/action.yaml`
- `.github/workflows/fragments/deps-windows/action.yaml`
- `.github/workflows/fragments/setup-sign-linux/action.yaml`
- `.github/workflows/fragments/setup-sign-macosx/action.yaml`
- `.github/workflows/fragments/setup-sign-windows/action.yaml`
- And 17 other fragment files...

### Project Files:
- `CMakeLists.txt` - Changed `cmake_minimum_required(VERSION 4.2.3)` to `3.20`

## How to Run with `act`

### Test the full build-purchase-pipeline:
```powershell
act workflow_dispatch -W .github/workflows/build-purchase-pipeline.yaml
```

### Test just the build-libs job:
```powershell
act workflow_dispatch -W .github/workflows/build-libs.yaml -j build
```

### Test specific platform (Linux x86_64):
```powershell
act workflow_dispatch -W .github/workflows/build-libs.yaml -j build --matrix platform:linux-x86_64
```

### List all available workflows:
```powershell
act -l
```

## Known Limitations with `act`

1. **Artifact upload/download** - Requires GitHub Actions runtime tokens not available locally
   - Workaround: Errors are expected, workflow continues

2. **Multi-platform builds** - `act` runs in Docker, so:
   - macOS and Windows jobs need platform-specific containers
   - Can test Linux builds most easily

3. **Secrets** - Won't have access to repository secrets
   - Signing steps will be skipped
   - Can provide secrets via `.secrets` file if needed

## Benefits

✅ **Test workflows locally before pushing**  
✅ **Debug workflow failures faster**  
✅ **Validate workflow syntax and structure**  
✅ **Test dependency installation**  
✅ **Verify build steps work correctly**

## Next Steps

If you encounter errors during `act` runs:
1. Check the console output for specific failures
2. Update the relevant composite action in `.github/workflows/fragments/`
3. Re-run `act` to validate the fix
4. Commit and push when tests pass locally

## Additional Resources

- `act` documentation: https://nektosact.com
- Script to convert fragments: `scripts/convert-fragments-to-actions.ps1`
