# PCA Module Migration (v1.1.73)

## Migration Summary
- All references to Eigen types removed from PCA header and source files.
- All matrix and vector types replaced with `matrix::DenseMatrix` and `std::vector<double>`.
- All method signatures, member variables, and static helpers updated to use new types.
- Old/duplicate Eigen-based code blocks removed from header.
- Source logic stubbed for DenseMatrix; compilation unblocked, but full math support pending.

## Current Status
- PCA builds without Eigen dependency.
- All Eigen-based helpers and members replaced.
- Logic for mean, std, and preprocessing is stubbed for DenseMatrix (to be implemented).
- Ready for further math/operator support in DenseMatrix.

## Next Steps
- Implement actual math logic for mean, std, and preprocessing using DenseMatrix.
- Add tests for PCA with mytrix types.

---

*This file documents the migration of the PCA module from Eigen to mytrix for v1.1.73.*
