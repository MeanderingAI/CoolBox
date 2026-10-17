#pragma once

#include <memory>
#include <stdexcept>
#include "matrix_dense.h"

namespace mytrix {
namespace opencl {

/**
 * @brief OpenCL compute backend for matrix operations.
 *
 * Provides cross-platform GPU acceleration via OpenCL.
 * Works on AMD, Intel, NVIDIA, and other OpenCL-compliant devices.
 * Separate kernel methods for each operation.
 * Falls back to CPU when OpenCL is unavailable.
 *
 * TODO: Implement actual OpenCL kernels and device management.
 */
class OpenCLBackend {
public:
    static bool is_available() {
#ifdef MYTRIX_ENABLE_OPENCL
        return true;
#else
        return false;
#endif
    }

    /**
     * @brief Matrix multiplication kernel for OpenCL.
     *
     * Implements matrix product using OpenCL compute kernels.
     * Portable across GPU vendors.
     */
    static DenseMatrix multiply_kernel(const DenseMatrix& lhs, const DenseMatrix& rhs) {
        if (lhs.cols() != rhs.rows()) {
            throw std::invalid_argument("opencl multiply_kernel: incompatible dimensions");
        }
        // TODO: Implement OpenCL matrix multiplication kernel
        // For now, fallback to CPU
        auto result = std::make_unique<DenseMatrix>(lhs.rows(), rhs.cols());
        result->data = lhs.data * rhs.data;
        return *result;
    }

    /**
     * @brief Transpose kernel for OpenCL.
     *
     * Optimized for efficient global memory access patterns.
     */
    static DenseMatrix transpose_kernel(const DenseMatrix& mat) {
        // TODO: Implement OpenCL transpose kernel with local memory optimization
        // For now, fallback to CPU
        auto result = std::make_unique<DenseMatrix>(mat.cols(), mat.rows());
        result->data = mat.data.transpose();
        return *result;
    }

    /**
     * @brief Element-wise addition kernel for OpenCL.
     *
     * Simple parallelized element-wise operation.
     */
    static DenseMatrix add_kernel(const DenseMatrix& lhs, const DenseMatrix& rhs) {
        if (lhs.rows() != rhs.rows() || lhs.cols() != rhs.cols()) {
            throw std::invalid_argument("opencl add_kernel: dimension mismatch");
        }
        // TODO: Implement OpenCL element-wise addition kernel
        // For now, fallback to CPU
        auto result = std::make_unique<DenseMatrix>(lhs.rows(), lhs.cols());
        result->data = lhs.data + rhs.data;
        return *result;
    }

    /**
     * @brief Public dispatch method for multiplication.
     * Routes through kernel with size-based threshold heuristic.
     */
    static DenseMatrix multiply(const DenseMatrix& lhs, const DenseMatrix& rhs) {
        const int gpu_threshold = 256;
        if (lhs.rows() * rhs.cols() > gpu_threshold * gpu_threshold) {
            return multiply_kernel(lhs, rhs);
        }
        // CPU fallback for small matrices
        auto result = std::make_unique<DenseMatrix>(lhs.rows(), rhs.cols());
        result->data = lhs.data * rhs.data;
        return *result;
    }

    /**
     * @brief Public dispatch method for transpose.
     */
    static DenseMatrix transpose(const DenseMatrix& mat) {
        const int gpu_threshold = 256;
        if (mat.rows() * mat.cols() > gpu_threshold * gpu_threshold) {
            return transpose_kernel(mat);
        }
        auto result = std::make_unique<DenseMatrix>(mat.cols(), mat.rows());
        result->data = mat.data.transpose();
        return *result;
    }

    /**
     * @brief Public dispatch method for addition.
     */
    static DenseMatrix add(const DenseMatrix& lhs, const DenseMatrix& rhs) {
        const int gpu_threshold = 256;
        if (lhs.rows() * lhs.cols() > gpu_threshold * gpu_threshold) {
            return add_kernel(lhs, rhs);
        }
        auto result = std::make_unique<DenseMatrix>(lhs.rows(), lhs.cols());
        result->data = lhs.data + rhs.data;
        return *result;
    }
};

} // namespace opencl
} // namespace mytrix
