# Linux Release Signing

## Goal

Set up a reproducible Linux release-signing flow so CoolBox release artifacts can be authenticated by downstream users, package managers, and automation systems.

## Important Difference From Windows And macOS

Linux does not have one universal platform trust mechanism equivalent to Windows Authenticode or macOS Developer ID plus notarization.

In practice, Linux signing usually means one or more of these:

- signing release archives and checksum manifests with GPG
- signing package repositories or package metadata
- signing individual package formats such as `.deb`, `.rpm`, or container images
- optionally signing ELF binaries with tools such as `signelf`, though this is much less common as the primary trust channel

For this repository, the most practical first step is to sign release archives and checksum files with a dedicated GPG release key.

## Recommended Approach

Use a dedicated CoolBox release GPG key for Linux distribution artifacts.

- Sign `SHA256SUMS` and release manifests.
- Optionally create detached signatures for each published `.tar.gz`, `.zip`, `.crate`, or other archive.
- If CoolBox later publishes apt, yum, or dnf repositories, use the same release key or a dedicated repository signing key for metadata signing.

This approach fits the current repository better than trying to embed signatures directly into every Linux binary.

## What You Need

- A maintainer-owned GPG keypair dedicated to release signing.
- A published public key that users can import and trust.
- A decision on whether CI should hold the private key directly or use an external signing service.
- A passphrase strategy suitable for CI, or a hardware-backed/offline signing process if stronger controls are required.

## Key Strategy Options

### 1. Dedicated GPG release key

Recommended for the current repository.

- Create a dedicated signing identity for CoolBox releases.
- Export the private key for CI only if the project accepts storing it in GitHub secrets.
- Publish the armored public key in the repository, release notes, website, or documentation.

### 2. Offline signing key

More secure, but not aligned with the current fully automated release workflows.

- CI builds the artifacts.
- A maintainer downloads them and signs them offline.
- Signed artifacts are uploaded afterward.

### 3. Sigstore or keyless signing

Worth considering later if the project wants transparent signing with OIDC-backed identity.

- Good fit for container images and provenance.
- More workflow redesign is required than for basic GPG-signed archives.

## Suggested GitHub Actions Secrets

If CI will sign Linux releases directly with GPG, store:

- `COOLBOX_LINUX_GPG_PRIVATE_KEY_BASE64`
  Base64-encoded ASCII-armored or binary secret key export.
- `COOLBOX_LINUX_GPG_PASSPHRASE`
  Passphrase for the private key.
- `COOLBOX_LINUX_GPG_KEY_ID`
  Long key ID or fingerprint used for signing.

Optional:

- `COOLBOX_LINUX_GPG_PUBLIC_KEY_ASC`
  Public key text for validation or release-note publication.

## Creating A Release Key

Example local flow:

```bash
gpg --full-generate-key
```

Recommended characteristics:

- RSA 4096 or modern ECC if compatible with your downstream tooling
- dedicated name such as `CoolBox Release Signing`
- email alias intended for release operations
- expiration date with a documented rotation policy

Export examples:

```bash
gpg --armor --export-secret-keys YOUR_KEY_ID > coolbox-release-private.asc
gpg --armor --export YOUR_KEY_ID > coolbox-release-public.asc
base64 < coolbox-release-private.asc | pbcopy
```

On Linux:

```bash
base64 -w 0 coolbox-release-private.asc | xclip -selection clipboard
```

## CI Setup Pattern

The repository already produces Linux release artifacts in workflows such as:

- `.github/workflows/build-libs.yaml`
- `.github/workflows/build-products.yaml`
- `generate-purchase-*` workflows for language bindings

Add Linux signing after release assets are assembled and before final upload or release attachment.

Recommended CI sequence:

1. Decode the GPG private key into `${RUNNER_TEMP}`.
2. Create an isolated `GNUPGHOME`.
3. Import the private key.
4. Configure loopback pinentry for non-interactive signing.
5. Sign `SHA256SUMS` with a detached armored signature.
6. Optionally sign each archive with `gpg --detach-sign --armor`.
7. Upload both the artifacts and their signatures.

## Example CI Commands

