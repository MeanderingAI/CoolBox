$ErrorActionPreference = 'Stop'

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $scriptDir))
$outDir = Join-Path $repoRoot '_local_build_pipeline\out\build-libs-native-windows'
$defaultTmpRoot = if ($env:COOLBOX_NATIVE_DEFAULT_ROOT) {
    $env:COOLBOX_NATIVE_DEFAULT_ROOT
} elseif ($env:SystemDrive) {
    Join-Path $env:SystemDrive 'coolbox-lbp'
} else {
    Join-Path $repoRoot '_local_build_pipeline\tmp'
}
$tmpRoot = if ($env:COOLBOX_NATIVE_TMP_ROOT) {
    $env:COOLBOX_NATIVE_TMP_ROOT
} else {
    $defaultTmpRoot
}
$runStamp = Get-Date -Format 'yyyyMMddHHmmss'
$buildDir = Join-Path $tmpRoot ("build-libs-native-windows-" + $runStamp)
$logPath = Join-Path $outDir 'command.log'
$runLogPath = Join-Path $outDir ("command-" + $runStamp + '.log')

New-Item -ItemType Directory -Force -Path $outDir | Out-Null
New-Item -ItemType Directory -Force -Path $tmpRoot | Out-Null

$status = 0

function Sync-PathIfExists {
    param(
        [Parameter(Mandatory = $true)]
        [string]$SourcePath,

        [Parameter(Mandatory = $true)]
        [string]$DestinationName
    )

    if (Test-Path $SourcePath) {
        $destinationPath = Join-Path $outDir $DestinationName
        if (Test-Path $destinationPath) {
            Remove-Item $destinationPath -Recurse -Force
        }
        Copy-Item $SourcePath $destinationPath -Recurse -Force
    }
}

function Get-RuntimeDllPathPrefix {
    param(
        [Parameter(Mandatory = $true)]
        [string]$RootPath,

        [Parameter(Mandatory = $true)]
        [string]$Configuration
    )

    if (-not (Test-Path $RootPath)) {
        return ''
    }

    $dllDirectories = Get-ChildItem -Path $RootPath -Recurse -File -Filter '*.dll' |
        Where-Object {
            $_.DirectoryName -and (
                $_.DirectoryName.EndsWith("\$Configuration") -or
                $_.DirectoryName.EndsWith("/$Configuration")
            )
        } |
        Select-Object -ExpandProperty DirectoryName -Unique

    if (-not $dllDirectories) {
        return ''
    }

    return (($dllDirectories | Sort-Object -Unique) -join ';')
}

function Find-VcpkgExe {
    $command = Get-Command vcpkg.exe -ErrorAction SilentlyContinue
    $candidates = @(
        $(if ($null -ne $command) { $command.Source }),
        $(if ($null -ne $command) { $command.Path }),
        $(if ($env:VCPKG_ROOT) { Join-Path $env:VCPKG_ROOT 'vcpkg.exe' }),
        'C:\vcpkg\vcpkg.exe',
        $(if ($env:USERPROFILE) { Join-Path $env:USERPROFILE 'vcpkg\vcpkg.exe' }),
        $(if ($env:LOCALAPPDATA) { Join-Path $env:LOCALAPPDATA 'vcpkg\vcpkg.exe' })
    ) | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | Select-Object -Unique

    foreach ($candidate in $candidates) {
        if (Test-Path $candidate) {
            return $candidate
        }
    }

    return $null
}

function Ensure-VcpkgToolchain {
    $vcpkgExe = Find-VcpkgExe
    if ($null -eq $vcpkgExe) {
        Write-Host 'vcpkg was not found; attempting local bootstrap via _scripts/install_vcpkg.ps1'
        & powershell -NoProfile -NonInteractive -ExecutionPolicy Bypass -File (Join-Path $repoRoot '_scripts\install_vcpkg.ps1')
        if ($LASTEXITCODE -ne 0) {
            throw "vcpkg installation failed with exit code $LASTEXITCODE"
        }
        $vcpkgExe = Find-VcpkgExe
    }

    if ($null -eq $vcpkgExe) {
        throw 'vcpkg.exe was not found after installation attempt. Windows native builds require vcpkg to provide GSL, Eigen, SQLite, and OpenSSL.'
    }

    $vcpkgRoot = Split-Path -Parent $vcpkgExe
    $env:VCPKG_ROOT = $vcpkgRoot
    $toolchain = Join-Path $vcpkgRoot 'scripts\buildsystems\vcpkg.cmake'
    if (!(Test-Path $toolchain)) {
        throw "vcpkg toolchain file not found at $toolchain"
    }

    Write-Host "Installing required vcpkg packages for x64-windows from $vcpkgExe"
    & $vcpkgExe install eigen3:x64-windows sqlite3:x64-windows gsl:x64-windows openssl:x64-windows
    if ($LASTEXITCODE -ne 0) {
        throw "vcpkg dependency installation failed with exit code $LASTEXITCODE"
    }

    return $toolchain
}

