# _scripts/create_version_branch.ps1
# Creates a new version branch and optionally tags it
# Usage: .\create_version_branch.ps1 -Version <version> [-Tag] [-Push]

param(
    [Parameter(Mandatory=$true)]
    [string]$Version,

    [switch]$Tag,
    [switch]$Push
)

$ErrorActionPreference = "Stop"

# Ensure version starts with 'v'
if ($Version -notmatch '^v') {
    $Version = "v$Version"
}

Write-Host "Creating version branch: $Version" -ForegroundColor Cyan

# Check if branch already exists locally
$localBranches = git branch --list $Version
if ($localBranches) {
    Write-Error "Branch $Version already exists locally. Use 'git checkout $Version' to switch to it."
    exit 1
}

# Check if branch exists on remote
$remoteBranches = git branch -r --list "origin/$Version"
if ($remoteBranches) {
    Write-Host "Branch exists on remote. Checking out..." -ForegroundColor Yellow
    git checkout -b $Version "origin/$Version"
} else {
    # Create new branch from current HEAD
    Write-Host "Creating new branch from current HEAD..." -ForegroundColor Green
    git checkout -b $Version
}

if ($Tag) {
    Write-Host "Creating tag: $Version" -ForegroundColor Cyan

    # Check if tag exists
    $existingTag = git tag -l $Version
    if ($existingTag) {
        Write-Warning "Tag $Version already exists locally."
        $response = Read-Host "Delete and recreate? (y/N)"
        if ($response -eq 'y' -or $response -eq 'Y') {
            git tag -d $Version
            git tag $Version
        }
    } else {
        git tag $Version
    }
}

if ($Push) {
    Write-Host "Pushing branch to remote..." -ForegroundColor Cyan
    git push -u origin $Version

    if ($Tag) {
        Write-Host "Pushing tag to remote..." -ForegroundColor Cyan
        git push origin $Version
    }
}

Write-Host ""
Write-Host "Success! You are now on branch: $Version" -ForegroundColor Green
if (-not $Push) {
    Write-Host "To push to remote, run: git push -u origin $Version" -ForegroundColor Yellow
}
