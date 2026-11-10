function Invoke-MovieEditorSetupTool {
    param([string]$Executable, [string[]]$Arguments)
    & $Executable @Arguments | Out-Host
    return $LASTEXITCODE
}

function Find-MovieEditorFfmpeg {
    if ($env:COOLBOX_FFMPEG) {
        $candidate = $env:COOLBOX_FFMPEG
        if (-not (Test-Path -LiteralPath $candidate -PathType Leaf) -or
            -not (Test-Path -LiteralPath (Join-Path (Split-Path -Parent $candidate) 'ffprobe.exe') -PathType Leaf)) {
            throw 'COOLBOX_FFMPEG must point to ffmpeg.exe with ffprobe.exe in the same folder. Correct or clear the override before running setup.'
        }
        return (Resolve-Path -LiteralPath $candidate).Path
    }

    $command = Get-Command ffmpeg.exe -CommandType Application -ErrorAction SilentlyContinue
    if ($command -and (Test-Path -LiteralPath (Join-Path (Split-Path -Parent $command.Source) 'ffprobe.exe') -PathType Leaf)) {
        return $command.Source
    }

    foreach ($root in @(
        (Join-Path $env:LOCALAPPDATA 'Microsoft\WinGet\Packages'),
        (Join-Path $env:ProgramFiles 'WinGet\Packages')
    )) {
        if (-not (Test-Path -LiteralPath $root -PathType Container)) { continue }
        $packages = Get-ChildItem -LiteralPath $root -Directory -Filter 'Gyan.FFmpeg.Essentials_*'
        foreach ($package in $packages) {
            $candidates = Get-ChildItem -LiteralPath $package.FullName -Recurse -File -Filter 'ffmpeg.exe' |
                Sort-Object FullName
            foreach ($candidate in $candidates) {
                if (Test-Path -LiteralPath (Join-Path $candidate.DirectoryName 'ffprobe.exe') -PathType Leaf) {
                    return $candidate.FullName
                }
            }
        }
    }
    return $null
}

function Initialize-MovieEditorFfmpeg {
    param([string]$EnvironmentFile)

    $executable = Find-MovieEditorFfmpeg
    if (-not $executable) {
        $winget = Get-Command winget.exe -CommandType Application -ErrorAction SilentlyContinue
        if (-not $winget) {
            throw 'FFmpeg and ffprobe were not found, and WinGet is unavailable. Install Microsoft App Installer (WinGet), or install FFmpeg manually and set COOLBOX_FFMPEG.'
        }
        Write-Host 'FFmpeg/ffprobe not found. Installing Gyan.FFmpeg.Essentials through WinGet...'
        $result = Invoke-MovieEditorSetupTool -Executable $winget.Source -Arguments @(
            'install', '--id', 'Gyan.FFmpeg.Essentials', '--exact', '--source', 'winget', '--scope', 'user',
            '--accept-package-agreements', '--accept-source-agreements', '--disable-interactivity'
        )
        if ($result -ne 0) {
            throw "FFmpeg installation failed (WinGet exit code $result). See the installer output above; install FFmpeg manually or retry setup."
        }
        $executable = Find-MovieEditorFfmpeg
        if (-not $executable) {
            throw 'WinGet finished, but ffmpeg.exe and ffprobe.exe could not be located. Set COOLBOX_FFMPEG to the installed ffmpeg.exe and retry.'
        }
    }

    $probe = Join-Path (Split-Path -Parent $executable) 'ffprobe.exe'
    foreach ($tool in @($executable, $probe)) {
        $result = Invoke-MovieEditorSetupTool -Executable $tool -Arguments @('-version')
        if ($result -ne 0) { throw "FFmpeg setup validation failed for $tool (exit code $result)." }
    }
    $env:COOLBOX_FFMPEG = $executable
    if ($EnvironmentFile) {
        $directory = Split-Path -Parent $EnvironmentFile
        New-Item -ItemType Directory -Path $directory -Force | Out-Null
        $encoding = New-Object System.Text.UTF8Encoding($false)
        [System.IO.File]::WriteAllText($EnvironmentFile, "COOLBOX_FFMPEG=$executable`n", $encoding)
    }
    Write-Host "FFmpeg ready: $executable"
}
