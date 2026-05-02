# _scripts/test_workflow_locally.ps1
# Test GitHub Actions workflows locally using act
# Usage: .\test_workflow_locally.ps1 <workflow-name> [additional-act-args]

param(
    [Parameter(Mandatory=$true)]
    [string]$WorkflowName,

    [Parameter(ValueFromRemainingArguments=$true)]
    [string[]]$ActArgs
)

$ErrorActionPreference = "Stop"

# Get repository root
$RepoRoot = Split-Path $PSScriptRoot -Parent

# Remove .yaml/.yml extension if provided
$WorkflowName = $WorkflowName -replace '\.(yaml|yml)$', ''

# Add extension if not present
if ($WorkflowName -notmatch '\.(yaml|yml)$') {
    $WorkflowPath = Join-Path $RepoRoot ".github\workflows\$WorkflowName.yaml"
    if (-not (Test-Path $WorkflowPath)) {
        $WorkflowPath = Join-Path $RepoRoot ".github\workflows\$WorkflowName.yml"
    }
} else {
    $WorkflowPath = Join-Path $RepoRoot ".github\workflows\$WorkflowName"
}

if (-not (Test-Path $WorkflowPath)) {
    Write-Error "Workflow not found: $WorkflowPath"
    exit 1
}

Write-Host "Testing workflow: $WorkflowPath" -ForegroundColor Cyan
Write-Host ""

# Check if act is installed
if (-not (Get-Command act -ErrorAction SilentlyContinue)) {
    Write-Host "act is not installed. Installing now..." -ForegroundColor Yellow
    & "$PSScriptRoot\install_scripts\install_act.ps1"
}

# Check if Docker is running
try {
    $dockerVersion = docker version 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Host ""
        Write-Warning "Docker is not running. Act requires Docker Desktop to be running."
        Write-Host "Please start Docker Desktop and try again." -ForegroundColor Yellow
        Write-Host ""
        Write-Host "Alternatively, you can:" -ForegroundColor Cyan
        Write-Host "  - Run workflows on GitHub using workflow_dispatch" -ForegroundColor Cyan
        Write-Host "  - Use GitHub's 'Act' GitHub-hosted runner alternative" -ForegroundColor Cyan
        exit 1
    }
} catch {
    Write-Warning "Docker is not installed or not running."
    Write-Host "Act requires Docker Desktop to test workflows locally." -ForegroundColor Yellow
    exit 1
}

# Common act arguments for local testing
$CommonArgs = @(
    'workflow_dispatch',  # Trigger type
    '-W', $WorkflowPath,  # Workflow file
    '--container-architecture', 'linux/amd64',  # Architecture
    '-P', 'ubuntu-latest=catthehacker/ubuntu:act-latest',  # Use act-optimized images
    '-P', 'ubuntu-22.04=catthehacker/ubuntu:act-22.04',
    '-P', 'ubuntu-20.04=catthehacker/ubuntu:act-20.04'
)

# Add user-provided args
$AllArgs = $CommonArgs + $ActArgs

Write-Host "Running: act $($AllArgs -join ' ')" -ForegroundColor Green
Write-Host ""

# Run act
Push-Location $RepoRoot
try {
    & act @AllArgs
} finally {
    Pop-Location
}
