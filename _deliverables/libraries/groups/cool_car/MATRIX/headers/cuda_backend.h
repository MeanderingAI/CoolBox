#pragma once

#include <memory>
#include <stdexcept>
#include "matrix_dense.h"

namespace mytrix {
namespace cuda {

/**
 * @brief CUDA compute backend for matrix operations.
 *
 * Provides GPU acceleration on Windows/NVIDIA via CUDA.
 * Separate kernel methods for each operation.
 * Falls back to CPU for unsupported shapes or when CUDA is unavailable.
 *
 * TODO: Implement actual CUDA kernels once CUDA SDK is integrated.
 */
class CudaBackend {
public:
    static bool is_available() {
#ifdef MYTRIX_ENABLE_CUDA
        return true;
#else
        return false;
#endif
    }

    /**
     * @brief Matrix multiplication kernel for CUDA.
     *
     * Optimal for large dense matrices on NVIDIA GPUs.
     * Uses CUBLAS or custom kernel implementation.
     */
    static DenseMatrix multiply_kernel(const DenseMatrix& lhs, const DenseMatrix& rhs) {
        if (lhs.cols() != rhs.rows()) {
            throw std::invalid_argument("cuda multiply_kernel: incompatible dimensions");
        }
        // TODO: Implement CUDA matrix multiplication kernel
        // For now, fallback to CPU
        auto result = std::make_unique<DenseMatrix>(lhs.rows(), rhs.cols());
        result->data = lhs.data * rhs.data;
        return *result;
    }

    /**
     * @brief Transpose kernel for CUDA.
     *
     * Optimized for coalesced memory access patterns on NVIDIA GPUs.
     */
    static DenseMatrix transpose_kernel(const DenseMatrix& mat) {
        // TODO: Implement CUDA transpose kernel with optimized memory layout
        // For now, fallback to CPU
        auto result = std::make_unique<DenseMatrix>(mat.cols(), mat.rows());
        result->data = mat.data.transpose();
        return *result;
    }

    /**
     * @brief Element-wise addition kernel for CUDA.
     *
     * Parallelized element-wise operation across GPU threads.
     */
    static DenseMatrix add_kernel(const DenseMatrix& lhs, const DenseMatrix& rhs) {
        if (lhs.rows() != rhs.rows() || lhs.cols() != rhs.cols()) {
            throw std::invalid_argument("cuda add_kernel: dimension mismatch");
        }
        // TODO: Implement CUDA element-wise addition kernel
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

} // namespace cuda
} // namespace mytrix
