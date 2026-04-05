$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $repoRoot 'build'
$logPath = Join-Path $repoRoot 'ctest-release.log'

if (-not (Test-Path $buildDir)) {
    throw "Build directory not found at $buildDir"
}

Write-Host "[run_release_tests] Running Release tests from $buildDir"

Push-Location $buildDir
try {
    & ctest -C Release --output-on-failure 2>&1 | Tee-Object -FilePath $logPath
    $exitCode = $LASTEXITCODE
}
finally {
    Pop-Location
}

Write-Host "[run_release_tests] Test log written to $logPath"
exit $exitCode