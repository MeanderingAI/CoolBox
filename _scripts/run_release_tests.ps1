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

$previousPath = $env:PATH
$cachePath = Join-Path $buildDir 'CMakeCache.txt'
if (Test-Path $cachePath) {
    $compilerEntry = Select-String -Path $cachePath -Pattern '^CMAKE_CXX_COMPILER:(FILEPATH|STRING)=(.+)$' |
        Select-Object -First 1
    if ($null -ne $compilerEntry) {
        $compilerPath = $compilerEntry.Matches[0].Groups[2].Value
        if ([System.IO.Path]::GetFileName($compilerPath) -match '^(g\+\+|c\+\+)(\.exe)?$') {
            $compilerDirectory = Split-Path -Parent $compilerPath
            if (-not (Test-Path $compilerPath)) {
                throw "Configured MinGW compiler not found: $compilerPath"
            }
            Write-Host "[run_release_tests] MinGW compiler: $compilerPath"
            Write-Host "[run_release_tests] Prioritizing matching runtime directory: $compilerDirectory"
            foreach ($runtime in @('libstdc++-6.dll', 'libgcc_s_seh-1.dll', 'libwinpthread-1.dll')) {
                $runtimePath = Join-Path $compilerDirectory $runtime
                if (-not (Test-Path $runtimePath)) {
                    throw "Required MinGW runtime not found: $runtimePath"
                }
                Write-Host "[run_release_tests] Runtime: $runtimePath"
            }
            $env:PATH = "$compilerDirectory;$previousPath"
        }
    }
}

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
    $env:PATH = $previousPath
}

Write-Host "[run_release_tests] Test log written to $logPath"
exit $exitCode