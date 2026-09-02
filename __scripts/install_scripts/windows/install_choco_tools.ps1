# PowerShell script to install Chocolatey and common developer tools on Windows
# Usage: Run this script from PowerShell as Administrator

$ErrorActionPreference = 'Stop'

# Install Chocolatey if not already installed
if (!(Get-Command choco -ErrorAction SilentlyContinue)) {
    Write-Host "Installing Chocolatey..."
    Set-ExecutionPolicy Bypass -Scope Process -Force
    [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072
    Invoke-Expression ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))
} else {
    Write-Host "Chocolatey is already installed."
}

# Install WinFlexBison
Write-Host "Installing WinFlexBison..."
choco install -y winflexbison

Write-Host "WinFlexBison installation complete."
Write-Host "\nDoxygen is not reliably available via Chocolatey. Please download and install it manually from: https://www.doxygen.nl/download.html\n"
