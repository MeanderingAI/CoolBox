param(
    [Parameter(Mandatory = $true)]
    [string]$PfxPath,

    [Parameter(Mandatory = $true)]
    [string]$PfxPassword,

    [string]$Repository = '',

    [string]$TimestampUrl = 'http://timestamp.digicert.com',

    [string]$Workflow = 'build-products.yaml',

    [switch]$TriggerWorkflow,

    [switch]$PrintOnly
)

$ErrorActionPreference = 'Stop'

function Get-RepoRoot {
    $scriptDirectory = Split-Path -Parent $PSCommandPath
    return Split-Path -Parent $scriptDirectory
}

function Resolve-RepositorySlug {
    param(
        [string]$ExplicitRepository,
        [string]$RepoRoot
    )

    if ($ExplicitRepository) {
        return $ExplicitRepository
    }

    $originUrl = git -C $RepoRoot remote get-url origin 2>$null
    if (-not $originUrl) {
        throw 'Unable to resolve the GitHub repository from git remote origin. Pass -Repository owner/repo explicitly.'
    }

    $match = [regex]::Match($originUrl.Trim(), 'github\.com[:/](?<slug>[^\s]+?)(?:\.git)?$')
    if (-not $match.Success) {
        throw "Unable to parse a GitHub repository slug from origin URL: $originUrl"
    }

    return $match.Groups['slug'].Value
}

function Assert-GitHubCliReady {
    $gh = Get-Command gh -ErrorAction SilentlyContinue
    if (-not $gh) {
        $candidatePaths = @(
            (Join-Path $env:ProgramFiles 'GitHub CLI\gh.exe'),
            (Join-Path $env:LOCALAPPDATA 'Programs\GitHub CLI\gh.exe')
        ) | Where-Object { $_ -and (Test-Path $_) }

        if ($candidatePaths.Count -gt 0) {
            $gh = [pscustomobject]@{ Source = $candidatePaths[0] }
        }
    }

    if (-not $gh) {
        throw 'GitHub CLI (gh) is required. Install it from https://cli.github.com/ and authenticate with gh auth login.'
    }

    & $gh.Source auth status 1>$null 2>$null
    if ($LASTEXITCODE -ne 0) {
        throw 'GitHub CLI is installed but not authenticated. Run gh auth login first.'
    }

    return $gh.Source
}

$repoRoot = Get-RepoRoot
$resolvedPfxPath = $PfxPath
if (-not $PrintOnly) {
    $resolvedPfxPath = Resolve-Path $PfxPath
    if (-not (Test-Path $resolvedPfxPath)) {
        throw "PFX file not found: $PfxPath"
    }
}

$repositorySlug = Resolve-RepositorySlug -ExplicitRepository $Repository -RepoRoot $repoRoot
$pfxBase64 = ''
if (-not $PrintOnly) {
    $pfxBytes = [System.IO.File]::ReadAllBytes($resolvedPfxPath)
    $pfxBase64 = [Convert]::ToBase64String($pfxBytes)
}

Write-Host "[publish_windows_signing_secrets] Repository: $repositorySlug"
Write-Host "[publish_windows_signing_secrets] PFX path: $resolvedPfxPath"
Write-Host "[publish_windows_signing_secrets] Workflow: $Workflow"
Write-Host "[publish_windows_signing_secrets] Trigger workflow: $TriggerWorkflow"

if ($PrintOnly) {
    Write-Host '[publish_windows_signing_secrets] PrintOnly enabled; no secrets or workflows will be modified.'
    Write-Host '[publish_windows_signing_secrets] Secrets that would be updated:'
    Write-Host '  - COOLBOX_WINDOWS_SIGN_CERT_BASE64'
    Write-Host '  - COOLBOX_WINDOWS_SIGN_CERT_PASSWORD'
    Write-Host '  - COOLBOX_WINDOWS_SIGN_TIMESTAMP_URL'
    return
}

$ghExecutable = Assert-GitHubCliReady

$pfxBase64 | & $ghExecutable secret set COOLBOX_WINDOWS_SIGN_CERT_BASE64 --repo $repositorySlug
if ($LASTEXITCODE -ne 0) {
    throw 'Failed to set COOLBOX_WINDOWS_SIGN_CERT_BASE64.'
}

$PfxPassword | & $ghExecutable secret set COOLBOX_WINDOWS_SIGN_CERT_PASSWORD --repo $repositorySlug
if ($LASTEXITCODE -ne 0) {
    throw 'Failed to set COOLBOX_WINDOWS_SIGN_CERT_PASSWORD.'
}

$TimestampUrl | & $ghExecutable secret set COOLBOX_WINDOWS_SIGN_TIMESTAMP_URL --repo $repositorySlug
if ($LASTEXITCODE -ne 0) {
    throw 'Failed to set COOLBOX_WINDOWS_SIGN_TIMESTAMP_URL.'
}

Write-Host '[publish_windows_signing_secrets] GitHub Actions secrets updated successfully.'

if ($TriggerWorkflow) {
    & $ghExecutable workflow run $Workflow --repo $repositorySlug
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to trigger workflow: $Workflow"
    }

    Write-Host "[publish_windows_signing_secrets] Triggered workflow: $Workflow"
}