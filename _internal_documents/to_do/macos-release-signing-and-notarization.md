# macOS Release Signing And Notarization

## Goal

Set up a reproducible macOS release-signing flow so CoolBox macOS products can be distributed with Apple-trusted signatures and pass Gatekeeper checks after notarization.

## Recommended Approach

CoolBox macOS releases should use Apple Developer ID signing plus notarization.

- Use a `Developer ID Application` certificate for `.app`, standalone binaries, dynamic libraries, and other executable payloads.
- Use a `Developer ID Installer` certificate only if CoolBox starts shipping signed `.pkg` installers.
- Use `xcrun notarytool` with an App Store Connect API key for CI notarization.
- Staple the notarization ticket to the distributed app or archive before publishing release assets.

This is the closest macOS equivalent to the Windows trusted-signing workflow already documented in `to_do/windows-trusted-code-signing-certificate.md`.

## Related Guides

- See `to_do/windows-trusted-code-signing-certificate.md` for the Windows release-signing setup.
- See `to_do/linux-release-signing.md` for Linux release-signing options and CI setup.

## What You Need

- An Apple Developer Program membership for the publisher identity that should appear in macOS trust dialogs.
- Access to Certificates, Identifiers & Profiles in the Apple Developer account.
- A machine with Keychain Access or Xcode tools available to export signing identities as password-protected `.p12` files.
- An App Store Connect API key with permission to submit notarization requests.
- A clear decision about which macOS artifacts will be shipped as signed deliverables:
  - `.app` bundles for products such as MStudio, file browser, or bower shell
  - standalone command-line binaries
  - `.dmg` or `.zip` archives containing signed apps
  - optional future `.pkg` installers

## Certificates And Credentials To Acquire

### 1. Developer ID Application certificate

Use this to sign:

- `.app` bundles
- Mach-O executables
- `.dylib` files
- embedded helper tools

Export the certificate and private key as a password-protected `.p12`.

### 2. Developer ID Installer certificate

Only required if CoolBox begins distributing `.pkg` installers through `productsign`.

### 3. App Store Connect API key

Create an API key for `notarytool` and record:

- Key ID
- Issuer ID
- the `.p8` private key file

This is preferable to Apple ID plus app-specific password for CI.

## Suggested GitHub Actions Secrets

Store macOS signing material in repository or environment secrets.

- `COOLBOX_MACOS_SIGN_APP_CERT_BASE64`
  Base64-encoded `Developer ID Application` `.p12` file.
- `COOLBOX_MACOS_SIGN_APP_CERT_PASSWORD`
  Password used to export the application signing `.p12`.
- `COOLBOX_MACOS_SIGN_INSTALLER_CERT_BASE64`
  Base64-encoded `Developer ID Installer` `.p12` file if installer signing is needed later.
- `COOLBOX_MACOS_SIGN_INSTALLER_CERT_PASSWORD`
  Password for the installer `.p12` if used.
- `COOLBOX_MACOS_KEYCHAIN_PASSWORD`
  Temporary CI keychain password.
- `COOLBOX_MACOS_NOTARY_KEY_ID`
  App Store Connect API key ID.
- `COOLBOX_MACOS_NOTARY_ISSUER_ID`
  App Store Connect issuer ID.
- `COOLBOX_MACOS_NOTARY_API_KEY_BASE64`
  Base64-encoded contents of the `.p8` notary API key.
- `COOLBOX_MACOS_TEAM_ID`
  Apple Developer Team ID used for signing identities and notarization.

## Local Export Preparation

### Export the signing certificate

1. Import or create the `Developer ID Application` certificate in the macOS login keychain.
2. In Keychain Access, export the certificate plus private key as `coolbox-macos-app-signing.p12`.
3. Protect the export with a strong password.
4. Base64-encode the `.p12` for GitHub secret storage.

Example:

```bash
base64 -i coolbox-macos-app-signing.p12 | pbcopy
```

### Export the notary API key

1. Create an App Store Connect API key.
2. Download the `.p8` file once.
3. Base64-encode the file contents for secret storage.

Example:

```bash
base64 -i AuthKey_ABC123XYZ.p8 | pbcopy
```

## CI Setup Pattern

The current repository already builds macOS artifacts in workflows such as:

- `.github/workflows/build-products.yaml`
- `.github/workflows/build-libs.yaml`
- `generate-purchase-*` workflows for language bindings

No macOS signing or notarization step exists yet. Add one after the macOS artifact build step and before the final archive upload step.

Recommended CI sequence:

1. Decode the `.p12` and `.p8` secrets into temporary files under `${RUNNER_TEMP}`.
2. Create a temporary keychain.
3. Import the `Developer ID Application` certificate into that keychain.
4. Unlock the keychain and allow `/usr/bin/codesign` access to the private key.
5. Sign the built app bundle or executable payloads with `codesign --force --options runtime --timestamp`.
6. Build the distributable archive, typically `.zip` or `.dmg`, from already signed content.
7. Submit the archive to Apple notarization with `xcrun notarytool submit --wait`.
8. Staple the notarization ticket with `xcrun stapler staple` when the output type supports stapling.
9. Verify the final artifact with `codesign`, `spctl`, and optionally `stapler validate`.

