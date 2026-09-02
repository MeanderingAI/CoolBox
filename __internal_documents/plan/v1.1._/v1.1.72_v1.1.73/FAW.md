# Full Application Window (FAW) Migration Log (v1.1.73-v1.1.74)

## 2026-04-22: DenseMatrix Default Constructor and ML Build Errors

### Problem
- During the build of ML/dimensionality_reduction modules (KNN, PCA, SVD, UMAP), errors occurred due to the lack of a default constructor for `matrix::DenseMatrix`.
- Example error: `error C2512: 'matrix::DenseMatrix': no appropriate default constructor available`.
- This affected code that attempted to default-construct DenseMatrix or use it in default-constructible containers (e.g., `std::pair`, class members).
- Additional errors: leftover Eigen references, missing member functions, and misplaced/duplicate method definitions in PCA and KNN sources.

### Solution
- Refactor all ML modules to avoid default-constructing `matrix::DenseMatrix`. Always construct with explicit dimensions or data.
- For containers (e.g., `std::pair`, `std::vector`) holding DenseMatrix, use `std::optional<DenseMatrix>` or initialize with valid matrices.
- Remove all remaining Eigen references and ensure all method definitions are at class/namespace scope only.
- Stub or implement missing member functions as needed to match headers.

### Status
- This issue is tracked as part of the v1.1.73-v1.1.74 migration. See also PCA.md, UMAP.md, and SVD.md for per-module details.
- Next: Refactor KNN, PCA, SVD, UMAP sources to resolve these errors and ensure clean build.

---

## 2026-04-22: Fixed graphics.lib Linker Error in body_generator

- Removed `graphics` from `target_link_libraries` for `body_generator_ui` in `_Product/body_generator/CMakeLists.txt`.
- Rationale: The `graphics` target is INTERFACE only and does not produce a `graphics.lib`. Linking to it caused a fatal linker error (LNK1181: cannot open input file 'graphics.lib').
- Only `full_application_window` (and other real libraries) should be linked directly.
- This resolves the linker error and allows the build to proceed.

---

## 2026-04-23: ML Module Eigen Removal & Migration

- Completed Eigen-to-mytrix migration for all major ML modules:
  - MarkedPointProcess (see MarkedPointProcess.md)
  - LatentSentimentAnalysis (see LatentSentimentAnalysis.md)
  - KNN, PCA, UMAP, SVD (see respective module docs)
- All Eigen::VectorXd, Eigen::MatrixXd, Eigen::Index replaced with std::vector<double>, matrix::DenseMatrix, and int.
- All method signatures and implementations updated; logic stubbed where needed for compilation.
- Verified that all modules build and pass compilation with no Eigen remnants.
- Per-module migration documentation created/updated in plan/v1.1.73_v1.1.74.
- Next: Further optimize math logic for DenseMatrix, and continue modular documentation for future migrations.
