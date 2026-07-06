# _scripts/install_scripts/install_docker.ps1
# Installs Docker Desktop on Windows

Write-Host "Docker Desktop Installer for Windows" -ForegroundColor Cyan
Write-Host "=====================================" -ForegroundColor Cyan
Write-Host ""

# Check if Docker is already installed
if (Get-Command docker -ErrorAction SilentlyContinue) {
    Write-Host "Docker is already installed!" -ForegroundColor Green
    docker --version
    Write-Host ""
    docker version 2>$null
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✓ Docker is running" -ForegroundColor Green
    } else {
        Write-Host "⚠ Docker is installed but not running" -ForegroundColor Yellow
        Write-Host "Please start Docker Desktop from the Start menu" -ForegroundColor Yellow
    }
    exit 0
}

Write-Host "Docker Desktop is not installed." -ForegroundColor Yellow
Write-Host ""

# Check for Winget
if (Get-Command winget -ErrorAction SilentlyContinue) {
    Write-Host "Using Winget to install Docker Desktop..." -ForegroundColor Green
    winget install --id Docker.DockerDesktop --accept-package-agreements --accept-source-agreements
    if ($LASTEXITCODE -eq 0) {
        Write-Host ""
        Write-Host "✓ Docker Desktop installed!" -ForegroundColor Green
        Write-Host "Please start Docker Desktop from the Start menu" -ForegroundColor Yellow
        exit 0
    }
}

Write-Host "Manual installation required:" -ForegroundColor Yellow
Write-Host "https://www.docker.com/products/docker-desktop" -ForegroundColor Cyan
Start-Process "https://www.docker.com/products/docker-desktop"
exit 1