## Example CI Commands

### Create and prepare a temporary keychain

```bash
KEYCHAIN_PATH="$RUNNER_TEMP/coolbox-signing.keychain-db"
CERT_PATH="$RUNNER_TEMP/coolbox-macos-app-signing.p12"

echo "$COOLBOX_MACOS_SIGN_APP_CERT_BASE64" | base64 --decode > "$CERT_PATH"

security create-keychain -p "$COOLBOX_MACOS_KEYCHAIN_PASSWORD" "$KEYCHAIN_PATH"
security set-keychain-settings -lut 21600 "$KEYCHAIN_PATH"
security unlock-keychain -p "$COOLBOX_MACOS_KEYCHAIN_PASSWORD" "$KEYCHAIN_PATH"
security import "$CERT_PATH" -k "$KEYCHAIN_PATH" -P "$COOLBOX_MACOS_SIGN_APP_CERT_PASSWORD" -T /usr/bin/codesign -T /usr/bin/security
security list-keychains -d user -s "$KEYCHAIN_PATH" login.keychain-db
security set-key-partition-list -S apple-tool:,apple:,codesign: -s -k "$COOLBOX_MACOS_KEYCHAIN_PASSWORD" "$KEYCHAIN_PATH"
```

### Sign an app bundle

```bash
codesign --force --deep --options runtime --timestamp \
  --sign "Developer ID Application: CoolBox Publisher Name ($COOLBOX_MACOS_TEAM_ID)" \
  build/_Product/MStudio/Release/MStudio.app
```

Use `--deep` cautiously. Prefer signing nested frameworks, helpers, and dylibs explicitly if the final bundle layout becomes more complex.

### Package and notarize

```bash
ditto -c -k --keepParent build/_Product/MStudio/Release/MStudio.app MStudio-macos.zip

NOTARY_KEY_PATH="$RUNNER_TEMP/AuthKey_$COOLBOX_MACOS_NOTARY_KEY_ID.p8"
echo "$COOLBOX_MACOS_NOTARY_API_KEY_BASE64" | base64 --decode > "$NOTARY_KEY_PATH"

xcrun notarytool submit MStudio-macos.zip \
  --key "$NOTARY_KEY_PATH" \
  --key-id "$COOLBOX_MACOS_NOTARY_KEY_ID" \
  --issuer "$COOLBOX_MACOS_NOTARY_ISSUER_ID" \
  --wait

xcrun stapler staple build/_Product/MStudio/Release/MStudio.app
spctl --assess --type exec --verbose build/_Product/MStudio/Release/MStudio.app
```

## Where To Integrate In This Repository

### Product releases

The best initial integration point is the macOS branch of `.github/workflows/build-products.yaml`.

- Build the product targets.
- Sign the `.app` bundles for `MStudio`, `file_browser`, and `bower_shell` if those targets emit app bundles.
- Notarize the final `.zip` or `.dmg` assets before upload.

### Language extension releases

Not every language-binding artifact needs notarization.

- Pure source archives typically do not.
- `.dylib` payloads intended for direct end-user execution or system loading may still need signing depending on distribution expectations.
- If a binding ships a macOS app bundle, framework, or installer, apply the same signing and notarization flow.

## Local Validation Checklist

1. Confirm the identity is visible:

```bash
security find-identity -v -p codesigning
```

2. Sign the app or executable locally with the Developer ID Application identity.
3. Verify the signature:

```bash
codesign --verify --deep --strict --verbose=2 path/to/MyApp.app
codesign -dv --verbose=4 path/to/MyApp.app
```

4. Verify Gatekeeper acceptance:

```bash
spctl --assess --type exec --verbose path/to/MyApp.app
```

5. After notarization, validate stapling when applicable:

```bash
xcrun stapler validate path/to/MyApp.app
```

## Security Notes

- Restrict access to the `.p12` export, `.p8` notary key, and all corresponding passwords and IDs.
- Prefer GitHub environment secrets if release signing should require additional approvals.
- Use a temporary CI keychain and delete it at job teardown.
- Rotate the App Store Connect API key and signing certificates when personnel or access requirements change.
- Avoid embedding signing secrets in the repository, build logs, or generated artifacts.

## Current Decision

CoolBox does not currently implement macOS signing or notarization in CI. The recommended future design is:

1. Sign macOS app bundles and executable payloads with `Developer ID Application`.
2. Notarize the final macOS release archive with `notarytool`.
3. Staple the resulting ticket when the artifact type supports it.
4. Publish only the signed and notarized macOS release assets.