/**
 * @file gabor_patches.h
 * @brief Gabor filter / patch generation library.
 *
 * Generates 2-D Gabor kernels and filter-banks commonly used in computer
 * vision (texture analysis, edge detection) and computational neuroscience
 * (modelling simple-cell receptive fields in V1).
 *
 * A Gabor function is a Gaussian envelope modulated by a sinusoid:
 *
 *   g(x, y) = exp(-(x'^2 + γ² y'^2) / (2σ²)) · cos(2π x'/λ + ψ)
 *
 * where
 *   x' =  x cos θ + y sin θ
 *   y' = -x sin θ + y cos θ
 *
 * Parameters:
 *   λ  (lambda)  – wavelength of the sinusoidal carrier
 *   θ  (theta)   – orientation of the filter (radians)
 *   ψ  (psi)     – phase offset of the cosine
 *   σ  (sigma)   – standard deviation of the Gaussian envelope
 *   γ  (gamma)   – spatial aspect ratio (ellipticity)
 *
 * Usage:
 * @code{.cpp}
 * using namespace ml;
 *
 * // Single kernel
 * GaborParams<double> p{4.0, M_PI / 4, 0.0, 2.0, 0.5};
 * Eigen::MatrixXd kernel = gabor_kernel(p, 21);
 *
 * // Filter bank (8 orientations × 3 scales)
 * GaborFilterBank<double> bank;
 * bank.add_orientations(8);
 * bank.add_wavelengths({4.0, 8.0, 16.0});
 * bank.build();
 *
 * auto responses = bank.apply(image);   // image is MatrixXd
 * @endcode
 */
#pragma once


#include "mytrix_eigen_compat.hpp"
#include <vector>
#include <cmath>
#include <stdexcept>
#include <string>
#include <sstream>
#include <iomanip>
#include "matrix_dense.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace ml {

// ===================================================================
// Parameter structure
// ===================================================================

/**
 * @brief Parameters that fully describe a single 2-D Gabor kernel.
 */
template <typename Scalar = double>
struct GaborParams {
    Scalar lambda = 4.0;   ///< Wavelength of sinusoidal carrier (pixels)
    Scalar theta  = 0.0;   ///< Orientation in radians
    Scalar psi    = 0.0;   ///< Phase offset in radians
    Scalar sigma  = 2.0;   ///< Std-dev of Gaussian envelope
    Scalar gamma  = 0.5;   ///< Spatial aspect ratio (ellipticity)

    /** Compute a reasonable kernel size (odd) that covers ≥3σ in each direction. */
    int default_kernel_size() const;

    std::string to_string() const;
};

// ===================================================================
// Free-function declarations
// ===================================================================

/**
 * @brief Generate a 2-D Gabor kernel (real / cosine part).
 *
 * @param params    Gabor parameters.
 * @param size      Kernel side length (must be odd and ≥ 1). 0 = auto from σ.
 * @param normalize If true the kernel is L2-normalised.
 */
template <typename Scalar = double>
matrix::DenseMatrix gabor_kernel(const GaborParams<Scalar>& params, int size = 0,
             bool normalize = false);

/**
 * @brief Generate the imaginary (sine) part of the Gabor function.
 */
template <typename Scalar = double>
matrix::DenseMatrix gabor_kernel_imaginary(const GaborParams<Scalar>& params, int size = 0,
                       bool normalize = false);

/**
 * @brief Gabor energy: sqrt(real² + imag²) per pixel.
 */
template <typename Scalar = double>
matrix::DenseMatrix gabor_energy(const GaborParams<Scalar>& params, int size = 0);

/**
 * @brief 2-D convolution with valid padding.
 */
template <typename Scalar = double>
matrix::DenseMatrix convolve2d(const matrix::DenseMatrix& image,
           const matrix::DenseMatrix& kernel);

/**
 * @brief 2-D convolution with zero-padding (output same size as input).
 */
template <typename Scalar = double>
matrix::DenseMatrix convolve2d_same(const matrix::DenseMatrix& image,
                const matrix::DenseMatrix& kernel);

// ===================================================================
// Filter bank class
// ===================================================================

/**
 * @brief A bank of Gabor filters spanning multiple orientations and scales.
 */
template <typename Scalar = double>
class GaborFilterBank {
public:
    using MatrixT = matrix::DenseMatrix;

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

    // Apply
    std::vector<MatrixT> apply(const MatrixT& image, bool same = true) const;
    matrix::DenseMatrix mean_response(const MatrixT& image, bool same = true) const;
    std::vector<MatrixT> apply_energy(const MatrixT& image, bool same = true) const;

    // Accessors
    size_t num_kernels() const { return kernels_.size(); }
    size_t num_orientations() const { return thetas_.size(); }
    size_t num_wavelengths() const { return lambdas_.size(); }
    bool is_built() const { return built_; }
    const std::vector<MatrixT>& kernels() const { return kernels_; }
    const std::vector<GaborParams<Scalar>>& params() const { return params_; }
    std::string to_string() const;

private:
    std::vector<Scalar> thetas_;
    std::vector<Scalar> lambdas_;
    Scalar sigma_       = Scalar(0);
    Scalar gamma_       = Scalar(0.5);
    Scalar psi_         = Scalar(0);
    int    kernel_size_  = 0;

    bool built_ = false;
    std::vector<MatrixT> kernels_;
    std::vector<GaborParams<Scalar>> params_;
};

} // namespace ml
