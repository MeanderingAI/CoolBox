# PowerShell script to install WinFlexBison (Bison for Windows) and add it to PATH
# Usage: Run this script from PowerShell as Administrator

$ErrorActionPreference = 'Stop'

# Install WinFlexBison via Chocolatey
if (!(Get-Command choco -ErrorAction SilentlyContinue)) {
    Write-Host "Chocolatey is not installed. Please install Chocolatey first."
    exit 1
}

Write-Host "Installing WinFlexBison (Bison for Windows)..."
choco install -y winflexbison

# Path to WinFlexBison tools
$winflexbisonPath = "C:\\ProgramData\\chocolatey\\lib\\winflexbison\\tools"

# Add to system PATH if not already present
$envPath = [System.Environment]::GetEnvironmentVariable("Path", [System.EnvironmentVariableTarget]::Machine)
if ($envPath -notlike "*${winflexbisonPath}*") {
    Write-Host "Adding $winflexbisonPath to system PATH..."
    [System.Environment]::SetEnvironmentVariable("Path", "$envPath;${winflexbisonPath}", [System.EnvironmentVariableTarget]::Machine)
    Write-Host "PATH updated. You may need to restart your terminal or log out/in for changes to take effect."
} else {
    Write-Host "$winflexbisonPath is already in PATH."
}

Write-Host "WinFlexBison installation and PATH update complete."
