# Go Windows bridge remaining smoke-test ABI follow-up

- Added dedicated `cbridge` translation units for HMM, Bayesian network, multi-arm bandit, and GUI constructor ABI so Windows Go builds no longer depend on the truncated placeholder bridge for the remaining smoke-tested modules.
- Kept GUI bridge coverage intentionally narrow to the constructors/free functions exercised by `extracted_modules_test.go`, avoiding unnecessary chart/rendering dependencies while restoring the required exported symbols.
- Extended `coolboxbridge` CMake target to compile the new units alongside the earlier PCA/SVM bridge implementation.