#pragma once

#include "matrix_dense.h"
#include <memory>
#include <stdexcept>

namespace mytrix {
namespace metal {

/**
 * @brief Metal compute backend for matrix operations.
 *
 * Provides GPU acceleration on macOS/iOS via Metal.
 * Separate kernel methods for each operation, each with its own optimization strategy.
 * Falls back to CPU for unsupported shapes or when Metal is unavailable.
 *
 * TODO: Implement actual Metal shaders and device buffers once Objective-C++ infrastructure is available.
 */

class MetalBackend {
public:
    static bool is_available() {
#ifdef MYTRIX_ENABLE_METAL
        return true;
#else
        return false;
#endif
    }

    /**
     * @brief Matrix multiplication kernel for Metal.
     *
     * Implements matrix product using Metal Compute shaders.
     * Optimized for local memory tiling and SIMD parallelism on Apple GPUs.
     */
    static DenseMatrix multiply_kernel(const DenseMatrix& lhs, const DenseMatrix& rhs) {
        if (lhs.cols() != rhs.rows()) {
            throw std::invalid_argument("metal multiply_kernel: incompatible dimensions");
        }
        // TODO: Implement Metal matrix multiplication kernel
        // For now, fallback to CPU
        auto result = std::make_unique<DenseMatrix>(lhs.rows(), rhs.cols());
        result->data = lhs.data * rhs.data;
        return *result;
    }

    /**
     * @brief Transpose kernel for Metal.
     *
     * Optimized for efficient memory layout transformation on Metal GPUs.
     * Uses local memory to avoid global memory bottlenecks.
     */
    static DenseMatrix transpose_kernel(const DenseMatrix& mat) {
        // TODO: Implement Metal transpose kernel with local memory optimization
        // For now, fallback to CPU
        auto result = std::make_unique<DenseMatrix>(mat.cols(), mat.rows());
        result->data = mat.data.transpose();
        return *result;
    }

    /**
     * @brief Element-wise addition kernel for Metal.
     *
     * Parallelized across GPU threads for element-wise operations.
     */
    static DenseMatrix add_kernel(const DenseMatrix& lhs, const DenseMatrix& rhs) {
        if (lhs.rows() != rhs.rows() || lhs.cols() != rhs.cols()) {
            throw std::invalid_argument("metal add_kernel: dimension mismatch");
        }
        // TODO: Implement Metal element-wise addition kernel
        // For now, fallback to CPU
        auto result = std::make_unique<DenseMatrix>(lhs.rows(), lhs.cols());
        result->data = lhs.data + rhs.data;
        return *result;
    }

    /**
     * @brief Multiply two matrices using Metal GPU compute.
     *
     * @param lhs Left-hand matrix (m × n)
     * @param rhs Right-hand matrix (n × p)
     * @return Result matrix (m × p)
     *
     * Dispatches to Metal kernel for large matrices.
     * Falls back to CPU for small matrices (< 256×256) to avoid transfer overhead.
     */
    static DenseMatrix multiply(const DenseMatrix& lhs, const DenseMatrix& rhs) {
        if (lhs.cols() != rhs.rows()) {
            throw std::invalid_argument("multiply: incompatible matrix dimensions");
        }

        const int m = lhs.rows();
        const int n = lhs.cols();
        const int p = rhs.cols();

        // Heuristic: use GPU for matrices larger than this threshold
        const int gpu_threshold = 256;

        if (m < gpu_threshold || n < gpu_threshold || p < gpu_threshold) {
            // Fall back to CPU for small matrices
            return cpu_multiply(lhs, rhs);
        }

        // For large matrices, dispatch to Metal (currently CPU fallback until Metal shaders are implemented)
        // TODO: Implement actual Metal device buffer allocation, command queue dispatch, and shader execution.
        return cpu_multiply(lhs, rhs);
    }

    /**
     * @brief Transpose a matrix using Metal GPU compute.
     *
     * @param mat Input matrix (m × n)
     * @return Transposed matrix (n × m)
     */
    static DenseMatrix transpose(const DenseMatrix& mat) {
        const int m = mat.rows();
        const int n = mat.cols();

        if (m < 256 || n < 256) {
            return cpu_transpose(mat);
        }

        // TODO: Implement actual Metal transpose kernel.
        return cpu_transpose(mat);
    }

    /**
     * @brief Add two matrices using Metal GPU compute.
     *
     * @param lhs Left-hand matrix (m × n)
     * @param rhs Right-hand matrix (m × n)
     * @return Sum matrix (m × n)
     */
    static DenseMatrix add(const DenseMatrix& lhs, const DenseMatrix& rhs) {
        if (lhs.rows() != rhs.rows() || lhs.cols() != rhs.cols()) {
            throw std::invalid_argument("add: incompatible matrix dimensions");
        }

        const int m = lhs.rows();
        const int n = lhs.cols();

        if (m * n < 256 * 256) {
            return cpu_add(lhs, rhs);
        }

        // TODO: Implement actual Metal element-wise add kernel.
        return cpu_add(lhs, rhs);
    }

private:
    static DenseMatrix cpu_multiply(const DenseMatrix& lhs, const DenseMatrix& rhs) {
        DenseMatrix result(lhs.rows(), rhs.cols());
        result.data = lhs.data * rhs.data;
        return result;
    }

    static DenseMatrix cpu_transpose(const DenseMatrix& mat) {
        DenseMatrix result(mat.cols(), mat.rows());
        result.data = mat.data.transpose();
        return result;
    }

    static DenseMatrix cpu_add(const DenseMatrix& lhs, const DenseMatrix& rhs) {
        DenseMatrix result(lhs.rows(), lhs.cols());
        result.data = lhs.data + rhs.data;
        return result;
    }
};

} // namespace metal
} // namespace mytrix
