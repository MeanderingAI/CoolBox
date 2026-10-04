# Java Extension Prebuilt Archive Fixes (v1.5.0)

## Issue

Java purchase packaging failed while preparing native runtime assets with:

- `tar (child): .prebuilt/release-assets/coolbox-misc-linux-x86_64-<tag>.tar.gz: Cannot open: No such file or directory`

## Root causes

- Workflow treated `coolbox-misc-...tar.gz` as mandatory, but some releases only publish different aggregate archives.
- Workflow still referenced legacy `_libraries/java_bindings` paths for Maven build and staged target copy.

## Fix

Updated `.github/workflows/generate-purchase-java.yaml`:

- Made misc archive download optional (`|| true`).
- Added optional fallback download for `coolbox-libraries-linux-x86_64-<tag>.tar.gz`.
- In native asset prep:
  - extract `coolbox-misc` if present;
  - else extract `coolbox-libraries` if present;
  - else continue with C bindings archive only.
- Added explicit validation/error if required C bindings archive is missing.
- Switched Java module paths to current layout:
  - Maven `-f _deliverables/libraries/bindings/java_bindings/pom.xml`
  - package staging copy from `_deliverables/libraries/bindings/java_bindings/target/`

## Impact

Prevents Java packaging failures when `coolbox-misc` is absent and aligns Java binding build/staging with the active repository directory structure.
