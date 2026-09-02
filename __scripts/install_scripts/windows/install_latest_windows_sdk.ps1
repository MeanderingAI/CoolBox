param(
    [ValidateSet('Windows11', 'Windows10')]
    [string]$Channel = 'Windows11',

    [string]$DownloadDirectory = '',

    [switch]$SkipDownload,

    [string]$InstallerPath = '',

    [switch]$Interactive,

    [string[]]$InstallerArguments = @(),

    [switch]$PrintOnly
)

$ErrorActionPreference = 'Stop'

$downloadScript = Join-Path $PSScriptRoot 'download_latest_windows_sdk.ps1'
if (-not (Test-Path $downloadScript)) {
    throw "Download helper not found: $downloadScript"
}

if (-not $DownloadDirectory) {
    $DownloadDirectory = Join-Path $env:TEMP 'coolbox-downloads'
}

if (-not $InstallerPath) {
    if ($SkipDownload) {
        throw 'InstallerPath is required when -SkipDownload is used.'
    }

    $downloadArguments = @{
        Channel = $Channel
        Artifact = 'Installer'
        OutputDirectory = $DownloadDirectory
        PassThru = $true
    }
    if ($PrintOnly) {
        $downloadArguments['PrintOnly'] = $true
    }

    $downloadResult = & $downloadScript @downloadArguments
    if (-not $downloadResult -or -not $downloadResult.DestinationPath) {
        throw 'The download helper did not return an installer path.'
    }

    $InstallerPath = $downloadResult.DestinationPath
}

if (-not (Test-Path $InstallerPath)) {
    throw "Installer not found: $InstallerPath"
}

$effectiveArguments = @()
if ($InstallerArguments.Count -gt 0) {
    $effectiveArguments = $InstallerArguments
} elseif (-not $Interactive) {
    $effectiveArguments = @('/quiet', '/norestart')
}

Write-Host "[install_latest_windows_sdk] Installer: $InstallerPath"
if ($effectiveArguments.Count -gt 0) {
    Write-Host "[install_latest_windows_sdk] Arguments: $($effectiveArguments -join ' ')"
} else {
    Write-Host '[install_latest_windows_sdk] Arguments: <interactive defaults>'
}

if ($PrintOnly) {
    return
}

$process = Start-Process -FilePath $InstallerPath -ArgumentList $effectiveArguments -Wait -PassThru
Write-Host "[install_latest_windows_sdk] Installer exit code: $($process.ExitCode)"

if ($process.ExitCode -ne 0) {
    throw "Windows SDK installer exited with code $($process.ExitCode)"
}