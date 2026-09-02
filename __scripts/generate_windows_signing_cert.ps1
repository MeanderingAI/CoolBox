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

    function New-SecureStringFromPlainText {
        param(
            [string]$PlainText
        )

        $secureString = [System.Security.SecureString]::new()
        foreach ($character in $PlainText.ToCharArray()) {
            $secureString.AppendChar($character)
        }
        $secureString.MakeReadOnly()
        return $secureString
    }

    if ($Password) {
        return [pscustomobject]@{
            PlainText = $Password
            SecureString = (New-SecureStringFromPlainText -PlainText $Password)
        }
    }

    $generatedPassword = [guid]::NewGuid().ToString('N')
    return [pscustomobject]@{
        PlainText = $generatedPassword
        SecureString = (New-SecureStringFromPlainText -PlainText $generatedPassword)
    }
}

function New-CodeSigningCertificate {
    param(
        [string]$CertificateSubject,
        [string]$CertificateFriendlyName,
        [datetime]$CertificateNotAfter
    )

    $rsa = [System.Security.Cryptography.RSA]::Create(4096)

    try {
        $request = [System.Security.Cryptography.X509Certificates.CertificateRequest]::new(
            $CertificateSubject,
            $rsa,
            [System.Security.Cryptography.HashAlgorithmName]::SHA256,
            [System.Security.Cryptography.RSASignaturePadding]::Pkcs1
        )

        $request.CertificateExtensions.Add(
            [System.Security.Cryptography.X509Certificates.X509BasicConstraintsExtension]::new($false, $false, 0, $false)
        )
        $request.CertificateExtensions.Add(
            [System.Security.Cryptography.X509Certificates.X509KeyUsageExtension]::new(
                [System.Security.Cryptography.X509Certificates.X509KeyUsageFlags]::DigitalSignature,
                $true
            )
        )

        $ekuOids = [System.Security.Cryptography.OidCollection]::new()
        $ekuOids.Add([System.Security.Cryptography.Oid]::new('1.3.6.1.5.5.7.3.3', 'Code Signing')) | Out-Null
        $request.CertificateExtensions.Add(
            [System.Security.Cryptography.X509Certificates.X509EnhancedKeyUsageExtension]::new($ekuOids, $true)
        )
        $request.CertificateExtensions.Add(
            [System.Security.Cryptography.X509Certificates.X509SubjectKeyIdentifierExtension]::new($request.PublicKey, $false)
        )

        $notBefore = (Get-Date).AddMinutes(-5)
        $certificate = $request.CreateSelfSigned($notBefore, $CertificateNotAfter)
        $certificate.FriendlyName = $CertificateFriendlyName
        return $certificate
    }
    finally {
        $rsa.Dispose()
    }
}

function Export-CodeSigningCertificate {
    param(
        [System.Security.Cryptography.X509Certificates.X509Certificate2]$Certificate,
        [string]$PfxFilePath,
        [string]$CerFilePath,
        [string]$Password
    )

    $pfxBytes = $Certificate.Export([System.Security.Cryptography.X509Certificates.X509ContentType]::Pfx, $Password)
    [System.IO.File]::WriteAllBytes($PfxFilePath, $pfxBytes)

    $cerBytes = $Certificate.Export([System.Security.Cryptography.X509Certificates.X509ContentType]::Cert)
    [System.IO.File]::WriteAllBytes($CerFilePath, $cerBytes)
}

function Install-CertificateToTrustedPeople {
    param(
        [System.Security.Cryptography.X509Certificates.X509Certificate2]$Certificate
    )

    $store = [System.Security.Cryptography.X509Certificates.X509Store]::new(
        [System.Security.Cryptography.X509Certificates.StoreName]::TrustedPeople,
        [System.Security.Cryptography.X509Certificates.StoreLocation]::CurrentUser
    )

    try {
        $store.Open([System.Security.Cryptography.X509Certificates.OpenFlags]::ReadWrite)
        $store.Add($Certificate)
    }
    finally {
        $store.Close()
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
$certificate = New-CodeSigningCertificate `
    -CertificateSubject $Subject `
    -CertificateFriendlyName $FriendlyName `
    -CertificateNotAfter $notAfter

if (-not $certificate) {
    throw 'Failed to create the self-signed code-signing certificate.'
}

try {
    Export-CodeSigningCertificate -Certificate $certificate -PfxFilePath $pfxPath -CerFilePath $cerPath -Password $resolvedPassword.PlainText

    if ($InstallToTrustedPeople) {
        Install-CertificateToTrustedPeople -Certificate $certificate
        Write-Host '[generate_windows_signing_cert] Installed the public certificate into CurrentUser TrustedPeople'
    }
}
finally {
    $certificate.Dispose()
}

Write-Host '[generate_windows_signing_cert] Certificate generation complete.'
Write-Host ('[generate_windows_signing_cert] Exported PFX password: ' + $resolvedPassword.PlainText)
Write-Host '[generate_windows_signing_cert] Use these environment variables with the signing workflow:'
Write-Host ('$env:COOLBOX_WINDOWS_SIGN_CERT_PATH = "' + $pfxPath + '"')
Write-Host ('$env:COOLBOX_WINDOWS_SIGN_CERT_PASSWORD = "' + $resolvedPassword.PlainText + '"')

if ($InstallToTrustedPeople) {
    Write-Host '[generate_windows_signing_cert] This certificate will only be trusted on machines where its public cert is installed.'
}