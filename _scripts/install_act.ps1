# _scripts/install_act.ps1
# Installs the 'act' GitHub Actions runner on Windows using Scoop or direct download

# Try Scoop first
if (Get-Command scoop -ErrorAction SilentlyContinue) {
    Write-Host "Scoop detected. Installing act via Scoop..."
    scoop install act
    exit 0
}


# If Scoop is not available, download act from GitHub Releases
$actVersion = "v0.2.61"
$arch = if ([Environment]::Is64BitOperatingSystem) { "Windows_x86_64" } else { "Windows_x86_32" }
$actUrl = "https://github.com/nektos/act/releases/download/$actVersion/act_${arch}.zip"
$zipPath = "$env:TEMP\act.zip"
$destDir = "$env:USERPROFILE\.local\bin"

Write-Host "Downloading act from $actUrl..."
Invoke-WebRequest -Uri $actUrl -OutFile $zipPath

Write-Host "Extracting act..."
Expand-Archive -Path $zipPath -DestinationPath $destDir -Force

$actExe = Join-Path $destDir "act.exe"
if (-not (Test-Path $actExe)) {
    Write-Error "act.exe not found after extraction."
    exit 1
}

# Add to PATH for current session
$env:PATH = "$destDir;" + $env:PATH
Write-Host "'act' installed to $destDir. You may want to add this to your system PATH."

# Clean up
Remove-Item $zipPath -Force

# Test installation
Write-Host "Testing 'act' installation..."
act --version
