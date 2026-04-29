# R Bindings Release Archive Name Fix

## Summary
- Fixed the R bindings workflow so the generated zip filename matches the later artifact upload and release upload paths.

## Problem
- The R bindings packaging step created a zip file with an extra `}` in the filename.
- The upload and `gh release upload` steps expected the same filename without that extra character.
- This caused release upload failures such as missing `coolbox-r-bindings-macos-arm64-<tag>.zip`.

## Files Updated
- `.github/workflows/generate-purchase-r.yaml`

## Result
- The packaging step, artifact upload step, and release upload step now use the same archive path.
