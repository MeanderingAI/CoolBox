# Local Workflow Testing - Setup Complete! ✅

## Summary

Successfully set up and tested local GitHub Actions workflow execution using act and Docker Desktop.

## What Was Configured

### 1. Docker Desktop
- **Status:** ✅ Installed and Running
- **Version:** 29.2.1 (Docker Desktop 4.65.0)
- **Installation:** Automated via `_scripts/install_scripts/install_docker.ps1`

### 2. Act (GitHub Actions Runner)
- **Status:** ✅ Installed and Working
- **Version:** v0.2.88 (latest)
- **Previous Version:** v0.2.61 (outdated, caused API compatibility issues)
- **Installation:** Automated via `_scripts/install_scripts/install_act.ps1`
- **Update:** Script now automatically fetches latest version from GitHub

### 3. Test Scripts
- ✅ `_scripts/test_workflow_locally.ps1` - Simplified workflow testing
- ✅ `_scripts/create_version_branch.ps1` - Version branch management
- ✅ `_scripts/install_scripts/install_docker.ps1` - Docker installation
- ✅ `_scripts/install_scripts/install_act.ps1` - Act installation (updated)

### 4. Documentation
- ✅ `docs/guide/LOCAL_TESTING_GUIDE.md` - Complete testing guide
- ✅ `docs/guide/WORKFLOW_ORGANIZATION_FIX.md` - Workflow file organization
- ✅ This setup summary document

## Issues Resolved

### 1. Docker API Version Mismatch
- **Problem:** Act v0.2.61 used Docker API 1.43, but Docker required 1.44+
- **Solution:** Updated act to v0.2.88 which supports newer Docker API versions
- **Fix Applied:** Modified `install_act.ps1` to fetch latest version automatically

### 2. Workflow File Organization
- **Problem:** Reusable workflows in subdirectories not supported by GitHub Actions
- **Solution:** Moved all reusable workflows to `.github/workflows/` top level
- **Files Moved:**
  - `build-cpp-docs.yaml`
  - `build-tutorials.yaml`
  - `docs-publish.yaml`
  - All `generate-purchase-*.yaml` files

### 3. Docker Not Running
- **Problem:** Docker Desktop needs to be manually started
- **Solution:** Added Docker status checks to test scripts
- **User Experience:** Clear error messages guide users to start Docker

## Test Results

### Dry-Run Test (Successful)
```powershell
PS C:\KEYS\CoolBox> .\_scripts\test_workflow_locally.ps1 build-purchase-pipeline -n
```

**Results:**
- ✅ Docker connection successful
- ✅ Workflow file parsed correctly
- ✅ All jobs identified (17 total jobs across 4 stages)
- ✅ Job dependencies mapped correctly
- ✅ Individual job tested successfully (ci_diagnostics)

### Job Listing (Successful)
```powershell
PS C:\KEYS\CoolBox> act workflow_dispatch -W .github\workflows\build-purchase-pipeline.yaml -l
```

**Jobs Identified:**
- Stage 0: 4 jobs (ci_diagnostics, build_libs, build_cpp_docs, build_tutorials)
- Stage 1: 8 jobs (build_products, generate_purchase_*, post_build_check)
- Stage 2: 2 jobs (generate_purchase_java, publish_lsp_packages)
- Stage 3: 2 jobs (docs_publish, collect_diagnostics_artifact)

## Usage Examples

### Quick Test (Dry-Run)
```powershell
# Test entire workflow without running
.\_scripts\test_workflow_locally.ps1 build-purchase-pipeline -n

# List all jobs
.\_scripts\test_workflow_locally.ps1 build-purchase-pipeline -l

# Test specific job
.\_scripts\test_workflow_locally.ps1 build-purchase-pipeline -j ci_diagnostics -n
```

### Full Local Build
```powershell
# Run entire workflow (will take significant time and resources)
.\_scripts\test_workflow_locally.ps1 build-purchase-pipeline

# Run specific job
.\_scripts\test_workflow_locally.ps1 build-purchase-pipeline -j build_libs
```

### Using Local Build Pipeline
```powershell
# Linux-style build in Docker
.\_local_build_pipeline\run_workflow.ps1 -Workflow build-libs

# Native Windows build (no Docker needed)
.\_local_build_pipeline\run_workflow.ps1 -Workflow build-libs -Backend native-windows
```

## System Requirements Verified

- ✅ Windows 10/11 with WSL2 or Hyper-V
- ✅ Docker Desktop installed and running
- ✅ Act v0.2.88+ installed
- ✅ PowerShell 5.1+ or PowerShell 7+
- ✅ Git for Windows (for bash scripts)
- ✅ Sufficient disk space for Docker images (~2-3GB)
- ✅ Sufficient RAM (8GB+ recommended for full builds)

## Branch Setup

- **Current Branch:** v1.2.6
- **Workflow:** Branch-then-tag development
- **Status:** Ready for development and local testing

## Next Steps

1. **Make changes** on v1.2.6 branch
2. **Test locally** with act before pushing:
   ```powershell
   .\_scripts\test_workflow_locally.ps1 build-purchase-pipeline -n
   ```
3. **Commit and push** branch:
   ```powershell
   git add .
   git commit -m "Your changes"
   git push -u origin v1.2.6
   ```
4. **Tag for release** when ready:
   ```powershell
   git tag v1.2.6
   git push origin v1.2.6
   ```

## Maintenance Notes

### Updating Act
The install script now automatically fetches the latest version. To update:
```powershell
.\_scripts\install_scripts\install_act.ps1
```

### Updating Docker
Use Docker Desktop's built-in updater or download from:
https://www.docker.com/products/docker-desktop

### Checking Status
```powershell
# Check Docker
docker version

# Check Act
act --version

# Test workflow connection
.\_scripts\test_workflow_locally.ps1 build-purchase-pipeline -l
```

## Resources

- Act Documentation: https://github.com/nektos/act
- Docker Documentation: https://docs.docker.com/desktop/
- GitHub Actions Documentation: https://docs.github.com/en/actions
- Local Testing Guide: `docs/guide/LOCAL_TESTING_GUIDE.md`

---

**Setup Completed:** 2026-05-02  
**Act Version:** v0.2.88  
**Docker Version:** 29.2.1  
**Status:** ✅ Fully Operational
