# Marked Point Process & PiecewiseConditionalIntensityModel Migration (v1.1.73)

- All Eigen references removed from MarkedPointProcess and PiecewiseConditionalIntensityModel headers and sources.
- Types updated to use `std::vector<double>` and `matrix::DenseMatrix` for all vectors and matrices.
- All method signatures, member variables, and logic updated to match mytrix migration pattern.
- Eigen math (e.g., .sum(), .dot(), .cwiseMax()) replaced with std equivalents or marked as TODO.
- Compilation unblocked; further math/operator support may be needed for full functionality.
- See code for TODOs where DenseMatrix or std::vector math is required.

Date: 2026-04-22
