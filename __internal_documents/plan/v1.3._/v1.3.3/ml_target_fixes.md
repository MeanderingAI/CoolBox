# ML Target Build and Runtime Fixes

All fixes target the ML libraries under `_deliverables/libraries/groups/cool_car/ML/`.

## bayesian_network_db

**Problem:** `std::unordered_map<std::unordered_map<std::string,std::string>, int>` — `unordered_map` cannot be used as a key because `std::hash` is not defined for it.

**Fix:** Replaced the counts map with `std::unordered_map<std::string, int>` using a lambda that serializes each row to a sorted `key=value;` string. Added `#include <map>` for `std::map` used in the sort.

**File:** `bayesian_network_db/source/bayesian_network_db.cpp`

---

## latent_sentiment_analysis

**Build problem:** `'norm'` and `'array'` not members of `mytrix::DenseMatrix`.

**Fix:** Added `norm()` and `array()` to `DenseMatrix` in `matrix_dense.h`.

**Runtime problem:** `predict_score()` returned 0 — both `sgd_step()` and `train()` were stubs.

**Fix:** Implemented matrix factorization SGD in `latent_sentiment_analysis.cpp`:
- `sgd_step(i, j, error)` updates U[i,:] and V[j,:] with gradient + L2 regularization.
- `train()` iterates over all (document, term) pairs each epoch (including zeros, so regularization suppresses unobserved entries).

**File:** `latent_sentiment_analysis/source/latent_sentiment_analysis.cpp`

---

## hidden_markov_model

**Problem:** Tests passed `Eigen::VectorXd pi` to `set_initial_probabilities(const std::vector<double>&)`.

**Fix:** Replaced both occurrences with `std::vector<double>` using brace initializer syntax.

**File:** `hidden_markov_model/tests/test_hidden_markov_model.cpp`

---

## marked_point_process

**Problems (multiple):**
1. `marked_point_process.cpp` was missing its `#include` and standard library headers.
2. A stray `} // namespace ml` block caused a syntax error (functions use qualified `ml::` names, no block needed).
3. `compute_intensity()` and `compute_compensator()` were declared in the header but had no implementation.
4. `piecewise_conditional_intensity_model.h` had a stale relative path to the matrix header.
5. Tests used `Eigen::VectorXd` / `Eigen::MatrixXd` where `std::vector<double>` / `matrix::DenseMatrix` were expected.
6. Test used `matrix::DenseMatrix` from outside namespace `ml` — added `using namespace ml;`.

**Fixes:**
- Added `#include "../headers/marked_point_process.h"`, `<vector>`, `<random>`, `<numeric>`, `<algorithm>` to the cpp.
- Removed stray `} // namespace ml`.
- Implemented `compute_intensity()` as Hawkes-style exponential decay kernel.
- Implemented `compute_compensator()` as closed-form integral of the kernel.
- Fixed include path in `piecewise_conditional_intensity_model.h`.
- Updated both test files to use `std::vector` / `matrix::DenseMatrix` and added `using namespace ml;`.

**Files:** `marked_point_process/source/marked_point_process.cpp`, `marked_point_process/headers/piecewise_conditional_intensity_model.h`, `marked_point_process/tests/test_marked_point_process.cpp`, `marked_point_process/tests/test_piecewise_conditional_intensity_model.cpp`

---

## gabor_patches

**Problem:** Template functions declared as `matrix::DenseMatrix<Scalar> gabor_kernel(...)` — `DenseMatrix` is not a class template.

**Fix:** Stripped `<Scalar>` from all `matrix::DenseMatrix<Scalar>` occurrences in `gabor_patches.h`.

**File:** `gabor_patches/headers/gabor_patches.h`

---

## dimensionality_reduction (PCA)

**Problem:** Tests assigned `pca.get_explained_variance_ratio()` (returns `std::vector<double>`) to `Eigen::VectorXd`.

**Fix:** Replaced assignment with `Eigen::Map<const Eigen::VectorXd>(ratio_vec.data(), ratio_vec.size())`.

**File:** `dimensionality_reduction/tests/test_pca.cpp`

---

## tracker (Kalman Filter / EKF)

**Problem 1 — KalmanFilter:** `inverse2x2()` threw at runtime if S was not exactly 2×2 (`"Only 2x2 inverse implemented"`).

**Problem 2 — ExtendedKalmanFilter:** Used `Matrix::Identity(S.rows())` as the innovation covariance inverse instead of a real inverse, causing wrong numerical results.

**Fix:** Both replaced with `Matrix(M.data.inverse())` using Eigen's general LU-based matrix inverse.

**Files:** `tracker/source/kalman_filter.cpp`, `tracker/source/extended_kalman_filter.cpp`