function Ensure-BisonOnPath {
    $bison = Get-Command bison, win_bison -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($null -ne $bison) {
        return
    }

    $candidateDirs = @(
        'C:\ProgramData\chocolatey\lib\winflexbison\tools',
        'C:\msys64\usr\bin',
        'C:\tools\msys64\usr\bin'
    )

    foreach ($candidateDir in $candidateDirs) {
        if ((Test-Path (Join-Path $candidateDir 'win_bison.exe')) -or (Test-Path (Join-Path $candidateDir 'bison.exe'))) {
            $env:PATH = $candidateDir + ';' + $env:PATH
            return
        }
    }

    if (Get-Command choco -ErrorAction SilentlyContinue) {
        Write-Host 'Bison was not found; attempting installation via _scripts/install_bison.ps1'
        & powershell -NoProfile -NonInteractive -ExecutionPolicy Bypass -File (Join-Path $repoRoot '_scripts\install_bison.ps1')
        if ($LASTEXITCODE -ne 0) {
            throw "Bison installation failed with exit code $LASTEXITCODE"
        }

        $chocoDir = 'C:\ProgramData\chocolatey\lib\winflexbison\tools'
        if (Test-Path $chocoDir) {
            $env:PATH = $chocoDir + ';' + $env:PATH
        }
    }

    $bison = Get-Command bison, win_bison -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($null -eq $bison) {
        throw 'Bison is required but was not found. Run _scripts/install_bison.ps1 or install winflexbison and ensure bison or win_bison is on PATH.'
    }
}

Start-Transcript -Path $runLogPath | Out-Null

try {
    Set-Location $repoRoot

    if (Test-Path $buildDir) {
        Remove-Item $buildDir -Recurse -Force
    }

    Ensure-BisonOnPath
    $vcpkgToolchain = Ensure-VcpkgToolchain

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

    $cmakeArgs = @(
        '-S', '.',
        '-B', $buildDir,
        '-Wno-dev',
        '-A', 'x64',
        '-DBUILD_BINARIES=OFF',
        '-DBUILD_PRODUCTS=ON',
        '-DBUILD_PRODUCT_INSTALLER_ABSTRACTIONS=ON',
        '-DBUILD_IO_SQL=ON',
        '-DBUILD_TESTING=ON',
        '-DCMAKE_BUILD_TYPE=Release',
        "-DCMAKE_TOOLCHAIN_FILE=$($vcpkgToolchain)",
        '-DVCPKG_TARGET_TRIPLET=x64-windows'
    )

    if ($generator) {
        $cmakeArgs += @('-G', $generator)
    }

    & cmake @cmakeArgs
    if ($LASTEXITCODE -ne 0) {
        throw "CMake configure failed with exit code $LASTEXITCODE"
    }

    & cmake --build $buildDir --config Release
    if ($LASTEXITCODE -ne 0) {
        throw "CMake build failed with exit code $LASTEXITCODE"
    }

    & powershell -NoProfile -NonInteractive -ExecutionPolicy Bypass -File (Join-Path $repoRoot '_scripts\sign_windows_artifacts.ps1') -Root $buildDir -Configuration 'Release'
    if ($LASTEXITCODE -ne 0) {
        throw "Artifact signing failed with exit code $LASTEXITCODE"
    }

    if (-not (Test-Path (Join-Path $buildDir 'CTestTestfile.cmake'))) {
        throw 'CTest was not generated. Tests were likely skipped during configure.'
    }

    Push-Location $buildDir
    try {
        $runtimePathPrefix = Get-RuntimeDllPathPrefix -RootPath $buildDir -Configuration 'Release'
        $originalPath = $env:PATH
        if ($runtimePathPrefix) {
            $env:PATH = $runtimePathPrefix + ';' + $env:PATH
            Write-Host "Prepended runtime DLL directories for CTest: $runtimePathPrefix"
        }

        & ctest --output-on-failure --build-config Release
        if ($LASTEXITCODE -ne 0) {
            throw "CTest failed with exit code $LASTEXITCODE"
        }
    } finally {
        if ($null -ne $originalPath) {
            $env:PATH = $originalPath
        }
        Pop-Location
    }
} catch {
    $status = 1
    Write-Host $_
} finally {
    Stop-Transcript | Out-Null
    "EXIT_STATUS=$status" | Out-File -FilePath $runLogPath -Append -Encoding utf8
    try {
        Copy-Item $runLogPath $logPath -Force
    } catch {
        Write-Host "Unable to refresh ${logPath}: $($_.Exception.Message)"
    }
    Sync-PathIfExists -SourcePath (Join-Path $buildDir 'Testing') -DestinationName 'Testing'
    Sync-PathIfExists -SourcePath (Join-Path $buildDir 'test-logs') -DestinationName 'test-logs'
    Sync-PathIfExists -SourcePath (Join-Path $repoRoot 'release-assets') -DestinationName 'release-assets'
}

exit $status