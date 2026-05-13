# mytrix / MATRIX Package Fixes

`mytrix` is the internal matrix library in `_deliverables/libraries/groups/cool_car/MATRIX/`. After the file reorganization it was undefined as a CMake target and its header API was incomplete.

## CMake Target

`mytrix` is now an INTERFACE library defined in `MATRIX/CMakeLists.txt`. It exposes headers via `$<BUILD_INTERFACE:...>` and links `Eigen3::Eigen` when found.

## Header Rewrites (`matrix_dense.h`)

`DenseMatrix` and `DenseVector` were rewritten with a full API backed by `Eigen::MatrixXd` / `Eigen::VectorXd` internally:

### DenseMatrix additions
- Constructors: default, `(int r, int c)`, from `Eigen::MatrixXd` (implicit), from flat `std::vector<double>` + dimensions.
- Conversion operator `operator Eigen::MatrixXd()` for test interop.
- `at(int, int)`, `at(size_t, size_t)`, `operator()(int, int)`.
- `static Zero(r, c)`, `static Identity(n)`.
- `operator+`, `operator-`, `operator*` returning `DenseMatrix`.
- `multiply()`, `transpose()`, `add()` returning `unique_ptr<MatrixBase>`.
- `norm()` — Frobenius norm via `data.norm()`.
- `array()` — returns `data.array()` for element-wise Eigen ops (e.g. `.array().isFinite().all()`).

### DenseVector additions
- Constructors: default, `(int n)`, `(int n, double val)`, `(size_t n, double val)`, from `std::initializer_list<double>`, from iterator range, from `Eigen::VectorXd` (implicit).
- Conversion operator `operator Eigen::VectorXd()`.
- `size()`, `empty()`, `begin()`, `end()`.
- `operator[]` and `at()` for both `int` and `size_t`.
- `operator()(int)` for Eigen-style indexing.
- `operator+`, `operator-`.

## Namespace Alias

```cpp
namespace matrix = mytrix;
```

Added at the end of `matrix_dense.h` so existing code using `matrix::DenseMatrix` continues to compile.

## Broken Includes Fixed

Five ML headers had stale relative paths to the old matrix location. All updated to `"matrix_dense.h"` or `"mytrix_eigen_compat.hpp"`:

- `kalman_filter.h`
- `extended_kalman_filter.h`
- `gabor_patches.h`
- `bayesian_network.h`
- `marked_point_process.h`
- `piecewise_conditional_intensity_model.h`

## Template Syntax Fixes

`DenseMatrix` is not a class template. All occurrences of `DenseMatrix<double>` and `DenseMatrix<Scalar>` across ML source/headers were replaced with plain `DenseMatrix`.

Files affected: `fourier_tranforms.hpp`, `umap.cpp`, `pca.cpp`, `pca.h`, `hidden_markov_model.cpp`, `marked_point_process.cpp`, `marked_point_process.h`, `support_vector_machine.cpp`, `latent_sentiment_analysis.h`, `gabor_patches.h`.
