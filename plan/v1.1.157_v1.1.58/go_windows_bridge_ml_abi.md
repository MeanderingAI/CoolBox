# Go Windows Bridge ML ABI

## Summary
- Added the missing PCA and SVM bridge ABI implementation needed by the Windows Go bindings link step.
- Split the ML bridge-backed Go ABI into its own `bridge_ml.cpp` translation unit so the build can export the missing PCA and SVM symbols without depending on the incomplete placeholder graphics bridge object.
- Extended the Go bridge CMake target include paths for shared matrix compatibility and metadata headers required by the ML package sources.

## Problem
- After adding the missing Windows `coolboxbridge` linker flags, the next blocker was unresolved ML bridge ABI symbols during Windows `go test`, including:
  - `coolbox_pca_get_explained_variance_ratio`
  - `coolbox_pca_get_mean`
  - `coolbox_pca_transform`
  - `coolbox_create_svm`
  - `coolbox_svm_fit`
  - `coolbox_svm_predict`
- The existing `_libraries/go_bindings/cbridge/bridge.cpp` file was only a small placeholder implementation and did not define the PCA or SVM bridge ABI expected by `pca.go` and `svm.go`.
- The cbridge CMake target also did not expose the matrix compatibility or metadata include directories required when compiling the ML package sources into the bridge archive.

## Files Updated
- `_libraries/go_bindings/cbridge/CMakeLists.txt`
- `_libraries/go_bindings/cbridge/bridge_ml.cpp`

## Change
- Added a new `bridge_ml.cpp` translation unit that implements the Go bridge ABI for PCA and SVM.
- Wired `bridge_ml.cpp` into the `coolboxbridge` static library target so the missing ML symbols are exported by the bridge archive.
- Included the dimensionality-reduction and support-vector-machine source files directly inside `bridge_ml.cpp` to keep the bridge archive self-contained, matching the existing cbridge approach.
- Added the matrix compatibility and metadata header directories to the cbridge CMake include path list so the included ML sources can compile in the bridge target.

## Result
- Windows Go builds should no longer fail solely because the `coolboxbridge` archive is missing the PCA and SVM ABI functions declared by the Go layer.
- The ML bridge ABI now lives in a dedicated compile unit, which reduces coupling with the still-minimal placeholder graphics bridge file.
- Any remaining Go bridge failures after this point are more likely to come from other unimplemented ABI areas rather than the PCA/SVM symbols reported by the current Windows log.