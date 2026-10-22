$ErrorActionPreference = 'Stop'

if (Test-Path 'build/CMakeCache.txt') {
    # A cache created elsewhere (for example WSL's /mnt/c/... path) cannot be reused by Windows CMake.
    $homeLine = Select-String -Path 'build/CMakeCache.txt' -Pattern '^CMAKE_HOME_DIRECTORY:INTERNAL=(.*)$' | Select-Object -First 1
    $cachedSource = if ($homeLine) { $homeLine.Matches[0].Groups[1].Value } else { '' }
    $currentSource = (Get-Location).Path
    $cachedNormalized = $cachedSource.Replace('/', '\').TrimEnd('\')
    $generated = @(Get-ChildItem 'build' -File -ErrorAction SilentlyContinue |
        Where-Object { $_.Extension -eq '.sln' -or $_.Extension -eq '.slnx' -or $_.Name -in @('build.ninja', 'Makefile') })
    if ($cachedNormalized -and ($cachedNormalized -ieq $currentSource.TrimEnd('\')) -and $generated.Count -gt 0) {
        Write-Host '[Makefile.win] Build already configured (build/CMakeCache.txt exists).'
        exit 0
    }
    if ($cachedNormalized -ieq $currentSource.TrimEnd('\')) {
        Write-Host '[Makefile.win] Previous CMake configuration did not complete. Reconfiguring.'
    } else {
        Write-Host "[Makefile.win] Existing build/ was configured for '$cachedSource', not this Windows checkout. Reconfiguring."
    }
    Remove-Item -LiteralPath 'build/CMakeCache.txt' -Force
    if (Test-Path 'build/CMakeFiles') {
        Remove-Item -LiteralPath 'build/CMakeFiles' -Recurse -Force
    }
}

Write-Host '[Makefile.win] Running CMake configuration...'

$generator = ''
if ($env:VS_GENERATOR) {
    $generator = $env:VS_GENERATOR
} else {
    $vswherePath = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswherePath) {
        $displayName = & $vswherePath -latest -products * -requires Microsoft.Component.MSBuild -property displayName
        if ($displayName -match '2026') {
            $generator = 'Visual Studio 18 2026'
        } elseif ($displayName -match '2022') {
            $generator = 'Visual Studio 17 2022'
        } elseif ($displayName -match '2019') {
            $generator = 'Visual Studio 16 2019'
        } elseif ($displayName -match '2017') {
            $generator = 'Visual Studio 15 2017'
        }
    }
}

$generatorPlatform = ''
if ($env:VS_GENERATOR_PLATFORM) {
    $generatorPlatform = $env:VS_GENERATOR_PLATFORM
} elseif ((Get-ItemProperty 'HKLM:\SYSTEM\CurrentControlSet\Control\Session Manager\Environment').PROCESSOR_ARCHITECTURE -eq 'ARM64') {
    # Read the native architecture: emulated callers (such as 32-bit GnuWin32 make) report x86/AMD64.
    # CMake defaults to the native platform; fall back to x64 (emulated) when the ARM64 C++ toolset is absent.
    $vswherePath = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswherePath) {
        $installPath = & $vswherePath -latest -products * -requires Microsoft.Component.MSBuild -property installationPath
        if ($installPath) {
            $arm64Platform = Get-ChildItem (Join-Path $installPath 'MSBuild\Microsoft\VC\*\Platforms\ARM64') -Directory -ErrorAction SilentlyContinue
            if (-not $arm64Platform) {
                Write-Host '[Makefile.win] ARM64 C++ build tools are not installed; targeting x64. Install the "MSVC ARM64 build tools" component or set VS_GENERATOR_PLATFORM to override.'
                $generatorPlatform = 'x64'
            }
        }
    }
}

$args = @(
    '-S', '.',
    '-B', 'build',
    '-DBUILD_BINARIES=OFF'
)

# GSL is auto-detected; set ENABLE_GSL=ON/OFF to override.
if ($env:ENABLE_GSL) {
    $args += "-DENABLE_GSL=$($env:ENABLE_GSL)"
}

if ($generator) {
    $args += @('-G', $generator)
}

if ($generatorPlatform) {
    $args += @('-A', $generatorPlatform)
}

if ($env:VCPKG_TARGET_TRIPLET) {
    $args += "-DVCPKG_TARGET_TRIPLET=$($env:VCPKG_TARGET_TRIPLET)"
}

$vcpkgCandidates = @()
if ($env:VCPKG_ROOT) {
    $vcpkgCandidates += (Join-Path $env:VCPKG_ROOT 'scripts\buildsystems\vcpkg.cmake')
}
if ($env:USERPROFILE) {
    $vcpkgCandidates += (Join-Path $env:USERPROFILE 'vcpkg\scripts\buildsystems\vcpkg.cmake')
}
if ($env:LOCALAPPDATA) {
    $vcpkgCandidates += (Join-Path $env:LOCALAPPDATA 'vcpkg\scripts\buildsystems\vcpkg.cmake')
}

$vcpkgToolchain = $vcpkgCandidates |
    Select-Object -Unique |
    Where-Object { Test-Path $_ } |
    Select-Object -First 1

if ($vcpkgToolchain) {
    $args += "-DCMAKE_TOOLCHAIN_FILE=$vcpkgToolchain"
}

& cmake @args
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}