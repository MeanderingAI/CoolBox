# PowerShell script to install vcpkg on Windows
# Usage: Run this script from PowerShell as Administrator

$ErrorActionPreference = 'Stop'

# Set install directory
$vcpkgRoot = "$env:USERPROFILE\vcpkg"
$vcpkgExe = Join-Path $vcpkgRoot 'vcpkg.exe'

if (Test-Path $vcpkgExe) {
    Write-Host "vcpkg already exists at $vcpkgExe"
    exit 0
}

if (!(Test-Path $vcpkgRoot)) {
    Write-Host "Cloning vcpkg into $vcpkgRoot..."
    git clone https://github.com/microsoft/vcpkg.git $vcpkgRoot
} else {
    Write-Host "vcpkg directory exists at $vcpkgRoot but vcpkg.exe is missing; reusing existing clone."
}

Write-Host "Bootstrapping vcpkg..."
& "$vcpkgRoot\bootstrap-vcpkg.bat"

if (!(Test-Path $vcpkgExe)) {
    throw "vcpkg bootstrap completed but $vcpkgExe was not created"
}

[Environment]::SetEnvironmentVariable('VCPKG_ROOT', $vcpkgRoot, 'User')

$userPath = [Environment]::GetEnvironmentVariable('Path', 'User')
$pathEntries = @()
if ($userPath) {
    $pathEntries = $userPath -split ';' | Where-Object { $_ -ne '' }
}
if ($pathEntries -notcontains $vcpkgRoot) {
    $pathEntries += $vcpkgRoot
    [Environment]::SetEnvironmentVariable('Path', ($pathEntries -join ';'), 'User')
}

Write-Host "vcpkg installation complete. VCPKG_ROOT set to $vcpkgRoot and PATH updated for future shells."
