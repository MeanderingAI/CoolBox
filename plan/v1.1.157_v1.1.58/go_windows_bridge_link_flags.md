# Go Windows Bridge Link Flags

## Summary
- Fixed the Windows Go bindings build so cgo links the `coolboxbridge` static archive on Windows as well as Linux and macOS.
- Resolved undefined reference errors for PCA and SVM bridge functions during the Windows `go test` link step.
- Documented that the Windows Go package had package-level bridge link flags missing even though the CI workflow correctly built the `coolboxbridge` archive.

## Problem
- The Windows Go purchase workflow failed during the final link stage with undefined references to bridge-exported symbols such as:
  - `coolbox_pca_get_explained_variance_ratio`
  - `coolbox_pca_get_mean`
  - `coolbox_pca_transform`
  - `coolbox_create_svm`
  - `coolbox_svm_fit`
  - `coolbox_svm_predict`
- Those symbols belong to the Go `cbridge` static library, not the direct C bindings library.
- The workflow already built `_libraries/go_bindings/cbridge/build/libcoolboxbridge.a`, but the Go package only declared `#cgo` bridge `LDFLAGS` for `darwin` and `linux`.
- On Windows there was no package-level `#cgo windows LDFLAGS` entry for `-L${SRCDIR}/cbridge/build -lcoolboxbridge`, so the linker never pulled in the bridge archive that defines the PCA and SVM functions.

## Files Updated
- `_libraries/go_bindings/bindings.go`

## Change
- Added a Windows-specific package-level cgo linker directive in `bindings.go`:

```text
#cgo windows LDFLAGS: -lstdc++ -L${SRCDIR}/cbridge/build -lcoolboxbridge
```

- Kept the existing Linux and macOS bridge link directives unchanged.
- Left the direct C bindings linkage in `client.go` intact because that file calls the separate `coolbox_c_bindings` C API directly.

## Result
- Windows Go builds now link the same `coolboxbridge` archive that Linux and macOS already use for the PCA, SVM, graphics, and other C++ bridge-backed Go APIs.
- The Windows undefined-reference failures for PCA and SVM should no longer occur solely because the bridge archive was omitted from the final cgo link command.
- Any remaining Windows Go binding failures after this point are more likely to be true bridge implementation gaps or runtime issues rather than missing package-level linker flags.