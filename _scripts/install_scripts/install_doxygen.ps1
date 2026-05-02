# PowerShell script to download and launch the Doxygen installer for Windows
# Usage: Run this script from PowerShell as Administrator

$ErrorActionPreference = 'Stop'

$doxygenUrl = "https://www.doxygen.nl/files/doxygen-1.10.0-setup.exe"
$doxygenInstaller = "$env:TEMP\doxygen-setup.exe"

Write-Host "Downloading Doxygen installer from $doxygenUrl..."
Invoke-WebRequest -Uri $doxygenUrl -OutFile $doxygenInstaller

Write-Host "Launching Doxygen installer..."
Start-Process -FilePath $doxygenInstaller -Wait

Write-Host "Doxygen installation complete. You may delete $doxygenInstaller if desired."

# Attempt to add Doxygen bin directory to system PATH
$doxygenBin = "C:\\Program Files\\doxygen\\bin"
if (Test-Path $doxygenBin) {
	$envPath = [System.Environment]::GetEnvironmentVariable("Path", [System.EnvironmentVariableTarget]::Machine)
	if ($envPath -notlike "*${doxygenBin}*") {
		Write-Host "Adding $doxygenBin to system PATH..."
		[System.Environment]::SetEnvironmentVariable("Path", "$envPath;${doxygenBin}", [System.EnvironmentVariableTarget]::Machine)
		Write-Host "PATH updated. You may need to restart your terminal or log out/in for changes to take effect."
	} else {
		Write-Host "$doxygenBin is already in PATH."
	}
} else {
	Write-Host "Could not find Doxygen bin directory at $doxygenBin. If you installed to a custom location, please add it to PATH manually."
}
