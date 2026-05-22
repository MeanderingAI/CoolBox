# install_doxygen_windows.ps1
# Silent / non-interactive Doxygen install for Windows.
# Tries (in order): winget → Chocolatey → direct NSIS silent download.
# Called by _scripts/script_library_runner.py when doxygen is not on PATH.

$ErrorActionPreference = 'Stop'
$DoxygenBin = "C:\Program Files\doxygen\bin"

function Add-DoxygenToPath {
    if (Test-Path $DoxygenBin) {
        $machine = [System.Environment]::GetEnvironmentVariable("Path", "Machine")
        if ($machine -notlike "*$DoxygenBin*") {
            Write-Host "[install_doxygen] Adding $DoxygenBin to system PATH..."
            [System.Environment]::SetEnvironmentVariable(
                "Path", "$machine;$DoxygenBin", "Machine"
            )
            Write-Host "[install_doxygen] PATH updated (restart your terminal to pick it up)."
        } else {
            Write-Host "[install_doxygen] $DoxygenBin already in system PATH."
        }
        # Also inject into the current process so the caller can use it immediately
        $env:PATH = "$DoxygenBin;$env:PATH"
    } else {
        Write-Warning "[install_doxygen] Expected bin dir '$DoxygenBin' not found after install."
    }
}

# ── 1. winget (Windows 10 1709+ package manager, no admin required for user scope) ──
if (Get-Command winget -ErrorAction SilentlyContinue) {
    Write-Host "[install_doxygen] Using winget..."
    winget install --id doxygen.doxygen --exact --silent `
        --accept-package-agreements --accept-source-agreements
    if ($LASTEXITCODE -eq 0) {
        Add-DoxygenToPath
        Write-Host "[install_doxygen] Doxygen installed via winget."
        exit 0
    }
    Write-Warning "[install_doxygen] winget install returned $LASTEXITCODE, trying next method."
}

# ── 2. Chocolatey ────────────────────────────────────────────────────────────
if (Get-Command choco -ErrorAction SilentlyContinue) {
    Write-Host "[install_doxygen] Using Chocolatey..."
    choco install doxygen.install -y --no-progress
    if ($LASTEXITCODE -eq 0) {
        Add-DoxygenToPath
        Write-Host "[install_doxygen] Doxygen installed via Chocolatey."
        exit 0
    }
    Write-Warning "[install_doxygen] choco install returned $LASTEXITCODE, trying direct download."
}

# ── 3. Direct NSIS silent download ──────────────────────────────────────────
$Version  = "1.12.0"
$Url      = "https://www.doxygen.nl/files/doxygen-$Version-setup.exe"
$Dest     = "$env:TEMP\doxygen-$Version-setup.exe"

Write-Host "[install_doxygen] Downloading Doxygen $Version from $Url..."
Invoke-WebRequest -Uri $Url -OutFile $Dest -UseBasicParsing

Write-Host "[install_doxygen] Running silent installer (/S flag)..."
# NSIS installers accept /S for fully silent mode
Start-Process -FilePath $Dest -ArgumentList "/S" -Wait -NoNewWindow

Remove-Item $Dest -ErrorAction SilentlyContinue

Add-DoxygenToPath

if (Get-Command doxygen -ErrorAction SilentlyContinue) {
    $ver = & doxygen --version
    Write-Host "[install_doxygen] Doxygen $ver installed successfully."
    exit 0
} else {
    Write-Warning "[install_doxygen] doxygen not detected on PATH after install."
    Write-Warning "You may need to restart the terminal/server to pick up the new PATH."
    exit 1
}
