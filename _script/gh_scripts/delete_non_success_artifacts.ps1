param(
    [switch]$DryRun
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Get-RepoFromRemote {
    $remoteUrl = (git config --get remote.origin.url).Trim()
    if ([string]::IsNullOrWhiteSpace($remoteUrl)) {
        throw 'Unable to determine remote.origin.url from git config.'
    }

    $patternHttps = '^https://github\.com/([^/]+)/([^/]+?)(?:\.git)?$'
    $patternSsh = '^git@github\.com:([^/]+)/([^/]+?)(?:\.git)?$'

    if ($remoteUrl -match $patternHttps) {
        return @{ Owner = $Matches[1]; Repo = $Matches[2]; Remote = $remoteUrl }
    }

    if ($remoteUrl -match $patternSsh) {
        return @{ Owner = $Matches[1]; Repo = $Matches[2]; Remote = $remoteUrl }
    }

    throw "Unsupported GitHub remote URL format: $remoteUrl"
}

function Invoke-GhApiJson {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    $raw = gh api $Path
    if ($LASTEXITCODE -ne 0) {
        throw "gh api failed for path: $Path"
    }

    return ($raw | ConvertFrom-Json)
}

function Get-AllCompletedRuns {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Owner,
        [Parameter(Mandatory = $true)]
        [string]$Repo
    )

    $runs = @()
    $page = 1

    while ($true) {
        $endpoint = "repos/$Owner/$Repo/actions/runs?status=completed&per_page=100&page=$page"
        $response = Invoke-GhApiJson -Path $endpoint

        if ($null -eq $response.workflow_runs -or $response.workflow_runs.Count -eq 0) {
            break
        }

        $runs += $response.workflow_runs
        $page += 1
    }

    return $runs
}

function Get-RunArtifacts {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Owner,
        [Parameter(Mandatory = $true)]
        [string]$Repo,
        [Parameter(Mandatory = $true)]
        [int64]$RunId
    )

    $artifacts = @()
    $page = 1

    while ($true) {
        $endpoint = "repos/$Owner/$Repo/actions/runs/$RunId/artifacts?per_page=100&page=$page"
        $response = Invoke-GhApiJson -Path $endpoint

        if ($null -eq $response.artifacts -or $response.artifacts.Count -eq 0) {
            break
        }

        $artifacts += $response.artifacts
        $page += 1
    }

    return $artifacts
}

$repoInfo = Get-RepoFromRemote
$owner = $repoInfo.Owner
$repo = $repoInfo.Repo

Write-Host "Target repository: $owner/$repo"
Write-Host "Remote URL: $($repoInfo.Remote)"

$null = gh auth status --hostname github.com
if ($LASTEXITCODE -ne 0) {
    throw 'GitHub CLI is not authenticated for github.com. Run: gh auth login'
}

$completedRuns = Get-AllCompletedRuns -Owner $owner -Repo $repo
$nonSuccessRuns = @($completedRuns | Where-Object { $_.conclusion -ne 'success' })

$artifactsToDelete = @()
foreach ($run in $nonSuccessRuns) {
    $runArtifacts = Get-RunArtifacts -Owner $owner -Repo $repo -RunId ([int64]$run.id)
    foreach ($artifact in $runArtifacts) {
        $artifactsToDelete += [PSCustomObject]@{
            ArtifactId   = [int64]$artifact.id
            ArtifactName = [string]$artifact.name
            RunId        = [int64]$run.id
            Conclusion   = [string]$run.conclusion
            CreatedAt    = [string]$artifact.created_at
            SizeBytes    = [int64]$artifact.size_in_bytes
        }
    }
}

$deleted = @()
$failed = @()

if ($artifactsToDelete.Count -eq 0) {
    Write-Host 'No artifacts found for non-success workflow runs.'
} else {
    Write-Host ("Artifacts matched for deletion: {0}" -f $artifactsToDelete.Count)

    foreach ($artifact in $artifactsToDelete) {
        if ($DryRun) {
            Write-Host ("[DRY RUN] Would delete artifact {0} ({1}) from run {2} [{3}]" -f $artifact.ArtifactId, $artifact.ArtifactName, $artifact.RunId, $artifact.Conclusion)
            continue
        }

        gh api -X DELETE "repos/$owner/$repo/actions/artifacts/$($artifact.ArtifactId)" | Out-Null
        if ($LASTEXITCODE -eq 0) {
            $deleted += $artifact
            Write-Host ("Deleted artifact {0} ({1}) from run {2} [{3}]" -f $artifact.ArtifactId, $artifact.ArtifactName, $artifact.RunId, $artifact.Conclusion)
        } else {
            $failed += $artifact
            Write-Warning ("Failed to delete artifact {0} ({1})" -f $artifact.ArtifactId, $artifact.ArtifactName)
        }
    }
}

Write-Host ''
Write-Host 'Summary:'
Write-Host ("- Completed runs scanned: {0}" -f $completedRuns.Count)
Write-Host ("- Non-success runs scanned: {0}" -f $nonSuccessRuns.Count)
Write-Host ("- Artifacts matched: {0}" -f $artifactsToDelete.Count)
Write-Host ("- Deleted: {0}" -f $deleted.Count)
Write-Host ("- Failed deletions: {0}" -f $failed.Count)

if ($deleted.Count -gt 0) {
    Write-Host ''
    Write-Host 'Sample deleted artifacts:'
    $deleted | Select-Object -First 10 ArtifactId, ArtifactName, RunId, Conclusion | Format-Table -AutoSize
}

if ($failed.Count -gt 0) {
    exit 1
}
