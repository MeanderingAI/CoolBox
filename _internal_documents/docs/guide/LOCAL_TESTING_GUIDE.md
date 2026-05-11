# Local Workflow Testing and Version Management

## Prerequisites for Local Testing

### Docker Desktop Required

Act requires Docker to run workflows locally. Before testing:

1. **Install Docker Desktop** (if not already installed):

   **Option A - Automated Installation (Recommended):**
   ```powershell
   .\_scripts\install_scripts\install_docker.ps1
   ```

   **Option B - Manual Installation:**
   - Download from: https://www.docker.com/products/docker-desktop
   - Run the installer and follow the setup wizard

2. **Start Docker Desktop** after installation:
   - Open Docker Desktop from the Start menu
   - Wait for it to fully start (whale icon in system tray should be steady)
   - Complete any first-time setup prompts

### Verify Docker is Running

```powershell
docker version
```

Expected output should show both Client and Server versions. If you see an error about the daemon not running, start Docker Desktop.

## Testing Workflows Locally with Act

### Quick Start

Test a workflow locally without triggering GitHub Actions:

```powershell
# Test the build-purchase-pipeline workflow
.\_scripts\test_workflow_locally.ps1 build-purchase-pipeline

# Test with dry-run (show what would happen without running)
.\_scripts\test_workflow_locally.ps1 build-purchase-pipeline -n

# Test a specific job only
.\_scripts\test_workflow_locally.ps1 build-purchase-pipeline -j build_libs

# List all jobs in a workflow
.\_scripts\test_workflow_locally.ps1 build-purchase-pipeline -l
```

### Common Act Options

- `-n` / `--dry-run` - Show what would run without executing
- `-l` / `--list` - List all jobs in the workflow
- `-j <job>` / `--job <job>` - Run a specific job
- `-s <secret>=<value>` - Set a secret value
- `--env <key>=<value>` - Set an environment variable
- `-v` / `--verbose` - Verbose output

### Example: Test with Secrets

```powershell
.\_scripts\test_workflow_locally.ps1 build-purchase-pipeline `
    -s GITHUB_TOKEN="your-token-here" `
    -j build_libs
```

## Branch-Based Version Workflow

### Creating a New Version Branch

```powershell
# Create v1.2.6 branch (without pushing)
.\_scripts\create_version_branch.ps1 -Version 1.2.6

# Create and tag v1.2.6
.\_scripts\create_version_branch.ps1 -Version v1.2.6 -Tag

# Create, tag, and push to remote
.\_scripts\create_version_branch.ps1 -Version v1.2.6 -Tag -Push
```

### Recommended Workflow

1. **Start a new version branch:**
   ```powershell
   .\_scripts\create_version_branch.ps1 -Version v1.2.6
   ```

2. **Make your changes and commit:**
   ```powershell
   git add .
   git commit -m "Your changes"
   ```

3. **Test locally with act (optional):**
   ```powershell
   .\_scripts\test_workflow_locally.ps1 build-purchase-pipeline -n
   ```

4. **Push the branch:**
   ```powershell
   git push -u origin v1.2.6
   ```

5. **When ready to release, tag and push:**
   ```powershell
   git tag v1.2.6
   git push origin v1.2.6
   ```
   This will trigger the build-purchase-pipeline workflow.

### Switching Between Branches

```powershell
# Switch to master
git checkout master

# Switch to a version branch
git checkout v1.2.5

# List all branches
git branch -a
```

## Troubleshooting

### Docker Issues

**Problem:** Docker not installed
- **Solution:** Run the installation script:
  ```powershell
  .\_scripts\install_scripts\install_docker.ps1
  ```
- Or download manually from https://www.docker.com/products/docker-desktop

**Problem:** "Couldn't get a valid docker connection" or "no DOCKER_HOST"
- **Solution:** Start Docker Desktop
- Verify with: `docker version`
- Wait for Docker to fully initialize before running act

**Problem:** "The system cannot find the file specified" (Docker pipe error)
- **Solution:** Docker Desktop is not running - start it and wait for initialization

**Problem:** Docker Desktop won't start
- Check Windows features: WSL2 or Hyper-V must be enabled
- Restart your computer after installing Docker
- Check Docker Desktop logs for errors

### Act Issues

**Problem:** Docker images are slow to download
- Use `--pull=false` to skip pulling if you have images cached

**Problem:** Workflow fails with permission errors
- Act runs in Docker with limited permissions
- Some workflows may need to be tested on GitHub directly

**Problem:** Secrets not working
- Use `-s SECRET_NAME=value` to pass secrets to act
- Or create `.secrets` file in repo root (don't commit this!)

### Workflow Triggers

The `build-purchase-pipeline` workflow triggers on:
- ✅ Tag pushes matching `v*` (e.g., v1.2.5)
- ✅ Manual workflow_dispatch
- ❌ Regular branch pushes (prevented by `branches-ignore: ['**']`)

### Checking What Would Trigger

```powershell
# See what tags exist
git tag -l

# See current branch and tags
git log --oneline --decorate -5
```

## Notes

- Act runs workflows in Docker containers
- Not all GitHub Actions features work with act (e.g., some caching)
- Large workflows may take significant time/resources locally
- For full testing, consider using the workflow_dispatch trigger on GitHub
