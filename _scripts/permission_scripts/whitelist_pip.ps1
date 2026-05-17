# Whitelist pip.exe in AppLocker
# Run this script with Administrator privileges

param(
    [string]$VenvPath = "C:\KEYS\CoolBox\.venv\Scripts\pip.exe"
)

# Check if running as Administrator
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")

if (-not $isAdmin) {
    Write-Host "ERROR: This script must be run as Administrator!" -ForegroundColor Red
    Write-Host "Please right-click PowerShell and select 'Run as Administrator'" -ForegroundColor Yellow
    exit 1
}

Write-Host "Starting pip.exe whitelisting process..." -ForegroundColor Green
Write-Host "Target: $VenvPath" -ForegroundColor Cyan

# Step 1: Check current AppLocker policy
Write-Host "`n[1/4] Checking current AppLocker policy..." -ForegroundColor Yellow
try {
    $policy = Get-AppLockerPolicy -Effective
    if ($null -eq $policy) {
        Write-Host "No AppLocker policy found. Creating new policy..." -ForegroundColor Yellow
    } else {
        Write-Host "Existing AppLocker policy found." -ForegroundColor Green
    }
} catch {
    Write-Host "Warning: Could not retrieve AppLocker policy: $_" -ForegroundColor Yellow
}

# Step 2: Create rule for pip.exe
Write-Host "`n[2/4] Creating AppLocker rule for pip.exe..." -ForegroundColor Yellow
try {
    $rule = New-AppLockerRuleObject -ExecutablePath $VenvPath -Action Allow -User (New-Object System.Security.Principal.SecurityIdentifier("S-1-1-0"))
    Write-Host "Rule created successfully." -ForegroundColor Green
} catch {
    Write-Host "Error creating rule: $_" -ForegroundColor Red
    exit 1
}

# Step 3: Merge rule with existing policy
Write-Host "`n[3/4] Adding rule to AppLocker policy..." -ForegroundColor Yellow
try {
    $policy | Set-AppLockerPolicy -RuleObject $rule -Merge
    Write-Host "Rule added successfully." -ForegroundColor Green
} catch {
    Write-Host "Error adding rule: $_" -ForegroundColor Red
    exit 1
}

# Step 4: Verify rule was added
Write-Host "`n[4/4] Verifying pip.exe is whitelisted..." -ForegroundColor Yellow
try {
    $updatedPolicy = Get-AppLockerPolicy -Effective
    $pipRules = $updatedPolicy.RuleCollections | Where-Object { $_ -match $VenvPath -or $_ -match "pip.exe" }
    if ($null -ne $pipRules) {
        Write-Host "SUCCESS: pip.exe has been whitelisted!" -ForegroundColor Green
    } else {
        Write-Host "Rule added, but verification inconclusive. You may need to restart your system." -ForegroundColor Yellow
    }
} catch {
    Write-Host "Warning: Could not verify rule: $_" -ForegroundColor Yellow
    Write-Host "Rule was likely added successfully. You may need to restart your system." -ForegroundColor Yellow
}

Write-Host "`n✓ Whitelist process completed!" -ForegroundColor Green
Write-Host "If you continue to see errors, contact your IT administrator." -ForegroundColor Cyan
