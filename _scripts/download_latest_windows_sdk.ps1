param(
    [ValidateSet('Windows11', 'Windows10')]
    [string]$Channel = 'Windows11',

    [ValidateSet('Installer', 'ISO')]
    [string]$Artifact = 'Installer',

    [string]$OutputDirectory = '',

    [switch]$StartAfterDownload,

    [switch]$PrintOnly,

    [switch]$PassThru
)

$ErrorActionPreference = 'Stop'

$learnPageUrl = 'https://learn.microsoft.com/en-us/windows/apps/windows-sdk/downloads'
$markdownUrl = 'https://raw.githubusercontent.com/MicrosoftDocs/windows-dev-docs/refs/heads/docs/hub/apps/windows-sdk/downloads.md'

function Normalize-Link {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Url
    )

    $trimmed = $Url.Trim()
    if ($trimmed.StartsWith('//')) {
        return "https:$trimmed"
    }
    return $trimmed
}

function Get-SectionText {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Markdown,

        [Parameter(Mandatory = $true)]
        [string]$Heading
    )

    $pattern = "(?ms)^##\s+$([regex]::Escape($Heading))\s*$.*?(?=^##\s+|\z)"
    $match = [regex]::Match($Markdown, $pattern)
    if (-not $match.Success) {
        throw "Unable to find section '$Heading' in Windows SDK markdown source."
    }

    return $match.Value
}

function Get-LatestSdkEntry {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Markdown,

        [Parameter(Mandatory = $true)]
        [string]$SectionHeading
    )

    $section = Get-SectionText -Markdown $Markdown -Heading $SectionHeading
    $lines = $section -split "`r?`n"
    foreach ($line in $lines) {
        if (-not $line.TrimStart().StartsWith('| **Windows SDK')) {
            continue
        }

        $releaseMatch = [regex]::Match($line, '\*\*(?<release>.*?)\*\*')
        $installerMatch = [regex]::Match($line, '\[Installer\]\((?<url>[^)]+)\)')
        $isoMatch = [regex]::Match($line, '\[ISO\]\((?<url>[^)]+)\)')

        if (-not $releaseMatch.Success -or -not $installerMatch.Success) {
            continue
        }

        $versionMatch = [regex]::Match($releaseMatch.Groups['release'].Value, '\((?<version>[^)]+)\)')
        $version = if ($versionMatch.Success) { $versionMatch.Groups['version'].Value.Trim() } else { 'unknown-version' }

        return [pscustomobject]@{
            ReleaseTitle = $releaseMatch.Groups['release'].Value.Trim()
            Version = $version
            InstallerUrl = Normalize-Link -Url $installerMatch.Groups['url'].Value
            IsoUrl = if ($isoMatch.Success) { Normalize-Link -Url $isoMatch.Groups['url'].Value } else { '' }
            Section = $SectionHeading
        }
    }

    throw "Unable to parse the latest SDK entry from section '$SectionHeading'."
}

function Get-ArtifactPath {
    param(
        [Parameter(Mandatory = $true)]
        [pscustomobject]$Entry,

        [Parameter(Mandatory = $true)]
        [string]$ArtifactKind,

        [Parameter(Mandatory = $true)]
        [string]$DestinationDirectory
    )

    $safeVersion = ($Entry.Version -replace '[^0-9A-Za-z._-]', '_')
    switch ($ArtifactKind) {
        'Installer' { return Join-Path $DestinationDirectory "windows-sdk-$safeVersion-installer.exe" }
        'ISO' { return Join-Path $DestinationDirectory "windows-sdk-$safeVersion.iso" }
        default { throw "Unsupported artifact kind: $ArtifactKind" }
    }
}

Write-Host "[download_latest_windows_sdk] Fetching Windows SDK release metadata from $learnPageUrl"
$markdown = (Invoke-WebRequest -UseBasicParsing $markdownUrl).Content

$sectionHeading = if ($Channel -eq 'Windows11') { 'Windows 11' } else { 'Windows 10' }
$latestEntry = Get-LatestSdkEntry -Markdown $markdown -SectionHeading $sectionHeading

$selectedUrl = if ($Artifact -eq 'Installer') { $latestEntry.InstallerUrl } else { $latestEntry.IsoUrl }
if (-not $selectedUrl) {
    throw "The latest $sectionHeading entry does not expose a $Artifact link."
}

if (-not $OutputDirectory) {
    $OutputDirectory = Join-Path $env:TEMP 'coolbox-downloads'
}

New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
$destinationPath = Get-ArtifactPath -Entry $latestEntry -ArtifactKind $Artifact -DestinationDirectory $OutputDirectory

Write-Host "[download_latest_windows_sdk] Latest section: $($latestEntry.Section)"
Write-Host "[download_latest_windows_sdk] Latest release: $($latestEntry.ReleaseTitle)"
Write-Host "[download_latest_windows_sdk] Version: $($latestEntry.Version)"
Write-Host "[download_latest_windows_sdk] $Artifact URL: $selectedUrl"
Write-Host "[download_latest_windows_sdk] Destination: $destinationPath"

$result = [pscustomobject]@{
    Channel = $Channel
    Artifact = $Artifact
    Section = $latestEntry.Section
    ReleaseTitle = $latestEntry.ReleaseTitle
    Version = $latestEntry.Version
    Url = $selectedUrl
    DestinationPath = $destinationPath
}

if ($PrintOnly) {
    if ($PassThru) {
        $result
    }
    return
}

Invoke-WebRequest -UseBasicParsing -Uri $selectedUrl -OutFile $destinationPath
Write-Host "[download_latest_windows_sdk] Download complete."

if ($StartAfterDownload) {
    Write-Host "[download_latest_windows_sdk] Launching $destinationPath"
    Start-Process -FilePath $destinationPath
}

if ($PassThru) {
    $result
}