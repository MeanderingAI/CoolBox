param(
    [Parameter(Mandatory = $true)]
    [string]$Workflow,

    [switch]$RebuildImage,

    [switch]$Act,

    [string[]]$RemainingArgs
)

$repoRoot = Split-Path -Parent $PSScriptRoot
$scriptPath = Join-Path $repoRoot "_local_build_pipeline/scripts/run_workflow.sh"

if (-not (Test-Path $scriptPath)) {
    throw "Unable to locate $scriptPath"
}

$bash = Get-Command bash -ErrorAction SilentlyContinue
if ($null -eq $bash) {
    throw "bash is required. Run this from Git Bash, WSL, or install bash on PATH."
}

$args = @($scriptPath, $Workflow)
if ($RebuildImage) { $args += "--rebuild-image" }
if ($Act) { $args += "--act" }
if ($RemainingArgs) { $args += $RemainingArgs }

& $bash.Source $args
exit $LASTEXITCODE