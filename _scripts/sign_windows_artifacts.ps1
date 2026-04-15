param(
    [string]$Root = 'build',
    [string]$Configuration = 'Release',
    [string]$TimestampUrl = '',
    [string]$CertificatePath = '',
    [string]$CertificatePassword = ''
)

$ErrorActionPreference = 'Stop'

function Get-ConfiguredValue {
    param(
        [string]$ExplicitValue,
        [string]$EnvironmentName
    )

    if ($ExplicitValue) {
        return $ExplicitValue
    }

    $environmentValue = [Environment]::GetEnvironmentVariable($EnvironmentName)
    if ($environmentValue) {
        return $environmentValue
    }

    return ''
}

$resolvedRoot = Get-ConfiguredValue -ExplicitValue $Root -EnvironmentName 'COOLBOX_WINDOWS_SIGN_ROOT'
if (-not $resolvedRoot) {
    $resolvedRoot = 'build'
}

$resolvedConfiguration = Get-ConfiguredValue -ExplicitValue $Configuration -EnvironmentName 'COOLBOX_WINDOWS_SIGN_CONFIGURATION'
if (-not $resolvedConfiguration) {
    $resolvedConfiguration = 'Release'
}

$resolvedTimestampUrl = Get-ConfiguredValue -ExplicitValue $TimestampUrl -EnvironmentName 'COOLBOX_WINDOWS_SIGN_TIMESTAMP_URL'
if (-not $resolvedTimestampUrl) {
    $resolvedTimestampUrl = 'http://timestamp.digicert.com'
}

$resolvedCertificatePath = Get-ConfiguredValue -ExplicitValue $CertificatePath -EnvironmentName 'COOLBOX_WINDOWS_SIGN_CERT_PATH'
$resolvedCertificatePassword = Get-ConfiguredValue -ExplicitValue $CertificatePassword -EnvironmentName 'COOLBOX_WINDOWS_SIGN_CERT_PASSWORD'

if (-not (Test-Path $resolvedRoot)) {
    Write-Host "[sign_windows_artifacts] Root path not found, skipping signing: $resolvedRoot"
    exit 0
}

if (-not $resolvedCertificatePath) {
    Write-Host '[sign_windows_artifacts] No certificate configured. Set COOLBOX_WINDOWS_SIGN_CERT_PATH to enable signing.'
    exit 0
}

if (-not (Test-Path $resolvedCertificatePath)) {
    throw "Configured certificate path does not exist: $resolvedCertificatePath"
}

$signtool = Get-Command signtool.exe -ErrorAction SilentlyContinue
if (-not $signtool) {
    $signtool = Get-Command signtool -ErrorAction SilentlyContinue
}

if (-not $signtool) {
    throw 'signtool.exe was not found. Install the Windows SDK signing tools to enable publisher signing.'
}

$artifactFiles = Get-ChildItem -Path $resolvedRoot -Recurse -File |
    Where-Object {
        $_.Extension -in '.exe', '.dll' -and (
            $_.DirectoryName.EndsWith("\$resolvedConfiguration") -or
            $_.DirectoryName.EndsWith("/$resolvedConfiguration")
        )
    } |
    Sort-Object FullName -Unique

if (-not $artifactFiles) {
    Write-Host "[sign_windows_artifacts] No .$resolvedConfiguration executables or DLLs found under $resolvedRoot"
    exit 0
}

foreach ($artifact in $artifactFiles) {
    $arguments = @(
        'sign',
        '/fd', 'SHA256',
        '/td', 'SHA256',
        '/f', $resolvedCertificatePath
    )

    if ($resolvedCertificatePassword) {
        $arguments += @('/p', $resolvedCertificatePassword)
    }

    if ($resolvedTimestampUrl) {
        $arguments += @('/tr', $resolvedTimestampUrl)
    }

    $arguments += $artifact.FullName

    Write-Host "[sign_windows_artifacts] Signing $($artifact.FullName)"
    & $signtool.Source @arguments
    if ($LASTEXITCODE -ne 0) {
        throw "signtool failed for $($artifact.FullName) with exit code $LASTEXITCODE"
    }
}

Write-Host "[sign_windows_artifacts] Signed $($artifactFiles.Count) artifact(s)."