
#ifndef GABOR_PATCHES_H
#define GABOR_PATCHES_H

#include <cstddef> // for size_t
#include <vector>
#include <string>
#include <stdexcept>
#include <cmath>
#include <sstream>
#include <iomanip>
#include "mytrix_eigen_compat.hpp"
#include "matrix_dense.h"

namespace ml {




// ===================================================================
// Parameter structure
// ===================================================================

/**
 * @brief Parameters that fully describe a single 2-D Gabor kernel.
 */

template <typename Scalar>
struct GaborParamsT {
    Scalar lambda = 4.0;   ///< Wavelength of sinusoidal carrier (pixels)
    Scalar theta  = 0.0;   ///< Orientation in radians
    Scalar psi    = 0.0;   ///< Phase offset in radians
    Scalar sigma  = 2.0;   ///< Std-dev of Gaussian envelope
    Scalar gamma  = 0.5;   ///< Spatial aspect ratio (ellipticity)

    /** Compute a reasonable kernel size (odd) that covers ≥3σ in each direction. */
    int default_kernel_size() const;
    std::string to_string() const;
};

using GaborParams = GaborParamsT<double>;

// ===================================================================
// Filter bank class
// ===================================================================

template <typename Scalar>
class GaborFilterBank {

public:
    GaborFilterBank() = default;

    // Configuration
    void add_orientations(int n);
    void add_orientation(Scalar theta);
    void add_wavelengths(const std::vector<Scalar>& lambdas);
    void add_wavelength(Scalar lambda);
    void set_sigma(Scalar sigma);
    void set_gamma(Scalar gamma);
    void set_psi(Scalar psi);
    void set_kernel_size(int size);

    // Build
    void build();
    std::vector<mytrix::DenseMatrix> apply(const mytrix::DenseMatrix& image, bool same = true) const;
    mytrix::DenseMatrix mean_response(const mytrix::DenseMatrix& image, bool same = true) const;
    std::vector<mytrix::DenseMatrix> apply_energy(const mytrix::DenseMatrix& image, bool same = true) const;

    // Accessors
    size_t num_kernels() const { return kernels_.size(); }
    size_t num_orientations() const { return thetas_.size(); }
    size_t num_wavelengths() const { return lambdas_.size(); }
    bool is_built() const { return built_; }
    const std::vector<mytrix::DenseMatrix>& kernels() const { return kernels_; }
    const std::vector<GaborParamsT<Scalar>>& params() const { return params_; }
    std::string to_string() const;

private:
    std::vector<Scalar> thetas_;
    std::vector<Scalar> lambdas_;
    Scalar sigma_       = Scalar(0);
    Scalar gamma_       = Scalar(0.5);
    Scalar psi_         = Scalar(0);
    int    kernel_size_  = 0;

    bool built_ = false;
    std::vector<mytrix::DenseMatrix> kernels_;
    std::vector<GaborParamsT<Scalar>> params_;
};




// Free function template declarations (must be after all struct/class definitions)

template <typename Scalar>
mytrix::DenseMatrix gabor_kernel(const GaborParamsT<Scalar>& params, int size = 0, bool normalize = true);

template <typename Scalar>
mytrix::DenseMatrix gabor_kernel_imaginary(const GaborParamsT<Scalar>& params, int size, bool normalize = true);

template <typename Scalar>
mytrix::DenseMatrix convolve2d(const mytrix::DenseMatrix& image, const mytrix::DenseMatrix& kernel);

template <typename Scalar>
mytrix::DenseMatrix convolve2d_same(const mytrix::DenseMatrix& image, const mytrix::DenseMatrix& kernel);

} // namespace ml

#include "gabor_patches.tpp"

#endif // GABOR_PATCHES_H


