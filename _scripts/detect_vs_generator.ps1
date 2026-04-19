# PowerShell script to robustly detect the latest Visual Studio generator
# Usage: powershell -File _scripts/detect_vs_generator.ps1

$generators = & cmake --help | Select-String 'Visual Studio [0-9]+ [0-9]+' -AllMatches | ForEach-Object { $_.Matches.Value }
$parsed = $generators | ForEach-Object {
    if ($_ -match 'Visual Studio ([0-9]+) ([0-9]+)') {
        [PSCustomObject]@{
            Name = $_
            Major = [int]$matches[1]
            Minor = [int]$matches[2]
        }
    }
}
$latest = $parsed | Sort-Object Major, Minor -Descending | Select-Object -First 1
if ($latest) {
    Write-Output $latest.Name
    exit 0
} else {
    Write-Error "No Visual Studio generator found."
    exit 1
}
