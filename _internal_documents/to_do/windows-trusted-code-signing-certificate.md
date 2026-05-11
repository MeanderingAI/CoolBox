# Windows Trusted Code-Signing Certificate Acquisition

## Goal

Acquire a publicly trusted Windows code-signing certificate that can be used to sign CoolBox release binaries so Windows can identify the publisher without relying on a self-signed certificate.

## Chosen Approach

CoolBox release signing uses an exportable PFX delivered through GitHub Actions repository secrets.

- Acquire a CA-issued Authenticode certificate that can be exported as a password-protected PFX.
- Store the base64-encoded PFX in `COOLBOX_WINDOWS_SIGN_CERT_BASE64`.
- Store the PFX password in `COOLBOX_WINDOWS_SIGN_CERT_PASSWORD`.
- Keep `COOLBOX_WINDOWS_SIGN_TIMESTAMP_URL` set to the CA-recommended timestamp service when available.

## What You Need

- A legal entity or individual identity that matches the publisher name you want Windows to display.
- A certificate authority that issues Authenticode code-signing certificates.
- A certificate offering that can be exported as a PFX for CI use.
- A timestamp service URL so signatures remain valid after the certificate expires.

## Certificate Options

1. Standard code-signing certificate.
   The preferred fit for this repository if the CA allows exportable PFX delivery. SmartScreen reputation usually has to build over time.
2. Extended Validation (EV) code-signing certificate.
   Stronger identity checks and typically better SmartScreen treatment, but only usable with the current workflow if the EV certificate can still be exported as a PFX.

## Typical Providers To Evaluate

1. DigiCert
2. Sectigo
3. GlobalSign
4. SSL.com

Evaluate pricing, regional availability, EV support, timestamp endpoints, and whether exportable PFX delivery is allowed.

## Purchase And Issuance Flow

1. Choose whether the certificate should be issued to an individual or a business entity.
2. Complete the certificate authority identity verification process.
3. Confirm with the provider that the certificate can be exported as a password-protected PFX for CI use.
4. Export the issued certificate as a password-protected PFX.
5. Record the timestamp URL recommended by the provider.
6. Verify the certificate locally with `signtool verify /pa` after signing a test executable.

## GitHub Actions Preparation

1. Export the release PFX.
2. Base64-encode the PFX.
3. Store the base64 output in the repository secret `COOLBOX_WINDOWS_SIGN_CERT_BASE64`.
4. Store the PFX password in the repository secret `COOLBOX_WINDOWS_SIGN_CERT_PASSWORD`.
5. Optionally store a timestamp endpoint in `COOLBOX_WINDOWS_SIGN_TIMESTAMP_URL`.

Example PowerShell to produce the base64 payload:

```powershell
$pfxBytes = [System.IO.File]::ReadAllBytes('C:\path\to\coolbox-release-signing.pfx')
[Convert]::ToBase64String($pfxBytes) | Set-Clipboard
```

Repository helper:

```powershell
.\_scripts\publish_windows_signing_secrets.ps1 -PfxPath C:\path\to\coolbox-release-signing.pfx -PfxPassword 'replace-with-real-password' -TriggerWorkflow -Workflow build-products.yaml
```

The helper requires GitHub CLI authenticated against the target repository and updates the three expected repository secrets before optionally dispatching a workflow run.

## Local Validation Checklist

1. Install the Windows SDK signing tools if `signtool.exe` is not already present.
2. Set `COOLBOX_WINDOWS_SIGN_CERT_PATH` to the PFX path.
3. Set `COOLBOX_WINDOWS_SIGN_CERT_PASSWORD` to the PFX password.
4. Optionally set `COOLBOX_WINDOWS_SIGN_TIMESTAMP_URL`.
5. Run `make -f Makefile.win sign_windows_artifacts` after a Release build.
6. Verify a signed binary with `signtool verify /pa path\to\binary.exe`.

## Security Notes

- Restrict access to the exportable PFX and its password to the smallest set of maintainers possible.
- Rotate the certificate before expiration and update the GitHub Actions secrets promptly.
- Restrict access to repository secrets to the smallest set of maintainers possible.
- Prefer a CA offering with clear certificate reissue and revocation procedures in case the PFX is exposed.

## Current Decision

CoolBox releases use the exportable PFX workflow. If a provider only supports hardware-token or managed cloud signing, choose a different provider or plan a separate workflow redesign instead of forcing that model into the current release pipeline.

## Related Guides

- See `to_do/macos-release-signing-and-notarization.md` for the corresponding macOS release-signing and notarization setup guide.
- See `to_do/linux-release-signing.md` for the corresponding Linux release-signing guide.