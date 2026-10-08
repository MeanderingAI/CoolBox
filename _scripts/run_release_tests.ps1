param(
    [string]$Filter,
    [ValidateRange(1, 86400)]
    [int]$TimeoutSeconds = 300
)

$ErrorActionPreference = 'Stop'
$exitCode = 0

$repoRoot = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $repoRoot 'build'
$logPath = Join-Path $repoRoot 'ctest-release.log'

if (-not (Test-Path $buildDir)) {
    throw "Build directory not found at $buildDir"
}

Write-Host "[run_release_tests] Running Release tests from $buildDir"
Write-Host "[run_release_tests] Default per-test timeout: $TimeoutSeconds seconds"

Push-Location $buildDir
try {
    $ctestArgs = @('-C', 'Release', '--output-on-failure', '--timeout', "$TimeoutSeconds")
    if ($Filter) {
        $ctestArgs += @('-R', $Filter)
        Write-Host "[run_release_tests] Applying test filter: $Filter"
    }

    $restoreNativeCommandPreference = $false
    if (Get-Variable -Name PSNativeCommandUseErrorActionPreference -ErrorAction SilentlyContinue) {
        $previousNativeCommandPreference = $PSNativeCommandUseErrorActionPreference
        $PSNativeCommandUseErrorActionPreference = $false
        $restoreNativeCommandPreference = $true
    }

    try {
        $previousErrorActionPreference = $ErrorActionPreference
        $ErrorActionPreference = 'Continue'
        try {
            & ctest @ctestArgs 2>&1 | Tee-Object -FilePath $logPath
            $exitCode = $LASTEXITCODE
        }
        finally {
            $ErrorActionPreference = $previousErrorActionPreference
        }
    }
    finally {
        if ($restoreNativeCommandPreference) {
            $PSNativeCommandUseErrorActionPreference = $previousNativeCommandPreference
        }
    }
}
finally {
    Pop-Location
}

Write-Host "[run_release_tests] Test log written to $logPath"
exit $exitCode