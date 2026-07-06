# mars_regression_glm_integration

## Scope
- Add MARS regression implementation to the ML package.
- Integrate with generalized_linear_model abstractions.
- Use mytrix matrix utilities in coefficient solving path.
- Wire tests through tyst framework conventions.

## Module Added
- mars_regression under cool_car ML group.

## Design

### Integration Strategy
- MarsRegressionFitMethod inherits from FitMethod.
- MarsRegression inherits from GLM.
- MARS basis functions are modeled as hinge terms:
  - max(0, x_j - knot)
  - max(0, knot - x_j)

### Training Flow
1. Start with intercept-only model.
2. Generate candidate knots per feature.
3. Greedy forward stagewise term addition:
   - Evaluate candidate basis terms.
   - Solve coefficients for each candidate model.
   - Keep the best RSS improvement.
4. Stop when no improvement or term budget is reached.

### Numerical Solve
- Build design matrix with mytrix DenseMatrix.
- Use regularized least squares:
  - (X^T X + lambda I) beta = X^T y
- Solve with LDLT and fallback to complete orthogonal decomposition when needed.

## Build Wiring
- Added mars_regression CMake module under ML.
- Linked against:
  - generalized_linear_model
  - mytrix
- Added tyst-based tests under conditional BUILD_TESTING and tyst target checks.

## API Surface
- fit(X, y)
- predict(sample)
- predict_batch(X)
- num_terms()
- get_terms()

## Validation
- CMake reconfiguration shows mars_regression discovered under ML.
- Successful target build:
  - mars_regression
- Diagnostics for module header/source/tests show no errors.

## Notes
- tyst test target generation remains conditional on global build settings and target availability.
