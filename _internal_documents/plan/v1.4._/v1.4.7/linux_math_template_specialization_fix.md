# v1.4.7 Linux MATH Template Specialization Fix

## Issue

Linux CI failed while compiling `math_tests` with errors from:

- `_deliverables/libraries/groups/cool_car/MATH/pde_solver.hpp`
- `_deliverables/libraries/groups/cool_car/MATH/spde.hpp`

Representative errors:

- `explicit specialization in non-namespace scope`
- `too few template-parameter-lists`

## Root cause

Both classes (`PDESolver`, `SPDESolver`) declared an in-class trait and then attempted to explicitly specialize it inside the class body:

- `template<> struct is_mytrix_matrix<mytrix::DenseMatrix> ...`

C++ does not allow explicit specializations in class scope; explicit specialization must occur at namespace scope.

## Fix

Updated both headers to remove illegal in-class explicit specializations and rely on direct type checks in the ND dispatch branch:

- `std::is_same_v<std::remove_cv_t<std::remove_reference_t<typename NDArray::value_type>>, mytrix::DenseMatrix>`

Files changed:

- `_deliverables/libraries/groups/cool_car/MATH/pde_solver.hpp`
- `_deliverables/libraries/groups/cool_car/MATH/spde.hpp`

## Validation

Recommended CI/local verification:

1. Build `math_tests` target.
2. Confirm `pde_solver.hpp` / `spde.hpp` no longer emit specialization-scope errors.
3. Run `math_tests` if enabled in the current build profile.

## Impact

- Restores Linux compilation for the MATH test target.
- No algorithmic behavior change intended; this is a template legality/compile fix.
