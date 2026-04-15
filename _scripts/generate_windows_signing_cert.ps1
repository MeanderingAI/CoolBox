param(
    [string]$Subject = 'CN=CoolBox Dev Signing',

    [string]$FriendlyName = 'CoolBox Dev Signing',

    [string]$OutputDirectory = '',

    [string]$CertFileName = 'coolbox-dev-signing',

    [int]$ValidYears = 3,

    [string]$PfxPassword = '',

    [switch]$InstallToTrustedPeople,

    [switch]$PrintOnly
)

$ErrorActionPreference = 'Stop'

function Get-RepoRoot {
    $scriptDirectory = Split-Path -Parent $PSCommandPath
    return Split-Path -Parent $scriptDirectory
}

function Convert-ToSecurePassword {
    param(
        [string]$Password
    )

    if ($Password) {
        return [pscustomobject]@{
            PlainText = $Password
            SecureString = (ConvertTo-SecureString -String $Password -AsPlainText -Force)
        }
    }

    $generatedPassword = [guid]::NewGuid().ToString('N')
    return [pscustomobject]@{
        PlainText = $generatedPassword
        SecureString = (ConvertTo-SecureString -String $generatedPassword -AsPlainText -Force)
    }
}

if (-not $OutputDirectory) {
    $OutputDirectory = Join-Path (Get-RepoRoot) '.certs'
}

$baseName = $CertFileName -replace '[^0-9A-Za-z._-]', '-'
$pfxPath = Join-Path $OutputDirectory ($baseName + '.pfx')
$cerPath = Join-Path $OutputDirectory ($baseName + '.cer')

$resolvedPassword = Convert-ToSecurePassword -Password $PfxPassword

Write-Host "[generate_windows_signing_cert] Subject: $Subject"
Write-Host "[generate_windows_signing_cert] FriendlyName: $FriendlyName"
Write-Host "[generate_windows_signing_cert] Output directory: $OutputDirectory"
Write-Host "[generate_windows_signing_cert] PFX path: $pfxPath"
Write-Host "[generate_windows_signing_cert] CER path: $cerPath"
Write-Host "[generate_windows_signing_cert] Valid years: $ValidYears"
if (-not $PfxPassword) {
    Write-Host '[generate_windows_signing_cert] No password supplied; generated a random PFX password for this export.'
}

if ($PrintOnly) {
    Write-Host ''
    Write-Host '[generate_windows_signing_cert] Environment variable example:'
    Write-Host ('$env:COOLBOX_WINDOWS_SIGN_CERT_PATH = "' + $pfxPath + '"')
    Write-Host ('$env:COOLBOX_WINDOWS_SIGN_CERT_PASSWORD = "' + $resolvedPassword.PlainText + '"')
    return
}

New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null

$notAfter = (Get-Date).AddYears($ValidYears)
$certificate = New-SelfSignedCertificate `
    -Subject $Subject `
    -FriendlyName $FriendlyName `
    -Type CodeSigningCert `
    -KeyAlgorithm RSA `
    -KeyLength 4096 `
    -HashAlgorithm SHA256 `
    -CertStoreLocation 'Cert:\CurrentUser\My' `
    -NotAfter $notAfter `
    -KeyExportPolicy Exportable

if (-not $certificate) {
    throw 'Failed to create the self-signed code-signing certificate.'
}

Export-PfxCertificate -Cert $certificate -FilePath $pfxPath -Password $resolvedPassword.SecureString | Out-Null
Export-Certificate -Cert $certificate -FilePath $cerPath | Out-Null

if ($InstallToTrustedPeople) {
    Import-Certificate -FilePath $cerPath -CertStoreLocation 'Cert:\CurrentUser\TrustedPeople' | Out-Null
    Write-Host '[generate_windows_signing_cert] Installed the public certificate into Cert:\CurrentUser\TrustedPeople'
}

Write-Host '[generate_windows_signing_cert] Certificate generation complete.'
Write-Host ('[generate_windows_signing_cert] Exported PFX password: ' + $resolvedPassword.PlainText)
Write-Host '[generate_windows_signing_cert] Use these environment variables with the signing workflow:'
Write-Host ('$env:COOLBOX_WINDOWS_SIGN_CERT_PATH = "' + $pfxPath + '"')
Write-Host ('$env:COOLBOX_WINDOWS_SIGN_CERT_PASSWORD = "' + $resolvedPassword.PlainText + '"')

if ($InstallToTrustedPeople) {
    Write-Host '[generate_windows_signing_cert] This certificate will only be trusted on machines where its public cert is installed.'
}