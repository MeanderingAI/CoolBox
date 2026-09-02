# Permission Scripts

Scripts to manage application permissions and AppLocker whitelisting.

## whitelist_pip.ps1

Permanently whitelists `pip.exe` in Windows AppLocker to allow installation of Python dependencies.

### Usage

**Method 1: Basic usage (uses default path)**
```powershell
# Right-click PowerShell and select "Run as Administrator", then:
.\_scripts\permission_scripts\whitelist_pip.ps1
```

**Method 2: Custom venv path**
```powershell
.\_scripts\permission_scripts\whitelist_pip.ps1 -VenvPath "C:\path\to\your\.venv\Scripts\pip.exe"
```

### Requirements

- **Administrator privileges** - Must run PowerShell as Administrator
- Windows 10 or later with AppLocker enabled
- The virtual environment must already exist

### What it does

1. Checks current AppLocker policy
2. Creates a whitelist rule for pip.exe
3. Merges the rule with existing AppLocker policy
4. Verifies the rule was successfully added

### Troubleshooting

**"This script must be run as Administrator"**
- Right-click PowerShell and select "Run as Administrator"

**"No AppLocker policy found"**
- This is normal if AppLocker is not yet configured
- The script will create a new policy

**Still getting "Application Control policy has blocked this file"**
- Your system may use WDAC instead of AppLocker
- Contact your IT administrator for whitelisting assistance
- Provide them: `C:\KEYS\CoolBox\.venv\Scripts\pip.exe`

### Verify Installation

After running the script, test if pip works:
```powershell
pip install -r requirements.txt
```
