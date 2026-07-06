<#
.SYNOPSIS
    Installs Visual Studio 2022 Community Edition with C++ Desktop Development tools.
.DESCRIPTION
    Downloads and installs Visual Studio 2022 Community Edition silently with required workloads for C++ development.
    You can customize workloads and components as needed.
.PARAMETER InstallPath
    Optional. The installation directory for Visual Studio.
.PARAMETER AdditionalArgs
    Optional. Additional arguments to pass to the Visual Studio installer.
#>

param(
    [string]$InstallPath = "",
    [string[]]$AdditionalArgs = @()
)

$ErrorActionPreference = 'Stop'

$vsInstallerUrl = "https://aka.ms/vs/17/release/vs_Community.exe"
$vsInstallerExe = Join-Path $env:TEMP "vs_Community.exe"

Write-Host "[install_visual_studio] Downloading Visual Studio installer..."
Invoke-WebRequest -Uri $vsInstallerUrl -OutFile $vsInstallerExe

$workloads = @(
    "--add Microsoft.VisualStudio.Workload.NativeDesktop" # Desktop development with C++
    "--includeRecommended"
    "--passive"
    "--norestart"
)

if ($InstallPath) {
    $workloads += "--installPath `"$InstallPath`""
}
if ($AdditionalArgs.Count -gt 0) {
    $workloads += $AdditionalArgs
}

Write-Host "[install_visual_studio] Installing Visual Studio 2022 Community Edition..."
& $vsInstallerExe $workloads

Write-Host "[install_visual_studio] Installation complete."
