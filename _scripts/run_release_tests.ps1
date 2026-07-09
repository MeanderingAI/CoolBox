param(
    [string]$Filter
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

Push-Location $buildDir
try {
    $ctestArgs = @('-C', 'Release', '--output-on-failure')
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
        & ctest @ctestArgs 2>&1 | Tee-Object -FilePath $logPath
        $exitCode = $LASTEXITCODE
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