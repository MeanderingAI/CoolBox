$ErrorActionPreference = 'Stop'

if (Test-Path 'build/CMakeCache.txt') {
    Write-Host '[Makefile.win] Build already configured (build/CMakeCache.txt exists).'
    exit 0
}

Write-Host '[Makefile.win] Running CMake configuration...'

$generator = ''
if ($env:VS_GENERATOR) {
    $generator = $env:VS_GENERATOR
} elseif (Get-Command vswhere.exe -ErrorAction SilentlyContinue) {
    $displayName = & vswhere.exe -latest -products * -requires Microsoft.Component.MSBuild -property displayName
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

$generatorPlatform = ''
if ($env:VS_GENERATOR_PLATFORM) {
    $generatorPlatform = $env:VS_GENERATOR_PLATFORM
}

$args = @(
    '-S', '.',
    '-B', 'build',
    '-DBUILD_BINARIES=OFF',
    '-DENABLE_GSL=ON'
)

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