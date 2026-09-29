# Swift Extension CI Setup Fix (v1.5.0)

## Issue

`generate_purchase_swift` failed during `swift-actions/setup-swift@v3` with a `swiftly` post-install error:

- `Could not open lock file /var/lib/dpkg/lock-frontend`
- `Unable to acquire the dpkg frontend lock ... are you root?`

## Root cause

The v3 action path uses `swiftly`, whose post-install script attempted package-manager operations requiring elevated privileges in the runner context.

## Fix

Updated `.github/workflows/generate-purchase-swift.yaml`:

- switched setup action from `swift-actions/setup-swift@v3` to `swift-actions/setup-swift@v2`
- kept `swift-version: "5.9"`

## Impact

Avoids the `swiftly` post-install dpkg lock failure path and restores Swift toolchain provisioning for purchase builds.
