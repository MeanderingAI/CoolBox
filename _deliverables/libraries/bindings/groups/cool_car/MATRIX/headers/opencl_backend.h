#pragma once

#include <memory>
#include <stdexcept>
#include "matrix_dense.h"

namespace mytrix {
namespace opencl {

/**
 * @brief OpenCL backend placeholder for matrix operations.
 *
 * No OpenCL kernels or device management are implemented yet. This backend
 * must remain unavailable until it can execute operations on an OpenCL device.
 */
class OpenCLBackend {
public:
    static bool is_available() {
        return false;
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
        throw std::runtime_error("OpenCL backend is not implemented");
    }

    /**
     * @brief Transpose kernel for OpenCL.
     *
     * Optimized for efficient global memory access patterns.
     */
    static DenseMatrix transpose_kernel(const DenseMatrix& mat) {
        throw std::runtime_error("OpenCL backend is not implemented");
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
        throw std::runtime_error("OpenCL backend is not implemented");
    }

    /**
     * @brief Public dispatch method for multiplication.
     * Routes through kernel with size-based threshold heuristic.
     */
    static DenseMatrix multiply(const DenseMatrix& lhs, const DenseMatrix& rhs) {
        return multiply_kernel(lhs, rhs);
    }

    /**
     * @brief Public dispatch method for transpose.
     */
    static DenseMatrix transpose(const DenseMatrix& mat) {
        return transpose_kernel(mat);
    }

    /**
     * @brief Public dispatch method for addition.
     */
    static DenseMatrix add(const DenseMatrix& lhs, const DenseMatrix& rhs) {
        return add_kernel(lhs, rhs);
    }
};

} // namespace opencl
} // namespace mytrix
