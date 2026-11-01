param(
    [string]$BuildDirectory = 'build',
    [switch]$StandaloneInstall
)

$ErrorActionPreference = 'Stop'

if (-not $StandaloneInstall -and (Test-Path (Join-Path $BuildDirectory 'CMakeCache.txt'))) {
    Write-Host "[Makefile.win] Build already configured ($BuildDirectory/CMakeCache.txt exists)."
    exit 0
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
} elseif ($StandaloneInstall -and $generator -like 'Visual Studio*') {
    $generatorPlatform = 'x64'
}

$args = @(
    '-S', '.',
    '-B', $BuildDirectory,
    '-DBUILD_BINARIES=OFF'
)

if ($StandaloneInstall) {
    $args += @(
        '-DCOOLBOX_STANDALONE_INSTALL=ON',
        '-DCOOLBOX_INSTALL_PACKAGE=ON',
        '-DCOOLBOX_ENABLE_GPU_ACCELERATION=OFF',
        '-DBUILD_TESTING=OFF',
        '-DCMAKE_BUILD_TYPE=Release'
    )
} else {
    $args += '-DENABLE_GSL=ON'
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

if ($vcpkgToolchain -and -not $StandaloneInstall) {
    $args += "-DCMAKE_TOOLCHAIN_FILE=$vcpkgToolchain"
}

& cmake @args
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}