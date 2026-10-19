$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'movie_editor_ffmpeg_windows.ps1')

function Assert-True($Condition, [string]$Message) {
    if (-not $Condition) { throw $Message }
}
function Assert-Throws([scriptblock]$Action, [string]$Expected) {
    try { & $Action } catch {
        Assert-True ($_.Exception.Message -like "*$Expected*") "Unexpected error: $_"
        return
    }
    throw "Expected error containing: $Expected"
}

$root = Join-Path ([System.IO.Path]::GetTempPath()) ('coolbox-ffmpeg-setup-' + [guid]::NewGuid().ToString('N'))
$savedLocal = $env:LOCALAPPDATA
$savedPrograms = $env:ProgramFiles
$savedOverride = $env:COOLBOX_FFMPEG
try {
    $env:LOCALAPPDATA = Join-Path $root 'local'
    $env:ProgramFiles = Join-Path $root 'programs'
    $env:COOLBOX_FFMPEG = $null
    $script:bin = Join-Path $env:LOCALAPPDATA 'Microsoft\WinGet\Packages\Gyan.FFmpeg.Essentials_test\ffmpeg test\bin'
    $script:installs = 0
    $script:wingetAvailable = $true
    $script:installResult = 0
    $script:toolResult = 0
    $script:createPackage = $true
    $script:pathCandidate = $null

    function Get-Command {
        param($Name, $CommandType, $ErrorAction)
        if ($Name -eq 'winget.exe' -and $script:wingetAvailable) {
            return [pscustomobject]@{ Source = 'test-winget.exe' }
        }
        if ($Name -eq 'ffmpeg.exe' -and $script:pathCandidate) {
            return [pscustomobject]@{ Source = $script:pathCandidate }
        }
        return $null
    }
    function Invoke-MovieEditorSetupTool {
        param([string]$Executable, [string[]]$Arguments)
        if ($Executable -eq 'test-winget.exe') {
            $script:installs++
            Assert-True ($Arguments -contains 'Gyan.FFmpeg.Essentials') 'Wrong package ID.'
            Assert-True ($Arguments -contains '--disable-interactivity') 'Installer must not wait for input.'
            Assert-True ($Arguments -contains 'user') 'Installer must use user scope.'
            if ($script:installResult -eq 0 -and $script:createPackage) {
                New-Item -ItemType Directory -Path $script:bin -Force | Out-Null
                New-Item -ItemType File -Path (Join-Path $script:bin 'ffmpeg.exe'), (Join-Path $script:bin 'ffprobe.exe') -Force | Out-Null
            }
            return $script:installResult
        }
        Assert-True (Test-Path -LiteralPath $Executable) 'Validation tool does not exist.'
        Assert-True ($Arguments[0] -eq '-version') 'Validation must execute the tool.'
        return $script:toolResult
    }

    $environmentFile = Join-Path $root 'build\movie_editor.env'
    Initialize-MovieEditorFfmpeg -EnvironmentFile $environmentFile
    Assert-True ($script:installs -eq 1) 'Missing installation did not invoke WinGet once.'
    Assert-True ($env:COOLBOX_FFMPEG -eq (Join-Path $script:bin 'ffmpeg.exe')) 'Installed path was not propagated.'
    Assert-True ((Get-Content -LiteralPath $environmentFile -Raw) -eq "COOLBOX_FFMPEG=$env:COOLBOX_FFMPEG`n") 'Debugger environment file is wrong.'
    Initialize-MovieEditorFfmpeg
    Assert-True ($script:installs -eq 1) 'Existing override triggered a second install.'
    $env:COOLBOX_FFMPEG = $null
    Initialize-MovieEditorFfmpeg
    Assert-True ($script:installs -eq 1) 'Stale PATH triggered reinstall despite existing WinGet package.'
    $env:COOLBOX_FFMPEG = $null
    $script:pathCandidate = Join-Path $script:bin 'ffmpeg.exe'
    Initialize-MovieEditorFfmpeg
    Assert-True ($script:installs -eq 1) 'PATH installation triggered reinstall.'
    $script:toolResult = 7
    Assert-Throws { Initialize-MovieEditorFfmpeg } 'validation failed'
    $script:toolResult = 0
    $env:COOLBOX_FFMPEG = Join-Path $root 'missing.exe'
    Assert-Throws { Initialize-MovieEditorFfmpeg } 'Correct or clear the override'
    $env:COOLBOX_FFMPEG = Join-Path $script:bin 'ffmpeg.exe'
    Remove-Item -LiteralPath (Join-Path $script:bin 'ffprobe.exe')
    Assert-Throws { Initialize-MovieEditorFfmpeg } 'ffprobe.exe'
    $env:COOLBOX_FFMPEG = $null
    $script:pathCandidate = $null
    $script:wingetAvailable = $false
    Assert-Throws { Initialize-MovieEditorFfmpeg } 'WinGet is unavailable'
    $script:wingetAvailable = $true
    $script:installResult = 23
    Assert-Throws { Initialize-MovieEditorFfmpeg } 'WinGet exit code 23'
    $script:installResult = 0
    $script:createPackage = $false
    Assert-Throws { Initialize-MovieEditorFfmpeg } 'could not be located'
    Write-Host 'PASS: install, reuse, stale PATH, debugger environment, override validation, missing ffprobe/WinGet, installer failure, and post-install validation.'
} finally {
    $env:LOCALAPPDATA = $savedLocal
    $env:ProgramFiles = $savedPrograms
    $env:COOLBOX_FFMPEG = $savedOverride
    if (Test-Path -LiteralPath $root) { Remove-Item -LiteralPath $root -Recurse -Force }
}
