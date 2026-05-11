Adding a guide system in 

docs/guide

## Guide System Structure

Created comprehensive developer guides in `docs/guide/`:

### 1. Local Testing Guide (`LOCAL_TESTING_GUIDE.md`)
- Testing GitHub Actions workflows locally using act
- Branch-based version workflow management
- Prerequisites (Docker Desktop)
- Usage examples and troubleshooting

### 2. Workflow Organization Fix (`WORKFLOW_ORGANIZATION_FIX.md`)
- Documents GitHub Actions workflow file organization requirements
- Lists files moved from subdirectories to top level
- Explains reusable workflow limitations
- Recommendations for future improvements

## Supporting Scripts

### Installation Scripts (`_scripts/install_scripts/`)

#### `install_docker.ps1`
- Automated Docker Desktop installation for Windows
- Uses Winget (Windows Package Manager) when available
- Falls back to manual download instructions
- Verifies existing installations
- Checks if Docker is running

#### `install_act.ps1` (existing)
- Installs act (GitHub Actions runner)
- Used by local testing scripts

### Development Scripts (`_scripts/`)

#### `test_workflow_locally.ps1`
- Test GitHub Actions workflows locally with act
- Checks for Docker before running
- Provides helpful error messages
- Supports all act command-line options

#### `create_version_branch.ps1`
- Automated version branch creation
- Optionally creates tags
- Optionally pushes to remote
- Prevents duplicate branches/tags

## Workflow Improvements

### Branch-Based Versioning
- Moved from tag-only to branch-then-tag workflow
- Version branches (e.g., v1.2.6) for development
- Tags for releases
- Better tracking in IDE and GitHub

### Workflow File Organization
- Moved reusable workflows to `.github/workflows/` top level:
  - `build-cpp-docs.yaml`
  - `build-tutorials.yaml`
  - `docs-publish.yaml`
  - All `generate-purchase-*.yaml` files

### Workflow Triggers
- Fixed `build-purchase-pipeline.yaml` to only trigger on:
  - Tag pushes matching `v*`
  - Manual workflow_dispatch
- Prevented accidental triggers on branch pushes

## Usage Examples

### Quick Start - Test Workflow Locally
```powershell
# Install Docker (if needed)
.\_scripts\install_scripts\install_docker.ps1

# Test workflow without running
.\_scripts\test_workflow_locally.ps1 build-purchase-pipeline -n

# Test specific job
.\_scripts\test_workflow_locally.ps1 build-purchase-pipeline -j build_libs
```

### Quick Start - Create Version Branch
```powershell
# Create new version branch
.\_scripts\create_version_branch.ps1 -Version v1.2.7

# Create, tag, and push
.\_scripts\create_version_branch.ps1 -Version v1.2.7 -Tag -Push
```

## Benefits

1. **Better Developer Experience**
   - Clear documentation for common tasks
   - Automated scripts reduce manual steps
   - Troubleshooting guides for common issues

2. **Safer Workflow Testing**
   - Test workflows locally before pushing
   - Catch errors early
   - Understand workflow behavior

3. **Improved Version Management**
   - Consistent branch naming
   - Clear version tracking
   - Easy to see current version in IDE

4. **Reduced CI/CD Costs**
   - Test locally instead of triggering GitHub Actions
   - Fewer accidental workflow runs
   - Better control over when workflows trigger