```bash
export GNUPGHOME="$RUNNER_TEMP/gnupg"
mkdir -p "$GNUPGHOME"
chmod 700 "$GNUPGHOME"

echo "$COOLBOX_LINUX_GPG_PRIVATE_KEY_BASE64" | base64 --decode > "$RUNNER_TEMP/coolbox-release-private.asc"
gpg --batch --import "$RUNNER_TEMP/coolbox-release-private.asc"

printf 'pinentry-mode loopback\n' > "$GNUPGHOME/gpg.conf"
printf 'allow-loopback-pinentry\n' > "$GNUPGHOME/gpg-agent.conf"

gpgconf --kill gpg-agent || true

gpg --batch --yes --pinentry-mode loopback \
  --passphrase "$COOLBOX_LINUX_GPG_PASSPHRASE" \
  --local-user "$COOLBOX_LINUX_GPG_KEY_ID" \
  --armor --detach-sign release-assets/SHA256SUMS

for artifact in release-assets/*.tar.gz release-assets/*.zip release-assets/*.crate; do
  if [ -f "$artifact" ]; then
    gpg --batch --yes --pinentry-mode loopback \
      --passphrase "$COOLBOX_LINUX_GPG_PASSPHRASE" \
      --local-user "$COOLBOX_LINUX_GPG_KEY_ID" \
      --armor --detach-sign "$artifact"
  fi
done
```

## What To Publish Alongside Linux Releases

At minimum, publish:

- the artifact itself
- `SHA256SUMS`
- `SHA256SUMS.asc` or `SHA256SUMS.sig`
- optionally `artifact-name.tar.gz.asc` detached signatures per artifact
- the public GPG key or a stable link to it

This gives users a simple verification flow.

## User Verification Example

```bash
gpg --import coolbox-release-public.asc
gpg --verify SHA256SUMS.asc SHA256SUMS
sha256sum --check SHA256SUMS
```

For a detached archive signature:

```bash
gpg --verify coolbox-go-bindings-linux-x86_64-v1.2.3.tar.gz.asc coolbox-go-bindings-linux-x86_64-v1.2.3.tar.gz
```

## Package-Specific Notes

### Debian packages

- `.deb` payload signing is usually not the main trust layer.
- apt repository metadata signing is the usual distribution mechanism.
- If CoolBox later publishes an apt repo, sign the repository metadata with the release key.

### RPM packages

- RPM supports embedded package signatures.
- If CoolBox later publishes `.rpm` packages, configure `rpmsign` with a dedicated signing key.

### AppImage or standalone executables

- Detached GPG signatures are common and easy to automate.
- Embedded ELF signing is possible but not the normal primary user-verification method.

### Container images

- Prefer Sigstore `cosign` rather than GPG if container distribution becomes part of the release model.

## Where To Integrate In This Repository

The cleanest first integration point is in the Linux branches of workflows that already produce `release-assets` and `SHA256SUMS`.

- Sign Linux archives in `.github/workflows/build-products.yaml` before artifact upload.
- Sign Linux native library release assets in `.github/workflows/build-libs.yaml` if those archives are published directly.
- Sign Linux language-binding release assets in the relevant `generate-purchase-*` workflow after `finalize-purchase-assets` completes.

## Local Validation Checklist

1. Verify your secret key is available:

```bash
gpg --list-secret-keys
```

2. Produce `SHA256SUMS` for a test artifact.
3. Sign the checksum file.
4. Verify the signature with the exported public key.
5. Verify the checksum against the artifact.

## Security Notes

- Prefer a dedicated release-signing key rather than a maintainer personal key.
- Restrict access to the private key, passphrase, and GitHub secrets to the smallest possible maintainer set.
- Consider GitHub environment protection rules before enabling automatic signing on tag builds.
- Rotate the key before expiration and publish a clear replacement path for users.
- If CI secret exposure risk is unacceptable, switch to offline signing or a managed signing service.

## Current Decision

CoolBox does not currently implement Linux release signing in CI. The recommended future design is:

1. Create a dedicated CoolBox release GPG key.
2. Sign `SHA256SUMS` and optionally each Linux archive in CI.
3. Publish the public key and verification instructions alongside releases.
4. Reevaluate Sigstore or package-repository signing if Linux distribution expands beyond archive downloads.