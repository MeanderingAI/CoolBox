param(
    [Parameter(Mandatory = $true)]
    [string]$Workflow,

    [switch]$RebuildImage,

    [ValidateSet('docker-linux', 'native-windows', 'native-macos')]
    [string]$Backend = 'docker-linux',

    [switch]$Act,

    [string[]]$RemainingArgs
)

$repoRoot = Split-Path -Parent $PSScriptRoot
$scriptPath = Join-Path $repoRoot "_local_build_pipeline/scripts/run_workflow.sh"

if (-not (Test-Path $scriptPath)) {
    throw "Unable to locate $scriptPath"
}

$bash = $null
$bashCandidates = @(
    "C:\Program Files\Git\bin\bash.exe",
    "C:\Program Files\Git\usr\bin\bash.exe"
)

foreach ($candidate in $bashCandidates) {
    if (Test-Path $candidate) {
        $bash = [pscustomobject]@{ Source = $candidate }
        break
    }
}

if ($null -eq $bash) {
    $resolvedBash = Get-Command bash -ErrorAction SilentlyContinue
    if ($null -ne $resolvedBash -and $resolvedBash.Source -ne "C:\Windows\system32\bash.exe") {
        $bash = $resolvedBash
    }
}

if ($null -eq $bash) {
    throw "Git Bash is required. Install Git for Windows or place a non-WSL bash on PATH."
}

$args = @($scriptPath, $Workflow)
if ($RebuildImage) { $args += "--rebuild-image" }
if ($Backend) { $args += @("--backend", $Backend) }
if ($Act) { $args += "--act" }
if ($RemainingArgs) { $args += $RemainingArgs }

& $bash.Source $args
exit $LASTEXITCODE